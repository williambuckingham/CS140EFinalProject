---
layout: page
title: Mem-checker.
show_on_index: true
---
IN PROGRESS
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

### Explanation

**1. Set up Timer Interrupts**<br />
Consult the documentation in CS140E lab7 to see how the timer interrupt code was written (https://github.com/dddrrreee/cs140e-win19/tree/master/labs/lab7-interrupts).<br />

**2. Catch load/store instructions in int handler**<br />
In interrupt handling code, pass old pc into interrupt handler c code. Then you can look at bits 25:27 to check if it is a load or store. <br />

**3. Parse instruction for memory address**<br />
This is one of the more involved and trickier asspects of the code. I define a number of structs in mem-checker.h that help with parsing the actual 32 bit ARM instruction. If you want to write your own structs or just not use mine then you should look in the ARM v6 reference manual (http://cs107e.github.io/readings/armv6.pdf) at chapter A3 where the various instruction encodings are defined. I use bitfields to represent the encodings as this is significantly simpler than using bit masks as the instruction encodings are fairly complicated. It is important to fully understand what each of the bits means when parsing an instruction as a simple load/store can in fact take on many different forms. Importantly, the P bit defines wether the offset should be added/subtracted before or after the memory access. The U bit defines wether the offset should be added or subtracted. The W bit defines wether the instruction writes back to the base register. The L tells you if it is a load or is a store. The B bit tells you if it is a ldrb/strb as opposed to a ldrw/strw. <br />

**4. Set up heap allocator and shadow memory**<br />
I set up a shadow memory at a 4 MB offset from the main heap. This offset is somewhat arbitrary but also the test cases I run do not allocate more than 4 MB to the heap so this should be fine. Inside my kmalloc function I then mark allocated memory at address 0xabcd by writing a magic number for num allocated bytes to 0xabcd + 4MB. It is important to zero initialize this shadow memory as otherwise you may have garbage (including magic numbers!) written to this chunk of your memory that doesn't get cleaned up and will mess with your results on multiple runs of the program. In my kfree function, I simply rezero the relevant data in shadow memory that was previously set to magic number, to mark it as now free.<br />

**5. Determine if memory accesses legal**<br />
This is perhaps the most interesting part and offers the most opportunities to flesh out a useful and inciteful memory checker. I currently just check if the memory access was to a malloced area by looking in the shadow memory but you could do something more sophisticated and keep some kind of data structure that knows where different alloc'd parts of memory are in scope and not in scope. My current approach thinks of heap data as global and so any access is legal, of course a more advance memory checker would be able to see where there were accesses to out of scope memory.<br />

**6. Determine how many bytes of memory lost**<br />
To determine if any bytes were lost/not freed, you can simply loop over the shadow memory and count the number of bytes still set to the magic number, which indicates they were alloced but were never freed and then log this.<br />

**7. Log results to the console**<br />
I define a struct that holds data about each memory corruption and then a global queue of memory corruption data structures which I append new found corruptions to. At the end then I can just log this data out. I currently store the PC of the instruction making a corruption, the bad memory address being accessed and whether it was a read or write. It could also be cool to add a function name field to this struct which would allow you to then print out the function name of the offending instruction.<br />

### Future extensions and ideas

1. Process more ARM instructions than just load/store immediate offset. The structs for parsing register offset and load/store multiples are already defined in mem-checker.h, you would just need to extend the code in mem-checker.c to also parse these other kinds of load store instructions to get the memory address being accessed. 

2. Add backtrace : print out not just the program counter of where the memory corruption occures but use this and the symbol table to figure out the function in which the memory corruption occurred and print this out too.

3. More sophisticated memory loss tracking. In the code that checks if all memory was freed, actually trace back from shadow memory to real memory to figure out what areas of memory were lost, how many contiguous chunks/start address/maybe even where they were accessed, in terms of pc/function name.

4. Add more sophisticated tracking of heap allocated memory, in terms of the scope of that memory. Right now I assume that any allocated memory is a legal access but this makes tracking down illegal accesses from out of scope impossible. I would need to track more data in kmalloc about who/where it was allocated, to figure out if a memory access to heap memory was in scope.

