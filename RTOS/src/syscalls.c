/*
 * syscalls.c
 *
 *  Created on: Sep 22, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */


#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"
#include "syscalls.h"

#include <stdio.h>  // This will be removed from the future version as it takes 4096 Bytes of stack space to call

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield()
{
    __asm(" SVC  #7");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm(" SVC  #8");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm(" SVC  #9");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm(" SVC  #10");
}

uint8_t readPbs()
{
    uint8_t sum = 0;
    if(!getPinValue(PORTC, 4))
        sum += 1;
    if(!getPinValue(PORTC, 5))
        sum += 2;
    if(!getPinValue(PORTC, 6))
        sum += 4;
    if(!getPinValue(PORTC, 7))
        sum += 8;
    if(!getPinValue(PORTD, 6))
        sum += 16;
    if(!getPinValue(PORTD, 7))
        sum += 32;
    return sum;
}

// Turns priority inheritance on or off
void pi(bool on)
{
    char buffer[16];
    sprintf(buffer, "PI %s\n", (on) ? "on" : "off");
    putsUart0(buffer);
}

// Selected priority or round-robin scheduling
void sched(bool prioOn)
{
    __asm(" SVC #11");
}

// Turns preemption on or off
void preempt(bool on)
{
    __asm(" SVC #12");
}

void rebootSystem()
{
    __asm(" SVC #13");
}

// Displays the PID of the process (thread)
void pidof(char name[])
{
    __asm(" SVC #14");
}

// Kills the process (thread) with matching PID
void kill(int32_t pid)
{
    __asm(" SVC #15");
}
// Insert proc_name &

// Displays the inter-process (thread) communication state
void ipcs()
{
    __asm(" SVC #16");
}

// Displays the process (thread) information
void ps()
{
    __asm(" SVC #17");
}
