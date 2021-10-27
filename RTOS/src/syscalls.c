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
    __asm(" SVC  #1");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
}

// To be removed later
void initLedPb()
{
    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    // PB2 is connected to the anode of the LED's
    selectPinPushPullOutput(PORTB, 2);
    setPinValue(PORTB, 2, 1);

    selectPinPushPullOutput(PORTA, 2);
    selectPinPushPullOutput(PORTA, 3);
    selectPinPushPullOutput(PORTA, 4);
    selectPinPushPullOutput(PORTE, 0);
    selectPinPushPullOutput(PORTF, 2);

    setPinCommitControl(PORTD, 7);

    // Configure all the push buttons pins to be inputs
    selectPinDigitalInput(PORTC, 4);
    selectPinDigitalInput(PORTC, 5);
    selectPinDigitalInput(PORTC, 6);
    selectPinDigitalInput(PORTC, 7);
    selectPinDigitalInput(PORTD, 6);
    selectPinDigitalInput(PORTD, 7);

    // Configure pull-ups for all the push buttons
    enablePinPullup(PORTC, 4);
    enablePinPullup(PORTC, 5);
    enablePinPullup(PORTC, 6);
    enablePinPullup(PORTC, 7);
    enablePinPullup(PORTD, 6);
    enablePinPullup(PORTD, 7);
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

void rebootSystem()
{
    NVIC_APINT_R = (0x05FA0000 | NVIC_APINT_SYSRESETREQ);
}

// Displays the process (thread) information
void ps()
{
    putsUart0("PS called\n");
}

// Displays the inter-process (thread) communication state
void ipcs()
{
    putsUart0("IPCS called\n");
}

// Kills the process (thread) with matching PID
void kill(int32_t pid)
{
    char buffer[16];
    sprintf(buffer, "pid %d killed\n", pid);
    putsUart0(buffer);
}

// Turns priority inheritance on or off
void pi(bool on)
{
    char buffer[16];
    sprintf(buffer, "PI %s\n", (on) ? "on" : "off");
    putsUart0(buffer);
}

// Turns preemption on or off
void preempt(bool on)
{
    char buffer[16];
    sprintf(buffer, "preempt %s\n", (on) ? "on" : "off");
    putsUart0(buffer);
}

// Selected priority or round-robin scheduling
void sched(bool prioOn)
{
    char buffer[16];
    sprintf(buffer, "sched %s\n", (prioOn) ? "prio" : "rr");
    putsUart0(buffer);
}

// Displays the PID of the process (thread)
void pidof(char name[])
{
    char buffer[32];
    sprintf(buffer, "%s launched\n", name);
    putsUart0(buffer);
}
