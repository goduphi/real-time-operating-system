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
#include "kernel.h"

#define MAX_SEM_INFO_SIZE           5
#define MAX_TASKS_TASK_INFO         12

typedef struct _semaphoreInformation semaphoreInfo;
typedef struct _taskInfo taskInfo;

void yield();
void sleep(uint32_t tick);
void wait(int8_t semaphore);
void post(int8_t semaphore);
void rebootSystem();
// Displays the process (thread) information
void ps(taskInfo* ti, uint8_t* tiCount);
// Displays the inter-process (thread) communication state
void ipcs(semaphoreInfo* semInfo);
// Kills the process (thread) with matching PID
void kill(uint32_t pid);
// Turns priority inheritance on or off
void pi(bool on);
// Turns preemption on or off
void preempt(bool on);
// Selected priority or round-robin scheduling
void sched(bool prioOn);
// Displays the PID of the process (thread)
void pidof(uint32_t* pid, char name[]);
void resume(const char* name);

#endif /* INCLUDE_SYSCALLS_H_ */
