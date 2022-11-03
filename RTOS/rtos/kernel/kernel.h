#pragma once

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------
// function pointer
typedef void (*_fn)(void);

#define MAX_SEM_NAME                16
#define MAX_SEM_WAIT_QUEUE_SIZE     5

// Custom struct for user space semaphore info
struct _semaphoreInformation
{
    char name[MAX_SEM_NAME];
    uint16_t count;
    uint16_t waitingTasksNumber;
    uint32_t waitQueue[MAX_SEM_WAIT_QUEUE_SIZE];
};

// User space struct to store pid info
struct _taskInfo
{
    uint8_t state;                 // see STATE_ values above
    uint32_t pid;                  // used to uniquely identify thread
    char name[16];                 // name of task used in ps command
    uint32_t time;                 // CPU usage time
};

// Scheduler
typedef enum _schedulerId
{
    ROUND_ROBIN, PRIORITY
} schedulerId;

void initRtos();
void startRtos();
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
bool createSemaphore(uint8_t semaphore, uint8_t count);

void getIpcsData(struct _semaphoreInformation* si);
void getPsInfo(struct _taskInfo* ti, uint8_t* tiCount);
void initTaskNextPriorities();

#ifdef DEBUG

void infoTcb();

#endif

