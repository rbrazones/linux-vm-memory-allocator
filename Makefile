# Ryan Brazones - rbrazones@gmail.com
#
# File: Makefile
# Description: Run "make" to build the memory coordinator project

CC = gcc  
CFLAGS := -Wall -Werror --std=gnu99 -g3  
 
ARCH := $(shell uname) 
ifneq ($(ARCH),Darwin) 
	    LDFLAGS += -lvirt -lm  
endif 

all: Memory_coordinator

Memory_coordinator: Memory_coordinator.c metric.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o Memory_coordinator
