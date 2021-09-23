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

// To be removed later
void initLed()
{
    enablePort(PORTF);
    selectPinPushPullOutput(PORTF, 1);
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
