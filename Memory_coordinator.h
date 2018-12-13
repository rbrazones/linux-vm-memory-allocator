/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: Memory_coordinator.h
 *
 *  Description: Header file for the memory coordinator project. Contains the struct
 *  definitions for storing node and domian information.
 *
 * ********************************************************************************/

#include <libvirt/libvirt.h>

/*
 *  Struct for storing information about each domain
 */

typedef struct{

    unsigned long long mem_actual;            //  actual reported memory in KB
    unsigned long long mem_actual_last;       //  actual reported memory in KB
    unsigned long long mem_unused;            //  unused reported memory in KB
    unsigned long long mem_unused_last;       //  unused reported memory in KB
    unsigned long long mem_available;         //  available memory reported in KB
    unsigned long long mem_available_last;    //  available memory reported in KB
    unsigned long max_memory;                 //  max amount of physical memory allocated in kibibytes
    unsigned long long target;                //  target available memory for the domain
    virDomainMemoryStatPtr minfo;             //  used for storing memory info per domain
    int stats;                                //  number of memory stats provided

}domain_info;

/*
 *  Struct for storing information about hypervisor
 */

typedef struct{

    virConnectPtr conn;         // connection to hypervisor
    virNodeInfoPtr info;        // storing information about the host node
    virDomainPtr *domains;      // keeping track of all the domains
    int active_domains;         // how many active domains do we have
    domain_info *dom;           // domain info for each of our active domains

    virNodeMemoryStatsPtr params;
    int *nparams;

    unsigned long long mem_total;   // host total mem in KB
    unsigned long long mem_free;    // host free mem in KB
    unsigned long long mem_buffers; // host buffer mem in KB
    unsigned long long mem_cached;  // host cached mem in KB

}hypervisor;
