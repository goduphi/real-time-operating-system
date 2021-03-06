/*
 * kernel.h
 *
 *  Created on: Sep 29, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef INCLUDE_KERNEL_H_
#define INCLUDE_KERNEL_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// semaphore
#define MAX_SEMAPHORES 5
#define MAX_QUEUE_SIZE 5

#define MAX_SEM_NAME                16
#define MAX_SEM_WAIT_QUEUE_SIZE     5

typedef struct _semaphore
{
    uint16_t count;
    uint16_t queueSize;
    uint32_t processQueue[MAX_QUEUE_SIZE]; // store task index here
} semaphore;

// Custom struct for user space semaphore info
struct _semaphoreInformation
{
    char name[MAX_SEM_NAME];
    uint16_t count;
    uint16_t waitingTasksNumber;
    uint32_t waitQueue[MAX_SEM_WAIT_QUEUE_SIZE];
};

#define keyPressed 1
#define keyReleased 2
#define flashReq 3
#define resource 4

// task
#define STATE_INVALID    0 // no task
#define STATE_UNRUN      1 // task has never been run
#define STATE_READY      2 // has run, can resume at any time
#define STATE_DELAYED    3 // has run, but now awaiting timer
#define STATE_BLOCKED    4 // has run, but now blocked by semaphore
#define STATE_KILLED     5

#define MAX_TASKS 12       // maximum number of valid tasks

// REQUIRED: add store and management for the memory used by the thread stacks
//           thread stacks must start on 1 kiB boundaries so mpu can work correctly

struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    int8_t priority;               // 0=highest to 15=lowest
    uint32_t ticks;                // ticks until sleep complete
    uint32_t srd;                  // MPU subregion disable bits
    uint32_t time;                 // Amount of the time the task spent running
    char name[16];                 // name of task used in ps command
    void *semaphore;               // pointer to the semaphore that is blocking the thread
} tcb[MAX_TASKS];

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

#define REGION_0            0x00000000      // Lowest priority
#define REGION_1            0x00000001
#define REGION_2            0x00000002
#define REGION_3            0x00000003
#define REGION_4            0x00000004
#define REGION_5            0x00000005
#define REGION_6            0x00000006      // Highest priority

#define SRAM_REGION_0       0x20000000
#define SRAM_REGION_1       0x20002000
#define SRAM_REGION_2       0x20004000
#define SRAM_REGION_3       0x20006000

#define SIZE_8KIB           0x0000000C      // SIZE = 12 in 2^(SIZE + 1) to obtain a 8KiB size

#define DEBUG

// Set the processor to use the Process Stack Pointer for thread mode
void setPspMode();
// Set the starting address of the process stack. Make sure to pass in
// &Stack[N] instead of &Stack[N - 1]. This is a fully descending stack.
// So, the SP is decrement first to N - 1 then data is written.
void setPsp(uint32_t address);
// Puts the processor into privilege mode
void disablePrivilegeMode();
void enableExceptionHandler(uint32_t type);
void enableBackgroundRegionRule();
void enableFlashRule();
void enableSRAMRule(uint32_t startAddress, uint32_t size, uint32_t region);
void sRAMSubregionDisable(uint8_t region, uint32_t subregion);
void sRAMSubregionEnable(uint8_t region, uint32_t subregion);
void enableMPU();

void initRtos();
int rtosScheduler();
int priorityRtosScheduler();
void setSchedulerMode(schedulerId schedId);
void initTaskNextPriorities();
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
void restartThread(_fn fn);
void destroyThread(_fn fn);
void setThreadPriority(_fn fn, uint8_t priority);
void getIpcsData(struct _semaphoreInformation* si);
void getPsInfo(struct _taskInfo* ti, uint8_t* tiCount);
bool createSemaphore(uint8_t semaphore, uint8_t count);
void startRtos();

#ifdef DEBUG

void infoTcb();

#endif

#endif /* INCLUDE_KERNEL_H_ */
