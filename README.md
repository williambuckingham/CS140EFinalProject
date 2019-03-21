---
layout: page
title: Mem-checker.
show_on_index: true
---

### Overview

This is my final project for CS 140E : OS Design and Implementation on r/pi A+. It is a simple memory corruption checker built in c to detect memory corruption in programs running on the r/pi A+. At present, it can identify accessing and writing to non-allocated memory on the heap and also gives a summary of leaked memory that is allocated and then not freed. The timer interrupt code in timer-int is substantially taken from code written by Dawson Engler for CS 140E.<br />

### Files

**mem-checker**<br />
<br />
*mem-checker.h*<br />
Defines structs used in mem-checker.c including useful structs for decoding ARM load/store instructions and a struct           used to record memory corruptions. <br />
<br />
*mem-checker.c*<br />
This is where the bulk of the code for the memory checking tool lives.<br />
<br />
*Q.h*<br />
Defines a queue structure. Code taken from CS140E Lab 9 : Threads by Dawson Engler. <br />
<br />
*Makefile*<br />
Make file to run program.<br />
<br />
<br />
<br />
**timer-int**<br />
<br />
*cstart.c*<br />
Routine to initialize the c-runtime. Zero out the bss. Code taken from CS140E Lab 7 : gprof by Dawson Engler.<br />
<br />
*interrupts-asm.s*<br />
Assembly code for enabling interrupts and setting up interrupt handling. Code taken from CS140E Lab 7 : gprof by Dawson Engler.<br />
<br />
*interrupts-c.c*<br />
C code for setting up interrupt handling. Code taken from CS140E Lab 7 : gprof by Dawson Engler.<br />
<br />
*Makefile*<br />
Make file. Code taken from CS140E Lab 7 : gprof by Dawson Engler<br />
<br />
*rpi-armtimer.h*<br />
Defines for rpi ARM timer. Code taken from CS140E Lab 7 : gprof by Dawson Engler<br />
<br />
*rpi-interrupts.h*<br />
Useful defines, structs and function headers used in interrupt handling code. Code taken from CS140E Lab 7 : gprof by Dawson Engler.<br />
<br />
*start.s*<br />
Routine to set up stack pointer and branch to c-runtime setup. Code taken from CS140E Lab 7 : gprof by Dawson Engler<br />
<br />
*timer-interrupt.c/timer-interrupt.h*<br />
Code for intializing timer interrupts. Code taken from CS140E Lab 7 : gprof by Dawson Engler.<br />
<br />
*timer.c* <br />
Program to demonstrate timer interrupts on the raspberry pi. Code taken from CS140E Lab 7 : gprof by Dawson Engler. <br />
<br />


### How to run/compile

The code will compile on mac os with gcc hooked up to a raspberry pi A+. I have not tried running this code on other systems. Once your r/pi is hooked up, you can run "make" and this will compile the code and also start program execution.<br />

### How to build

1. Set up Timer Interrupts<br />
2. Catch load/store instructions in int handler<br />
3. Parse instruction for memory address<br />
4. Set up heap allocator and shadow memory<br />
5. Determine if memory accesses legal<br />
6. Determine how many bytes of memory lost<br />
7. Log results to the console<br />

### Future extensions and ideas

1. Process more ARM instructions than just load/store immediate offset. The structs for parsing register offset and load/store multiples are already defined in mem-checker.h, you would just need to extend the code in mem-checker.c to also parse these other kinds of load store instructions to get the memory address being accessed. 

2. Add backtrace : print out not just the program counter of where the memory corruption occures but use this and the symbol table to figure out the function in which the memory corruption occurred and print this out too.

3. More sophisticated memory loss tracking. In the code that checks if all memory was freed, actually trace back from shadow memory to real memory to figure out what areas of memory were lost, how many contiguous chunks/start address/maybe even where they were accessed, in terms of pc/function name.

4. Add more sophisticated tracking of heap allocated memory, in terms of the scope of that memory. Right now I assume that any allocated memory is a legal access but this makes tracking down illegal accesses from out of scope impossible. I would need to track more data in kmalloc about who/where it was allocated, to figure out if a memory access to heap memory was in scope.

