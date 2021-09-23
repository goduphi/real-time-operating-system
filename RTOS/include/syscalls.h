/*
 * syscalls.h
 *
 *  Created on: Sep 22, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef INCLUDE_SYSCALLS_H_
#define INCLUDE_SYSCALLS_H_

#include <stdint.h>
#include <stdbool.h>

// To be removed later
void initLed();

void rebootSystem();

// Displays the process (thread) information
void ps();

// Displays the inter-process (thread) communication state
void ipcs();

// Kills the process (thread) with matching PID
void kill(int32_t pid);

// Turns priority inheritance on or off
void pi(bool on);

// Turns preemption on or off
void preempt(bool on);

// Selected priority or round-robin scheduling
void sched(bool prioOn);

// Displays the PID of the process (thread)
void pidof(char name[]);

#endif /* INCLUDE_SYSCALLS_H_ */
