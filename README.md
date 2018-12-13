# Linux VM Memory Scheduler
Tool for allocating memory to virtual machines running in Linux

## About

This project presents a method for allocating host memory to virtual machines running on that system. It will attempt to allocate more memory to VMs as their memory load increases, and deallocate memory from VMs as their load decreases.

## Project Contents

This project contains the following items:

* `README.md` (this file)
* `Makefile`
* `Memory_coordinator` (pre-compiled binary)
* `Memory_coordinator.c`
* `Memory_coordinator.h`
* `metric.c`
* `metric.h`
* `metric.o` (pre-compiled)

## How to Compile

Verify the following packages are installed:

* `qemu-kvm`
* `libvirt-bin`
* `libvirt-dev`
* `virt-manager`
* `qemu-system`
* `uvtool`
* `uvtool-libvirt`

This project also utilizes the math libraries (math.h, linked via -lm)

To compile:  
    `$ make`

To clean:  
    `$ make clean`

To clean and recompile:  
    `$make clean; make`

## How to Run:

Prepare VMs

1. Create the desired amount of VMs - 4 are recommended to start with
2. Verify by typing "virsh list" to view all VMs
3. To run the scheduler, run the following command: `$ ./Memory_coordinator <time interval>`

You must specify 1 command line parameter; an integer to tell the memory coordinator how long 
to wait between triggering (in seconds). 

It is recommended to use the shortest time interval (1 second) so that the memory coordinator 
can react the fastest to changes in domain memory usage activity. 

Once the memory coordinator has being running, you should then see print-outs to console 
giving information on host and domian memory:

(Example of console output shown below)

(A terminal console of width cols=105 is recommended to view this output)

```
--------------------------------------------------------------------
> Domain Memory:
 
          Domain     Actual (KB)  Available (KB)     Unused (KB)
       thirdtest          287308          238568          147568
      secondtest          302988          254248          149468
      fourthtest          287220          238480          147500
       firsttest          301112          252372          148672
 
> Host Memory:
                     Memory (KB)
          Total:        32702388
           Free:        27157040
        Buffers:          117940
         Cached:         3771892
 
--------------------------------------------------------------------
```

## Test System Configuraiton:

This project was developed and testing on the following system configuration:

* Intel (R) Xeon (R) CPU E3-1268L v3 @ 2.30 GHz, 8M LLC (Haswell 2013)
* 2x8 GB of DDR3 @ 1600 MHz (Hynix) 
* OS: Ubuntu 14.04.1 (64-bit) Linux Kernel 3.19.0-25-generic
* BIOS: Hyperthreading disabled, SpeedStep disabled
* Compiler: GCC 4.8.4

## Design Choices and Notes

The memory coordinator will collect host and domain memory usage. Based on the the information,
it will attempt to add/remove memory from domains as needed.

An arbitrary target of 150000 KB of unused for memory is specified (this can be modified by 
changing the "TARGET" macro within Memory_coordinator.c). The memory coordinator will attempt
to allocate memory so that each domain has 150000 KB of unused memory available. Memory is 
assigned in increments of the 50000 KB (modified by changing the "THRESHOLD" macro within
memory_coordinator.c).

The memory coordinator assigns memory more aggressively than it removes it. The though process 
is that assigning memory when it is needed should have a high priority than removing excess
memory from a domain. In this case, when a domain is below its target, 2x (2 times) the 
threshold amount of memory will be attempted to be give to that domain. On the other hand, when
a domain is above its unused memory target, it will give back memory to the host in increments
of 1x (1 times) the threshold amount.

Before allocating any memory (taking it from the host and assigning to a domain), a check is 
performed to make sure that the host has enough memory (in this case, 2x the target amount for
each domain). If this host does not have enough memory, it will not be able to assign any more to 
any domains (though it can still receive memory back from domains that do not need it as the check
is performed after memory would be returned to the host). This puts the host at a high priority
than any domain--if there is not enough memory, it is better for a domain to be starved of memory
instead of the host.

While this check is implemented, I never actually tested it -- my development system had enough
memory that it was never starved.

There is also a check to make sure that no domain can be allocated more memory than its maximum
specifed amount.
