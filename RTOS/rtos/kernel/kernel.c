/*
 *  Created on: Sep 28, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

//*****************************************************************************
//
// Includes
//
//*****************************************************************************
#include <mcuSpecific/common_includes/tm4c123gh6pm.h>
#include <mcuSpecific/common_includes/tm4c123gh6pmMemoryMap.h>
#include <mcuSpecific/nvic/nvic.h>
#include <mcuSpecific/mpu/mpu.h>
#include <mcuSpecific/uart/uart0.h>
#include <rtos/include/tString.h>
#include <rtos/include/utils.h>
#include <rtos/kernel/kernel.h>
#include <mcuSpecific/sysTickTimer/sysTickTimer.h>
#include <mcuSpecific/timer/timer.h>

//*****************************************************************************
//
// Defines
//
//*****************************************************************************
#define EXEC_RETURN_THREAD_MODE             (0xFFFFFFFD)
#define OFFSET_TO_PC_AFTER_ISR_CALLED       (6)             // Total of 8 registers are pushed automatically when function called
                                                            // Offset by 6 4-byte registers to reach PC
#define OFFSET_TO_SVC_INSTRUCTION_FROM_PC   (2)             // 2 bytes since SVC is a 16-bit instruction
#define MAX_TASKS                           (12)            // maximum number of valid tasks
#define TASK_NAME_MAX_LEN                   (16)

// Task States
#define STATE_INVALID                       (0)             // no task
#define STATE_UNRUN                         (1)             // task has never been run
#define STATE_READY                         (2)             // has run, can resume at any time
#define STATE_DELAYED                       (3)             // has run, but now awaiting timer
#define STATE_BLOCKED                       (4)             // has run, but now blocked by semaphore
#define STATE_KILLED                        (5)

// semaphore
#define MAX_SEMAPHORES                      (5)
#define MAX_QUEUE_SIZE                      (5)

//*****************************************************************************
//
// Typedefs
//
//*****************************************************************************
typedef enum _svcNumber
{
    YIELD = 0x07,
    SLEEP,
    WAIT,
    POST,
    SCHED,
    PREEMPT,
    REBOOT,
    PID,
    KILL,
    RESUME,
    IPCS,
    PS
} svcNumber;

typedef struct _semaphore
{
    uint16_t count;
    uint16_t queueSize;
    uint32_t processQueue[MAX_QUEUE_SIZE];                  // store task index here
} semaphore;

//*****************************************************************************
//
// Structures
//
//*****************************************************************************
struct _tcb
{
    void *pid;                     // used to uniquely identify thread
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint32_t stackSize;            // current stack size
    uint32_t ticks;                // ticks until sleep complete
    uint32_t srd;                  // MPU subregion disable bits
    uint32_t time;                 // Amount of the time the task spent running
    void *semaphore;               // pointer to the semaphore that is blocking the thread
    char name[TASK_NAME_MAX_LEN + 1];                 // name of task used in ps command
    uint8_t state;
    int8_t priority;               // 0 = highest to 15 = lowest
} tcb[MAX_TASKS];

//*****************************************************************************
//
// Global variables
//
//*****************************************************************************
/*
 * The first 4kiB + 1kiB is allocated for the kernel + (.data, .bss, etc.).
 * All other threads will get stack space starting at 0x20001400.
 */
uint32_t   *heap                        = (uint32_t *)0x20001400;
uint8_t     taskCurrent                 = 0;                        // index of last dispatched task
uint8_t     taskCount                   = 0;                        // total number of valid tasks
uint16_t    sysTickCount                = 0;
semaphore   semaphores[MAX_SEMAPHORES]  = { 0 };
schedulerId schedulerIdCurrent          = ROUND_ROBIN;
bool        preemptionOn                = false;
uint32_t    taskStartTime               = 0;                        // Time when a task starts executing
uint32_t    taskEndTime                 = 0;                        // Time when a task ends executing
uint32_t    cpuUsageTime[MAX_TASKS]     = { 0 };

//*****************************************************************************
//
// Externs
//
//*****************************************************************************
extern uint32_t *getMsp(void);
extern void setPspMode(void);
extern void setPsp(uint32_t address);
extern uint32_t *getPsp(void);
extern void pushR4ToR11Psp(void);
extern void popR4ToR11Psp(void);
extern void pushPsp(uint32_t value);
extern void disablePrivilegeMode(void);

//*****************************************************************************
//
// Forward declarations
//
//*****************************************************************************
int priorityRtosScheduler(void);

//*****************************************************************************
//
// Inline functions
//
//*****************************************************************************
inline bool isTaskPointerInValidRange(uint32_t ptr)
{
    return false;
}

//*****************************************************************************
//
// Private RTOS routines
//
//*****************************************************************************
// REQUIRED: Implement prioritization to 8 levels
static int rtosScheduler(void)
{
    bool ok = false;
    static uint8_t task = 0xFF;

    while (!ok)
    {
        task++;

        if (task >= MAX_TASKS)
        {
            task = 0;
        }

        ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
    }

    return task;
}

// REQUIRED: modify this function to restart a thread
static void restartThread(_fn fn)
{
    uint8_t i = 0;
    for(; i < taskCount; i++)
        if(tcb[i].pid == fn)
        {
            tcb[i].sp = tcb[i].spInit;
            tcb[i].state = STATE_UNRUN;
            break;
        }
}

// REQUIRED: modify this function to destroy a thread
// REQUIRED: remove any pending semaphore waiting
// NOTE: see notes in class for strategies on whether stack is freed or not
static void destroyThread(_fn fn)
{
    uint8_t i = 0;
    for(; i < taskCount; i++)
        if(tcb[i].pid == fn)
        {
            semaphore* s = (semaphore*)tcb[i].semaphore;
            // Remove information from the semaphore array
            if(tcb[i].semaphore != 0 && tcb[i].state == STATE_BLOCKED)
            {
                uint8_t sCounter = 0;
                for(; sCounter < s->queueSize; sCounter++)
                    if(s->processQueue[sCounter] == i)
                    {
                        // Since I am removing a semaphore, I just named the counter r
                        // Shift all the elements back by 1 index to delete task
                        int8_t r = sCounter;
                        for(; r < s->queueSize - 1; r++)
                        {
                            s->processQueue[r] = s->processQueue[r + 1];
                            s->queueSize--;
                        }
                        s->processQueue[r] = 0;
                        // Decrement the queue size manually as the last element remaining is null
                        if(s->queueSize == 1)
                            s->queueSize = 0;
                    }
            }
            tcb[i].state = STATE_KILLED;
            break;
        }
}

//*****************************************************************************
//
// Public Interrupt Service Routines
//
//*****************************************************************************
void faultIsr(void)
{
    rtosPrintf("\nHard fault in process %u", taskCurrent);

    uint32_t *msp = getMsp();
    uint32_t *psp = getPsp();

    putsUart0("\n MSP = 0x");
    printUint32InHex((uint32_t)msp);
    putcUart0('\n');

    putsUart0(" PSP = 0x");
    printUint32InHex((uint32_t)psp);
    putcUart0('\n');

    putsUart0("Hard Fault Flags = 0x");
    printUint32InHex(NVIC_HFAULT_STAT_R);
    putcUart0('\n');

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

void mpuFaultIsr(void)
{
    putsUart0("\n\nMPU fault in process");
    printfInteger("%-u", 2, taskCurrent);
    putcUart0('\n');

    uint32_t *msp = getMsp();
    uint32_t *psp = getPsp();

    putsUart0("\nMSP = 0x");
    printUint32InHex((uint32_t)msp);
    putcUart0('\n');

    putsUart0("PSP = 0x");
    printUint32InHex((uint32_t)psp);
    putcUart0('\n');

    putsUart0("mFault Flags = 0x");
    printUint32InHex(NVIC_FAULT_STAT_R & 0xFF);
    putcUart0('\n');

    uint32_t faultAddress = NVIC_MM_ADDR_R;

    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_MMARV)
    {
        putsUart0("Faulted = 0x");
        printUint32InHex(faultAddress);
        putcUart0('\n');
    }
    else
    {
        putsUart0("Fault address not valid.\n");
    }

    // Process stack dump
    putsUart0("\nStack Dump\n-----------------\n  R0 = 0x");
    printUint32InHex(*(psp));
    putcUart0('\n');

    putsUart0("  R1 = 0x");
    printUint32InHex(*(psp + 1));
    putcUart0('\n');

    putsUart0("  R2 = 0x");
    printUint32InHex(*(psp + 2));
    putcUart0('\n');

    putsUart0("  R3 = 0x");
    printUint32InHex(*(psp + 3));
    putcUart0('\n');

    putsUart0(" R12 = 0x");
    printUint32InHex(*(psp + 4));
    putcUart0('\n');

    putsUart0("  LR = 0x");
    printUint32InHex(*(psp + 5));
    putcUart0('\n');

    putsUart0("  PC = 0x");
    printUint32InHex(*(psp + 6));
    putcUart0('\n');

    putsUart0("xPSR = 0x");
    printUint32InHex(*(psp + 7));
    putcUart0('\n');

    // Clear MPU fault pending flag, page 173
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;

    // Trigger a PendSV ISR call
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

void busFaultIsr(void)
{
    rtosPrintf("\nBus fault in process %u", taskCurrent);

    uint32_t *psp = getPsp();

    putsUart0("  PC = 0x");
    printUint32InHex(*(psp + 6));
    putcUart0('\n');

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

void usageFaultIsr(void)
{
    rtosPrintf("\nUsage fault in process %u", taskCurrent);

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    // Extract SVC #N
    // SVC is a 16-bit instruction. PC is pointing to the next instruction to execute after the SVC instruction.
    // When the ISR is called, 8 registers are automatically push onto the stack.

    // Let's get the PC by adding 6 4-byte registers to PSP as the registers pushed are xPSR, PC, LR, R12, R3 - R0.
    // Subtract 2 bytes from it go back to the previous instruction ran (2 bytes because SVC is a 16 bit instruction).
    // -----SVC #N
    // PC-> BX LR
    // Moving PC back by 2 bytes gets us back to SVC
    // Cast it to a 16-bit integer pointer because SVC is a 16-bit instruction.
    const uint32_t *psp = getPsp();
    const uint8_t   N   = ((*(uint16_t *)(*(psp + OFFSET_TO_PC_AFTER_ISR_CALLED) - OFFSET_TO_SVC_INSTRUCTION_FROM_PC)) & 0xFF);

    switch(N)
    {
    case YIELD:
        // Schedule and dispatch a new task to run
        nvicTriggerPendSv();
        break;
    case SLEEP:
        tcb[taskCurrent].ticks = *psp;          // R0 is the last register push automatically by hardware. PSP points to R0.
                                                // This will give us access to the first argument passed in for the sleep function.
        tcb[taskCurrent].state = STATE_DELAYED;
        // Schedule and dispatch a new task to run
        nvicTriggerPendSv();
        break;
    // IMPORTANT: 0th argument, r0 is the semaphore index.
    // As long as the semaphore count is greater than 0, the task will be scheduled,
    // otherwise, we want for a post to occur until the task resumes execution.
    // Store all relevant information about the task semaphore.
    case WAIT:
        {
            const uint8_t semaphoreIndex = *psp;

            if (semaphoreIndex < MAX_SEMAPHORES)
            {
                if(semaphores[semaphoreIndex].count > 0)
                {
                    semaphores[semaphoreIndex].count--;
                }
                else
                {
                    if(semaphores[semaphoreIndex].queueSize < MAX_QUEUE_SIZE)
                    {
                        semaphores[semaphoreIndex].processQueue[semaphores[semaphoreIndex].queueSize++] = taskCurrent;
                        // Store a pointer to the semaphore the task is waiting on
                        tcb[taskCurrent].semaphore = ((void *)(&semaphores[semaphoreIndex]));
                        tcb[taskCurrent].state = STATE_BLOCKED;
                        // Schedule and dispatch a new task to run
                        nvicTriggerPendSv();
                    }
                }
            }
            else
            {
                // Generate an error
            }
        }
        break;
    case POST:
        {
            const uint8_t semaphoreIndex = *psp;

            if (semaphoreIndex < MAX_SEMAPHORES)
            {
                // Increment the count for the selected semaphore
                semaphores[semaphoreIndex].count++;
                // If the current count of the semaphore is 1, then it suggests that a task is waiting in
                // the queue to be waken up
                if((semaphores[semaphoreIndex].count == 1) && (semaphores[semaphoreIndex].queueSize > 0))
                {
                    uint8_t currentTaskInSemaphoreQueue = semaphores[semaphoreIndex].processQueue[0];
                    uint8_t i                           = 0;

                    semaphores[semaphoreIndex].count--;
                    tcb[currentTaskInSemaphoreQueue].state = STATE_READY;

                    // Delete the task from the semaphore queue as it will now be scheduled to run
                    for(i = 0; i < semaphores[semaphoreIndex].queueSize - 1; i++)
                    {
                        semaphores[semaphoreIndex].processQueue[i] = semaphores[semaphoreIndex].processQueue[i + 1];
                    }

                    semaphores[semaphoreIndex].queueSize--;
                }
            }
            else
            {
                // Generate an error
            }
        }
        break;
    case SCHED:
        {
            const uint8_t prio = (uint8_t)(*psp);

            if (prio <= 1)
            {
                schedulerIdCurrent = ((prio) ? PRIORITY : ROUND_ROBIN);
            }
            else
            {
                // Generate an error
            }
        }
        break;
    case PREEMPT:
        {
            const uint8_t preempt = (uint8_t)(*psp);

            if (preempt <= 1)
            {
                preemptionOn = preempt;
            }
            else
            {
                // Generate an error
            }
        }
        break;
    case REBOOT:
        nvicSystemResetRequest();
        break;
    case PID:
        // One important thing to check for are all the pointers by the user. They have to
        // be within the region of the user task.
        {
        uint32_t *pid = (uint32_t *)(*psp);
        char* taskName = (char*)*(psp + 1);
        uint8_t p = 0;
        for(; p < taskCount; p++)
            if(stringCompare(tcb[p].name, taskName, 16))
            {
                // Validate pid if name found
                *pid = (uint32_t)tcb[p].pid;
                break;
            }
        }
        break;
    case KILL:
        destroyThread((_fn)*psp);
        break;
    case RESUME:
        // Restarts the thread.
        // This seems redundant as we can easily change the state of the task here. The
        // problem is that we need code on the user side which can also restart threads.
        // The SVC ISR is not accessible by user space, so an auxiliary restart function
        // is required.
        {
        char* taskName = (char*)*psp;
        uint8_t p = 0;
        for(; p < taskCount; p++)
            if(stringCompare(tcb[p].name, taskName, 16) && tcb[p].state == STATE_KILLED)
            {
                restartThread((_fn)tcb[p].pid);
                break;
            }
        // Set some ERRNO value if p == taskCount. This implies that the task was never found
        }
        break;
    case IPCS:
        {
        struct _semaphoreInformation* arg = (struct _semaphoreInformation*)*psp;
        getIpcsData(arg);
        }
        break;
    case PS:
        {
        struct _taskInfo* arg = (struct _taskInfo*)*psp;
        uint8_t* tiCountPtr = (uint8_t*)*(psp + 1);
        getPsInfo(arg, tiCountPtr);
        }
        break;
    }
}

void pendSvIsr(void)
{
    // Un-commenting the next two lines causes flash4Hz to run slower
    // putsUart0("\nCurrent PID: ");
    // printUint32InHex((uint32_t)tcb[taskCurrent].pid);

    // Check if it's an instruction error or a data error
    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR)
    {
        tcb[taskCurrent].state = STATE_KILLED;
        // Clear instruction access violation
        NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_IERR;
        putsUart0("IERR, called from MPU\n\n");
    }

    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR)
    {
        tcb[taskCurrent].state = STATE_KILLED;
        // Clear data access violation
        NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DERR;
        putsUart0("\nDERR, called from MPU\n\n");
    }

    // Save the context of the current task
    // Push R4 - R11
    // When a function gets called, it's understood that the function is free
    // to use R0 - R3, the flags register, etc.
    /*
    uint8_t i = 0;
    uint32_t* psp = getPsp();
    uint32_t* p = (uint32_t*)((uint32_t)psp - 0x20);
    for(i = 0; i < 8; i++)
    {
        printUint32InHex(*(p + i));
        putcUart0('\n');
    }
    */

    pushR4ToR11Psp();

    // Save the PSP into the tcb
    tcb[taskCurrent].sp = (void*)getPsp();

    taskEndTime = gptGetValue(_16_32_BIT_TIMER1_BASE, GPT_TIMERA);

    tcb[taskCurrent].time += taskEndTime - taskStartTime;

    // Get a new task to run
    switch(schedulerIdCurrent)
    {
    case ROUND_ROBIN:
        taskCurrent = (uint8_t)rtosScheduler();
        break;
    case PRIORITY:
        taskCurrent = (uint8_t)priorityRtosScheduler();
        break;
    }

    mpuSetSrdBits(tcb[taskCurrent].srd);
    // Reset the timer for calculating a new time interval
    gptSetValue(_16_32_BIT_TIMER1_BASE, GPT_TIMERA, TIMER_MAX_VALUE);

    taskStartTime = gptGetValue(_16_32_BIT_TIMER1_BASE, GPT_TIMERA);

    if(tcb[taskCurrent].state == STATE_READY)
    {
        // Put into PSP the task current to switch task
        setPsp((uint32_t)tcb[taskCurrent].sp);

        // Pop R4 - R11
        popR4ToR11Psp();
    }
    // Initially, the program is unrun. So, we want to make sure that we change the
    // state to ready, and jump to that routine. So, manually load in the value for
    // SP, PC, xPSR, and LR.
    else
    {
        tcb[taskCurrent].state = STATE_READY;
        // Put into PSP the task current to switch task
        // The values we push now will be pushed onto this stack
        setPsp((uint32_t)tcb[taskCurrent].spInit);
        // Push the values of xPSR - R0 on the stack because hardware will pop it automatically
        // We want to push the values corresponding to the task we want to run
        // The value loaded into xPSR is arbitrary for now.
        // Following the data sheet, bit 24 is the Thumb state bit and should always be set.
        pushPsp(0x61000000);                        // xPSR
        // We want the new task to run. So, load in the address of where the task is in memory.
        pushPsp((uint32_t)tcb[taskCurrent].pid);    // PC
        // This value is very important
        // The exception mechanism relies on the value loaded into LR
        // to know when the processor has completed the exception handler.
        pushPsp(EXEC_RETURN_THREAD_MODE);           // LR

        // Initial values of R0-R3, R12 can be initialized to 0 for my implementation
        pushPsp(0x00);                              // R12
        pushPsp(0x00);                              // R3
        pushPsp(0x00);                              // R2
        pushPsp(0x00);                              // R1
        pushPsp(0x00);                              // R0
    }
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void sysTickIsr(void)
{
    // Sleep implementation
    // --------------------
    // The SysTick ISR is being called @1KHz.
    // So, if a task was delayed, it will not get scheduled. Decrement the tick (in ms) until it reaches 0.
    // Until ticks reaches 0, the task will be sleeping (not get scheduled). Afterwards, we change it to
    // ready, so that it does get scheduled.

    uint8_t i = 0;

    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].state == STATE_DELAYED)
        {
            if(tcb[i].ticks == 0)
            {
                tcb[i].state = STATE_READY;
            }
            else
            {
                tcb[i].ticks--;
            }
        }
    }

    // Every two seconds, accumulate the total time of all the tasks run
    if(sysTickCount == SYSTICK_TWO_SECONDS)
    {
        sysTickCount = 0;

        for(i = 0; i < taskCount; i++)
        {
            if(tcb[i].state != STATE_INVALID)
            {
                cpuUsageTime[i] = tcb[i].time;
            }

            tcb[i].time = 0;
        }
    }

    sysTickCount++;

    if(preemptionOn)
    {
        // Schedule and dispatch a new task to run
        nvicTriggerPendSv();
    }
}

//*****************************************************************************
//
// Public RTOS Routines
//
//*****************************************************************************
/*
 * The priority scheduler will use the concept of levels to find out which task to schedule the task.
 * The idea is to create a table with all the task indices with decreasing priorities.
 */

int8_t priNextTask[MAX_TASKS];
uint8_t level = 0;

void initTaskNextPriorities()
{
    for(level = 0; level < MAX_TASKS; level++)
        priNextTask[level] = -1;
    level = 0;
    int8_t priority = 0;
    uint8_t j = 0;
    // The max priority level is 7
    for(priority = 0; priority <= 7; priority++)
        for(j = 0; j < taskCount && j < MAX_TASKS; j++)
            if(tcb[j].priority == priority)
                priNextTask[level++] = j;
    level = 0;
}

int priorityRtosScheduler()
{
    bool ok = false;
    while(!ok)
    {
        if(level >= taskCount)
            level = 0;
        ok = (tcb[priNextTask[level]].state == STATE_READY || tcb[priNextTask[level]].state == STATE_UNRUN);
        level++;
    }
    return priNextTask[level - 1];
}

void initRtos(void)
{
    // Enable all the required exception handlers before initializing everything other parts of the system
    // This will ensure that faults are caught even during initialization
    nvicEnableExceptionHandlers(NVIC_SYS_HND_CTRL_USAGE | NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_MEM);

    uint8_t i = 0;

    // No tasks running
    taskCount = 0;

    // Clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    sysTickTimerInit(SYSTICK_LOAD_VALUE_1KHZ);
    // Initialize this timer to accumulate time for processing cpu time usage calculation
    gptInit(_16_32_BIT_TIMER1_BASE, GPT_TIMERA, TIMER_CFG_32_BIT_TIMER, TIMER_TAMR_TAMR_PERIOD, TIMER_TAMR_TACDIR);

    mpuEnableBackgroundRegionRule();
    mpuEnableFlashRule();
    mpuEnableSramRule(SRAM_REGION_0_START, MPU_REGION_SIZE_8KIB, MPU_REGION_2);
    mpuEnableSramRule(SRAM_REGION_1_START, MPU_REGION_SIZE_8KIB, MPU_REGION_3);
    mpuEnableSramRule(SRAM_REGION_2_START, MPU_REGION_SIZE_8KIB, MPU_REGION_4);
    mpuEnableSramRule(SRAM_REGION_3_START, MPU_REGION_SIZE_8KIB, MPU_REGION_5);
    mpuEnable();
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, setting PSP, ASP bit, and PC
void startRtos(void)
{
    // Get a new task to run
    switch(schedulerIdCurrent)
    {
    case ROUND_ROBIN:
        taskCurrent = (uint8_t)rtosScheduler();
        break;
    case PRIORITY:
        taskCurrent = (uint8_t)priorityRtosScheduler();
        break;
    }

    taskStartTime = gptGetValue(_16_32_BIT_TIMER1_BASE, GPT_TIMERA);

    setPsp((uint32_t)tcb[taskCurrent].sp);
    setPspMode();
    _fn fn = (_fn)tcb[taskCurrent].pid;
    // Mark the task as ready because we are going to run it
    tcb[taskCurrent].state = STATE_READY;
    mpuSetSrdBits(tcb[taskCurrent].srd);
    // Preemption should be turned on by default
    preemptionOn = true;
    disablePrivilegeMode();
    fn();
}

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    // REQUIRED:
    // store the thread name
    // allocate stack space and store top of stack in sp and spInit
    // add task if room in task list
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid == fn);
        }

        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) { i++; }
            tcb[i].state = STATE_UNRUN;
            tcb[i].pid = fn;
            // During creation, the current stack pointer == the initial stack pointer
            // We need to know how many 1KiB blocks we need. We can get it from the number of SRD bits.
            // Number of SRD bits = stackBytes / 1kiB = ((stackBytes - 1) / 1024) + 1
            uint8_t nSrd = ((stackBytes - 1) / 0x400) + 1;
            // If it is the first task, the base is just the heap
            // Otherwise, get the previous sp and treat that as the base
            // tmp currently stores the base address to use
            uint32_t tmp = ((i == 0) ? (uint32_t)heap : (uint32_t)tcb[i - 1].spInit);
            // Base + (Number of 1kiB blocks * 1KiB)
            tcb[i].sp = (void *)(tmp + (nSrd * 0x400) - 1);
            tcb[i].spInit = (void *)(tmp + (nSrd * 0x400) - 1);
            tcb[i].stackSize = roundUp(stackBytes, 0x400);
            tcb[i].priority = priority;
            tcb[i].srd = 0;
            // Sets all required SRD bits
            while((tcb[i].srd |= 1) && --nSrd && (tcb[i].srd <<= 1));
            // Put the SRD bits in the correct bit positions
            // Find the offset to the end of the stack space for a thread
            // That is where the SRD bit for that thread begins
            // Explanation because Happy made me realize my mistake:
            // I always want to calculate the position of the SRD bits relative to the start of SRAM
            // Figure out how many 1kiB regions are required to get the start of the SRD region for a task
            tcb[i].srd <<= (((uint32_t)tcb[i].spInit - SRAM_BASE - tcb[i].stackSize) / 0x400) + 1;
            tcb[i].time = 0;
            stringCopy(name, tcb[i].name, 16);
            tcb[i].semaphore = 0;
            // increment task count
            taskCount++;
            ok = true;
        }
    }
    // REQUIRED: allow tasks switches again
    return ok;
}

bool createSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// Fix me: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
}

// This is very implementation specific
void getIpcsData(struct _semaphoreInformation *si)
{
    uint8_t i = 0;
    for(; i < MAX_SEMAPHORES; i++)
    {
        si[i].count = semaphores[i].count;
        si[i].waitingTasksNumber = semaphores[i].queueSize;
        uint8_t j = 0;
        for(; j < semaphores[i].queueSize; j++)
            si[i].waitQueue[j] = semaphores[i].processQueue[j];
    }

    // This does not need to be copied over every time. This should be changed so that
    // the names are persistent instead of being copied over and over again.
    stringCopy("null", si[0].name, 16);
    stringCopy("keyPressed", si[1].name, 16);
    stringCopy("keyReleased", si[2].name, 16);
    stringCopy("flashReq", si[3].name, 16);
    stringCopy("resource", si[4].name, 16);
}

void getPsInfo(struct _taskInfo *ti, uint8_t *tiCount)
{
    uint8_t i = 0;
    for(; i < taskCount; i++)
    {
        stringCopy(tcb[i].name, ti[i].name, 16);
        ti[i].pid = (uint32_t)tcb[i].pid;
        ti[i].state = tcb[i].state;
        ti[i].time = cpuUsageTime[i];
    }
    *tiCount = taskCount;
}

#ifdef DEBUG

void infoTcb()
{
    if(taskCount == 0)
    {
        putsUart0("No tasks to print\n");
        return;
    }

    uint8_t i = 0;
    for(; i < taskCount; i++)
    {
        putsUart0("\nProcess ");
        putsUart0(tcb[i].name);
        putsUart0(" is ");
        switch(tcb[i].state)
        {
        case STATE_INVALID:
            putsUart0("INVALID\n");
            break;
        case STATE_UNRUN:
            putsUart0("UNRUN\n");
            break;
        case STATE_READY:
            putsUart0("READY\n");
            break;
        }

        putsUart0("PID:        ");
        printUint32InHex((uint32_t)tcb[i].pid);
        putcUart0('\n');
        putsUart0("SP Initial: ");
        printUint32InHex((uint32_t)tcb[i].spInit);
        putcUart0('\n');
        putsUart0("SP:         ");
        printUint32InHex((uint32_t)tcb[i].sp);
        putcUart0('\n');
        putsUart0("Priority:   ");
        printUint8InDecimal(tcb[i].priority);
        putcUart0('\n');
        putsUart0("SRD:        ");
        printUint32InBinary(tcb[i].srd);
        putcUart0('\n');
    }
}

#endif
