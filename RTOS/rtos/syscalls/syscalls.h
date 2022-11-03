#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <rtos/kernel/kernel.h>

#define MAX_SEM_INFO_SIZE           (5)
#define MAX_TASKS_TASK_INFO         (12)

typedef struct _semaphoreInformation semaphoreInfo;
typedef struct _taskInfo taskInfo;

void yield(void);
void sleep(uint32_t tick);
void wait(int8_t semaphore);
void post(int8_t semaphore);
void pi(bool on);
void sched(bool prioOn);
void preempt(bool on);
void rebootSystem(void);
void pidof(uint32_t *pid, const char name[]);
void kill(uint32_t pid);
void resume(const char name[]);
void ipcs(semaphoreInfo *semInfo);
void ps(taskInfo *ti, uint8_t *tiCount);
