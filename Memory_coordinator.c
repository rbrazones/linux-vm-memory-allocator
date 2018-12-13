/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: Memory_coordinator.c
 *
 *  Description: Main file for memory coordinator project. The memory coordinator will
 *  take in one input parameter that specifies how often (in seconds) that it should
 *  trigger. Upon triggering, the memory coordinator will collect domain and host memory
 *  usage. It will then allocate more memory to domains that require it, will taking away
 *  unused memory from domains that do not need it.
 *
 * ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "Memory_coordinator.h"
#include "metric.h"

#define MEMORY_STATS_PERIOD 1 // shortest period for most results
#define TARGET 150000 // target for domain in KB
#define THRESHOLD 50000 // increments of memory allocation

// Global
hypervisor *h1;

// prototypes
int init_hypervisor(hypervisor *h);
int list_domains(hypervisor *h);
int init_domains(virDomainPtr domain, domain_info *dom, hypervisor *h);
int memory_snapshot(hypervisor *h);
int print_stats(hypervisor *h);
int allocate_memory(hypervisor *h);

static void _sig_handler(int signaled){

    if (signaled == SIGINT || signaled == SIGTERM){

        printf("\nIn the Cleanup function!\n");

        // free all memory
        int i = 0;
        
        for (i = 0; i < h1->active_domains; i++){
        
            free(h1->dom[i].minfo);
        }
    
        free(h1->nparams);
        free(h1->params);
        free(h1->info);
        free(h1->dom);

        // cleanup hypervisor
        virConnectClose(h1->conn);
        free(h1);
        
        // exit
        exit(signaled);
    }
}

int main(int argc, char **argv){


    /*
     *  Parse command line argument
     */

    int sleep_time = 0;

    int i = 0; // counter

    if (argc != 2){
        fprintf(stderr, "ERROR: invalid command line paramters\n");
        exit(1);
    
    } else if (argc == 2){
        const char* sleepy = argv[1];
        sleep_time = atoi(sleepy);

        if (sleep_time <= 0){
            fprintf(stderr, "ERROR: Invalid time interval specified\n");
            exit(1);
        }
    }

    printf("Time interval specified = %d\n", sleep_time);

    /*
     *  Setup signal handlers for cleanup
     */

    if (signal(SIGINT, _sig_handler) == SIG_ERR){
        fprintf(stderr, "Failed to catch SIGINT\n");
        exit(-1);
    }

    if (signal(SIGTERM, _sig_handler) == SIG_ERR){
        fprintf(stderr, "Failed to catch SIGTERM\n");
    }

    /*
     *  Hypervisor initializtion
     */

    h1 = malloc(sizeof(hypervisor));

    if (init_hypervisor(h1) != 0){
        fprintf(stderr, "ERROR: init_hypervisor\n");
        exit(1);
    }

    /*
     *  List all active domains
     */

    if (list_domains(h1) != 0){
        fprintf(stderr, "ERROR: list_domains\n");
        exit(1);
    }

    /*
     *  Domain initialization
     */

    for (i = 0; i < h1->active_domains; i++){

        if (init_domains(h1->domains[i], &h1->dom[i], h1) != 0){
            fprintf(stderr, "ERROR: init_domains\n");
            exit(1);
        }
    }

    /*
     *  Main loop
     */

    printf("right before main loop\n");

    for(;;){

        /*
         *  Memory snapshot
         */

        if (memory_snapshot(h1) != 0){
            fprintf(stderr, "ERROR: memory_snapshot\n");
            exit(1);
        }

        /*
         *  Print information
         */

        if (print_stats(h1) != 0){
            fprintf(stderr, "ERROR: print_stats\n");
            exit(1);
        }

        /*
         *  Make memory allocation decision
         */

        if (allocate_memory(h1) != 0){
            fprintf(stderr, "ERROR: allocate menmory\n");
            exit(1);
        }

        /*
         *  Sleep for the specified amount of time
         */

        sleep(sleep_time);

    }

    return 0;
}

/*
 *  Initializes the hypervisor connection and get node info
 */

int init_hypervisor(hypervisor *h){

    /*
     *  Initializes connection to hypervisor
     */

    h->conn = virConnectOpen("qemu:///system");
    if (h->conn == NULL){
        fprintf(stderr, "ERROR: virConnectOpen\n");
        return -1;
    }

    /*
     *  Get node information
     */

    h->info = malloc(sizeof(virNodeInfo));

    if (0 != virNodeGetInfo(h->conn, h->info)){
        fprintf(stderr, "ERROR: virNodeGetInfo\n");
        return -1;
    }

    /*
     *  Get params for node memory stats
     */

    int cellNum = VIR_NODE_MEMORY_STATS_ALL_CELLS;

    h->nparams = malloc(sizeof(int));
    memset(h->nparams, 0x0, sizeof(int));
  
    if (0 != virNodeGetMemoryStats(h->conn, cellNum, NULL, h->nparams, 0)){
        fprintf(stderr, "ERROR: virNodeGetMemoryStats for nparams\n");
        return -1;
    }

    h->params = calloc(*h->nparams, sizeof(virNodeMemoryStats));

    return 0;
}

/*
 *  List all active domains
 */

int list_domains(hypervisor *h){

    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE;

    h->active_domains = virConnectListAllDomains(h->conn, &(h->domains), flags);

    if (h->active_domains < 0){
        fprintf(stderr, "ERROR: virConnectListAllDomains\n");
        return -1;
    }

    h->dom = malloc(h->active_domains * sizeof(domain_info));

    return 0;
}

/*
 *  Domain initialization
 */

int init_domains(virDomainPtr domain, domain_info *dom, hypervisor *h){

    dom->target = TARGET;

    /*
     *  Set memory stats period
     */

    unsigned int flags = VIR_DOMAIN_AFFECT_LIVE;

    if (virDomainSetMemoryStatsPeriod(domain, MEMORY_STATS_PERIOD, flags) != 0){
        fprintf(stderr, "ERROR: virDomainSetMemoryStatsPeriod\n");
        return -1;
    }

    /*
     *  Get max memory
     */

    dom->max_memory = virDomainGetMaxMemory(domain);

    /*
     *  Allocate memory for stats
     */

    dom->minfo = calloc(VIR_DOMAIN_MEMORY_STAT_NR, sizeof(virDomainMemoryStatStruct));

    /*
     *  Set statistics to zeor
     */

    dom->mem_actual = 0;
    dom->mem_actual_last = 0;
    dom->mem_unused = 0;
    dom->mem_unused_last = 0;
    dom->mem_available = 0;
    dom->mem_available_last = 0;

    return 0;
}

/*
 *  Take a snapshot of domain and node memory information
 */

int memory_snapshot(hypervisor *h){
   
    int i = 0;

    int cellNum = VIR_NODE_MEMORY_STATS_ALL_CELLS;

    /*
     *  snapshot node memory
     */

    if (0 != virNodeGetMemoryStats(h->conn, cellNum, h->params, h->nparams, 0)){
        fprintf(stderr, "ERROR: getting node memory stats\n");
        return -1;
    }

    for (i = 0; i < *h->nparams; i++){

        if (strcmp(h->params[i].field, VIR_NODE_MEMORY_STATS_TOTAL) == 0){
            h->mem_total = h->params[i].value;
        }

        if (strcmp(h->params[i].field, VIR_NODE_MEMORY_STATS_FREE) == 0){
            h->mem_free = h->params[i].value;
        }
        
        if (strcmp(h->params[i].field, VIR_NODE_MEMORY_STATS_BUFFERS) == 0){
            h->mem_buffers = h->params[i].value;
        }
        
        if (strcmp(h->params[i].field, VIR_NODE_MEMORY_STATS_CACHED) == 0){
            h->mem_cached = h->params[i].value;
        }
    } 

    /*
     *  Snapshot each domains memory information
     */

    int j = 0;
    
    for (j = 0; j < h->active_domains; j++){
    
        h->dom[j].mem_actual_last = h->dom[j].mem_actual;
        h->dom[j].mem_unused_last = h->dom[j].mem_unused;
        h->dom[j].mem_available_last = h->dom[j].mem_available;
    }
    
    for (i = 0; i < h->active_domains; i++){

        h->dom[i].stats = virDomainMemoryStats(h->domains[i], h->dom[i].minfo, VIR_DOMAIN_MEMORY_STAT_NR, 0);

        if (h->dom[i].stats < 0){
            fprintf(stderr, "ERROR getting memory stats\n");
            return -1;
        }
    
        for (j = 0; j < h->dom[i].stats; j++){

            if (h->dom[i].minfo[j].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON){
                h->dom[i].mem_actual = h->dom[i].minfo[j].val;
            } else if (h->dom[i].minfo[j].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED){
                h->dom[i].mem_unused = h->dom[i].minfo[j].val;
            } else if (h->dom[i].minfo[j].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE){
                h->dom[i].mem_available = h->dom[i].minfo[j].val;
            }
        }
    }

    return 0;
}

/*
 *  Print node and domain memory stats
 */

int print_stats(hypervisor *h){

    int i = 0;

    /*
     *  Print out header for Domain info
     */

    printf("\n--------------------------------------------------------------------\n");
    printf("> Domain Memory:\n");
    printf("\n%16s%16s%16s%16s","Domain", "Actual (KB)", "Available (KB)", "Unused (KB)");

    for (i = 0; i < h->active_domains; i++){
        printf("\n%16s", virDomainGetName(h->domains[i]));
        printf("%16llu", h->dom[i].mem_actual);
        printf("%16llu", h->dom[i].mem_available);
        printf("%16llu", h->dom[i].mem_unused);
    }

    printf("\n");

    /*
     *  Print out the node stats
     */

    printf("\n> Host Memory:\n");
    printf("%16s%16s\n", " ", "Memory (KB)");
    printf("%16s%16llu\n", "Total:", h->mem_total);
    printf("%16s%16llu\n", "Free:", h->mem_free);
    printf("%16s%16llu\n", "Buffers:", h->mem_buffers);
    printf("%16s%16llu\n", "Cached:", h->mem_cached);

    printf("\n--------------------------------------------------------------------\n");
    
    return 0;
}

/*
 *  Allocate/Deallocate memory based on usage
 */

int allocate_memory(hypervisor *h){

    int i = 0;
    int res = 0;
    unsigned int flags = VIR_DOMAIN_AFFECT_LIVE;

    for (i = 0; i < h->active_domains; i++){

        res = compare(h->dom[i].mem_unused, h->dom[i].target, THRESHOLD);

        /*
         *  Above target, remove memory
         */

        if (res == 1){

            if (virDomainSetMemoryFlags(h->domains[i], h->dom[i].mem_actual - THRESHOLD, flags) != 0){
                fprintf(stderr, "ERROR: virDomainSetMemory\n");
                return -1;
            }
        }

        /*
         *  Above target, remove memory  
         */

        else if (res == 2){
        
            unsigned long long temp;
            temp = h->dom[i].mem_actual - (h->dom[i].mem_unused - h->dom[i].target);

            if (virDomainSetMemoryFlags(h->domains[i], temp, flags) != 0){
                fprintf(stderr, "ERROR: virDomainSetMemory\n");
                return -1;
            }
        } 
        
        /*
         *  Below target, add more memory 
         */
        
        else if (res == 3){

            /*
             *  Make sure we have enough host memory
             */

            if (h->mem_free < (2 * TARGET)){
                fprintf(stderr, "ERROR: not enough host memory\n");
                break;
            }

            /*
             *  Allocate additional memory for domain
             */

            unsigned long long temp;
            temp = h->dom[i].mem_actual + (2 * THRESHOLD);

            if (temp > h->dom[i].max_memory)
                temp = h->dom[i].max_memory;

            if (virDomainSetMemoryFlags(h->domains[i], temp, flags) != 0){
                fprintf(stderr, "ERROR: virDomainSetMemory\n");
                return -1;
            }

        } 

        /*
         *  Below target, add more memory
         */
        
        else if (res == 4){

            /*
             *  Make sure we have enough host memory
             */

            if (h->mem_free < (2 * TARGET)){
                fprintf(stderr, "ERROR: not enough host memory\n");
                break;
            }

            /*
             *  Allocate additional memory for domain
             */

            unsigned long long temp;
            temp = h->dom[i].mem_actual + THRESHOLD;

            if (temp > h->dom[i].max_memory)
                temp = h->dom[i].max_memory;

            if (virDomainSetMemoryFlags(h->domains[i], temp, flags) != 0){
                fprintf(stderr, "ERROR: virDomainSetMemory\n");
                return -1;
            }

        } 
        
        /*
         *  Error case - should not occur in normal operation
         */
        
        else if (res != 0) {
            fprintf(stderr, "ERROR: allocate_memory undefined\n");
            return -1;
        }
    }

    return 0;
}
