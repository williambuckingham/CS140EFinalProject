---
layout: page
title: Mem-checker.
show_on_index: true
---

### Overview

This is my final project for CS 140E : OS Design and Implementation on r/pi A+. It is a simple memory corruption checker built in c to detect memory corruption in programs running on the r/pi A+. At present, it can identify accessing and writing to non-allocated memory on the heap and also gives a summary of leaked memory that is allocated and then not freed.

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

### How to build

### Possible extensions
