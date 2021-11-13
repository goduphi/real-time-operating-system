/*
 * os.c
 *  Page: 88, Control Register (CONTROL)
 *      - Bit 0 controls whether threads execute in privileged mode or not
 *
 *  Created on: Sep 28, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

// Include headers
#include "tm4c123gh6pm.h"
#include "kernel.h"
#include "uart0.h"
#include "utils.h"
#include "tString.h"
#include "peripheral.h"

#define SRAM_BASE                       0x20000000
#define EXEC_RETURN_THREAD_MODE         0xFFFFFFFD
#define OFFSET_TO_PC_AFTER_FN_CALLED    6           // Total of 8 registers are pushed automatically when function called.
                                                    // Offset by 6 4-byte integers.
#define OFFSET_TO_SVC_INSTRUCTION       2           // This is a 16 bit instruction.

typedef enum _svcNumber
{
    YIELD = 7, SLEEP, WAIT, POST, SCHED, PREEMPT_MODE, REBOOT, PID, KILL, RESUME, IPCS, PS
} svcNumber;

extern void pushR4ToR11Psp();
extern void popR4ToR11Psp();
extern void pushPsp(uint32_t r0);

/*
 * Global Variables
 */

// The first 4KiB will be used by the OS. All other threads will get stack space
// starting at 0x20001400
uint32_t* heap = (uint32_t*)0x20001400;

uint8_t taskCurrent = 0;        // index of last dispatched task
uint8_t taskCount = 0;          // total number of valid tasks
uint32_t taskStartTime = 0;     // Time when a task starts executing
uint32_t taskEndTime = 0;      // Time when a task ends executing

uint32_t cpuUsageTime[MAX_TASKS];
uint16_t systickCount = 0;

semaphore semaphores[MAX_SEMAPHORES];

schedulerId schedulerIdCurrent = PRIORITY;
bool preemption = false;

/*
 * Set's the threads to be run using PSP. By default, threads make use of the MSP,
 * but the kernel should run using MSP and threads should run using the PSP,
 * in an OS environment. This ensures a separation of kernel space and user
 * space.
 */
void setPspMode()
{
    __asm(" MRS R1, CONTROL");  // CONTROL is a core register and is not memory mapped. It
                                // can be called by using its name.
    __asm(" ORR R1, #2");       // Set the ASP to use the PSP in thread mode.
    __asm(" MSR CONTROL, R1");
    __asm(" ISB");              // Page 88, ISB instruction is needed to ensure that all instructions
                                // after the ISB execute using the new stack.
}

void setPsp(uint32_t address)
{
    __asm(" MSR PSP, R0");      // Move the new stack pointer into the process stack
}

/*
 * If bit 0 of the Control Register on page 88 is enabled, no changes can be made to any
 * register by unprivileged software.
 *
 * This Raises the privilege level so that unprivileged software executes in the threads. This in
 * combination with using the PSP for threads allows for separation of kernel space and user
 * space.
 */
void disablePrivilegeMode()
{
    __asm(" MRS R0, CONTROL");
    __asm(" ORR R0, #1");       // Set the TMPL bit to turn of privilege mode for threads.
    __asm(" MSR CONTROL, R0");
}

static uint32_t* getMsp()
{
    __asm(" MRS R0, MSP");
}

static uint32_t* getPsp()
{
    __asm(" MRS R0, PSP");
}

void enableExceptionHandler(uint32_t type)
{
    NVIC_SYS_HND_CTRL_R |= type;
}

// This section is dedicated to MPU initialization
// Rules for accessing memory

#define AP_ACCESS_PRIVILEGE (1 << 24)
#define AP_FULL_ACCESS      (3 << 24)
#define SUBREGION_DISABLE   (1 << 8)
#define R_SIZE(x)           (x << 1)        // This refers to the SIZE variable for Region size = 2^(SIZE + 1)

// This will be the background region that gives access to the entire memory range
// Remove Executable privilege since we do not want everyone to be able to execute bit-banded memory for example
void enableBackgroundRegionRule()
{
    // Region 0 will be used to give RWX access for both privilege and unprivilege
    NVIC_MPU_BASE_R = 0x00000000 | NVIC_MPU_BASE_VALID | REGION_0;
    // Region size = 2^(SIZE + 1)
    // For the entire memory range, the SIZE would equal 0x1F, page 192
    NVIC_MPU_ATTR_R = NVIC_MPU_ATTR_XN | AP_FULL_ACCESS | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE
                    | NVIC_MPU_ATTR_BUFFRABLE | R_SIZE(0x1F) | NVIC_MPU_ATTR_ENABLE;
}

void enableFlashRule()
{
    // Set up the address for the MPU base
    // If I want to cover the entire Flash memory region of 256K,
    // the starting address needs to be 0x00000000

    // We will also use region 0 due to the relative prioritization
    // of the 8 memory regions
    // Base address | Valid Bit | Region Number
    NVIC_MPU_BASE_R = 0x00000000 | NVIC_MPU_BASE_VALID | REGION_1;
    // Region size = 2^(SIZE + 1)
    // For 256K, it will be 2^18, where SIZE = 17
    NVIC_MPU_ATTR_R = AP_FULL_ACCESS | NVIC_MPU_ATTR_CACHEABLE | R_SIZE(17) | NVIC_MPU_ATTR_ENABLE;
}

// This rule takes away RW privilege
// Parameters: Starting address of the region, SIZE parameter of 2^(SIZE + 1), Region number
void enableSRAMRule(uint32_t startAddress, uint32_t size, uint32_t region)
{
    // Base address | Valid Bit | Region Number
    NVIC_MPU_BASE_R = startAddress | NVIC_MPU_BASE_VALID | region;
    // Region size = 2^(SIZE + 1)
    NVIC_MPU_ATTR_R = AP_ACCESS_PRIVILEGE | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | R_SIZE(size) | NVIC_MPU_ATTR_ENABLE;
}

void sRAMSubregionDisable(uint8_t region, uint32_t subregion)
{
    if(region < (REGION_2 & 0xFF) || (subregion > (0xFF << 8)) || (subregion & 0xFF != 0))
        return;
    // Base address | Valid Bit | Region Number
    NVIC_MPU_NUMBER_R = region;
    // Disable the sub-regions
    NVIC_MPU_ATTR_R |= subregion << 8;
}

void sRAMSubregionEnable(uint8_t region, uint32_t subregion)
{
    if(region < (REGION_2 & 0xFF) || (subregion > (0xFF << 8)) || (subregion & 0xFF != 0))
        return;
    // Select the region
    NVIC_MPU_NUMBER_R = region;
    // Enable the sub-regions
    NVIC_MPU_ATTR_R &= ~(subregion << 8);
}

void enableMPU()
{
    // NVIC_MPU_CTRL_PRIVDEFEN - Use this for default memory region
    NVIC_MPU_CTRL_R = NVIC_MPU_CTRL_ENABLE;
}

/*
 * Interrupt Service Routines
 */

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr()
{
    uint8_t i = 0;
    // Sleep implementation
    // The Systick Isr is being called @1KHz
    // So, if a task was delayed it will not get scheduled, decrement the ticks (in ms) until it reaches 0
    // Until ticks reaches 0, the task will be sleeping (not get scheduled). Afterwards, we change it to ready,
    // so that it does get scheduled.
    for(; i < taskCount; i++)
        if(tcb[i].state == STATE_DELAYED)
        {
            if(tcb[i].ticks == 0)
            {
                tcb[i].state = STATE_READY;
                return;
            }
            tcb[i].ticks--;
        }

    // Every second, accumulate the total time of all the tasks run
    if(systickCount == TWO_SECOND_SYSTICK)
    {
        systickCount = 0;
        for(i = 0; i < taskCount; i++)
        {
            if(tcb[i].state != STATE_INVALID)
                cpuUsageTime[i] = tcb[i].time;
            tcb[i].time = 0;
        }
    }
    systickCount++;

    if(preemption)
        // Trigger a PendSV ISR call
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr()
{
    // Extract the SVC #N
    // SVC is a 16-bit instruction. PC is pointing to the next instruction to execute after the SVC instruction.
    // When the ISR is called, 8 registers are automatically push onto the stack.

    // Let's get the PC by adding 6 4-byte registers to PSP as the registers pushed are xPSR, PC, LR, R12, R3 - R0.
    // Subtract 2 bytes from it go back to the previous instruction ran (2 bytes because SVC is a 16 bit instruction).
    // -----SVC #N
    // PC-> BX LR
    // Moving PC back by 2 bytes gets us back to SVC
    // Cast it to a 16-bit integer pointer because SVC is a 16-bit instruction.
    uint32_t* psp = getPsp();
    uint8_t N = *(uint16_t*)(*(psp + OFFSET_TO_PC_AFTER_FN_CALLED) - OFFSET_TO_SVC_INSTRUCTION) & 0xFF;

    switch(N)
    {
    case YIELD:
        // Trigger a PendSV ISR call
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
        break;
    case SLEEP:
        tcb[taskCurrent].ticks = *psp;          // R0 is the last register push automatically by hardware. PSP points to R0.
                                                // This will give us access to the first argument passed in for the sleep function.
        tcb[taskCurrent].state = STATE_DELAYED;
        // Trigger a PendSV ISR call
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
        break;

    // IMPORTANT: *psp represents r0 for both wait and post. It is the semaphore index.

    // As long as the semaphore count is greater than 0, the task will be scheduled,
    // otherwise, we want for a post to occur until the task resumes execution.
    // Store all relevant information about the task semaphore.
    case WAIT:
        if(semaphores[*psp].count > 0)
            semaphores[*psp].count--;
        else
        {
            if(semaphores[*psp].queueSize < MAX_QUEUE_SIZE)
            {
                semaphores[*psp].processQueue[semaphores[*psp].queueSize++] = taskCurrent;
                // Store a pointer to the semaphore the task is waiting on
                tcb[taskCurrent].semaphore = (void*)(semaphores + *psp);
                tcb[taskCurrent].state = STATE_BLOCKED;
                // Trigger a PendSV ISR call
                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            }
        }
        break;
    case POST:
        // Increment the count for the selected semaphore
        semaphores[*psp].count++;
        // If the current count of the semaphore is 1, then it suggests that a task is waiting in
        // the queue to be waken up.
        if(semaphores[*psp].count == 1 && semaphores[*psp].queueSize > 0)
        {
            uint8_t currentTaskInSemaphoreQueue = semaphores[*psp].processQueue[0];
            semaphores[*psp].count--;
            tcb[currentTaskInSemaphoreQueue].state = STATE_READY;
            // Delete the task from the semaphore queue
            uint8_t i = 0;
            for(; i < semaphores[*psp].queueSize - 1; i++)
                semaphores[*psp].processQueue[i] = semaphores[*psp].processQueue[i + 1];
            semaphores[*psp].queueSize--;
        }
        break;
    case SCHED:
        // Function signature: void sched(bool prioOn)
        schedulerIdCurrent = ((bool)*psp) ? PRIORITY : ROUND_ROBIN;
        break;
    case PREEMPT_MODE:
        preemption = ((bool)*psp) ? true : false;
        break;
    case REBOOT:
        NVIC_APINT_R = (0x05FA0000 | NVIC_APINT_SYSRESETREQ);
        break;
    case PID:
        // One important thing to check for are all the pointers by the user. They have to
        // be within the region of the user task.
        {
        uint32_t* pid = (uint32_t*)*(psp);
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
        // The svc isr is not accessible by user space, so an auxiliary restart function
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

void MPUFaultHandler()
{
    putsUart0("\nMPU fault in process N\n");

    uint32_t* msp = getMsp();
    uint32_t* psp = getPsp();

    // Causes Hard Fault
    // GPIO_PORTE_DATA_R |= 2;

    // Print MSP and PSP
    putsUart0("\n MSP = 0x");
    printUint32InHex((uint32_t)msp);
    putcUart0('\n');

    putsUart0(" PSP = 0x");
    printUint32InHex((uint32_t)psp);
    putcUart0('\n');

    putsUart0("mFault Flags = 0x");
    printUint32InHex(NVIC_FAULT_STAT_R & 0xFF);
    putcUart0('\n');

    uint32_t faultAddress = NVIC_MM_ADDR_R;
    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_MMARV)
    {
        putsUart0("Faulted at address 0x");
        printUint32InHex(faultAddress);
        putcUart0('\n');
    }
    else
        putsUart0("Fault address not valid.\n");

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

void PendSVISR()
{
    // Un-commenting the next two lines causes flash4Hz to run slower
    // putsUart0("\nCurrent PID: ");
    // printUint32InHex((uint32_t)tcb[taskCurrent].pid);

    // Check if it's an instruction error or a data error
    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR)
    {
        // Clear instruction access violation
        NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_IERR;
        putsUart0("IERR, called from MPU\n\n");
    }

    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR)
    {
        // Clear data access violation
        NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DERR;
        putsUart0("DERR, called from MPU\n\n");
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

    taskEndTime = TIMER1_TAV_R;

    tcb[taskCurrent].time += taskEndTime - taskStartTime;

    // Reset the timer for calculating a new time interval
    TIMER1_TAV_R = 0xFFFFFFFF;

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

    taskStartTime = TIMER1_TAV_R;

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

void BusFaultHandler()
{
    putsUart0("\nBus fault in process N\n");
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

void UsageFaultHandler()
{
    putsUart0("\nUsage fault in process N\n");

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

void FaultISR()
{
    putsUart0("\nHard fault in process N\n");

    uint32_t* msp = getMsp();
    uint32_t* psp = getPsp();

    // Print MSP and PSP
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

/*
 * RTOS subroutines
 */

// REQUIRED: initialize systick for 1ms system timer
void initRtos()
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }
}

// Schedulers!!!

// REQUIRED: Implement prioritization to 8 levels
int rtosScheduler()
{
    bool ok;
    static uint8_t task = 0xFF;
    ok = false;
    while (!ok)
    {
        task++;
        if (task >= MAX_TASKS)
            task = 0;
        ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
    }
    return task;
}

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
            // Number of SRD bits = stackBytes / 1KiB = ((stackBytes - 1) / 1024) + 1
            uint8_t nSrd = ((stackBytes - 1) / 0x400) + 1;
            // If it is the first task, the base is just the heap
            // Otherwise, get the previous sp and treat that as the base
            // tmp currently stores the base address to use
            uint32_t tmp = (i == 0) ? (uint32_t)heap : (uint32_t)tcb[i - 1].spInit;
            // Base + (Number of 1KiB blocks * 1KiB)
            tcb[i].sp = (void*)(tmp + (nSrd * 0x400));
            tcb[i].spInit = (void*)(tmp + (nSrd * 0x400));
            tcb[i].priority = priority;
            tcb[i].srd = 0;
            // Sets all required SRD bits
            while((tcb[i].srd |= 1) && --nSrd && (tcb[i].srd <<= 1));
            // Put the SRD bits in the correct bit positions
            // Find the offset to the end of the stack space for a thread
            // That is where the SRD bit for that thread begins
            tcb[i].srd <<= ((uint32_t)tcb[i].spInit - stackBytes) / 0x400;
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

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
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
void destroyThread(_fn fn)
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

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
}

// This is very implementation specific
void getIpcsData(struct _semaphoreInformation* si)
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

void getPsInfo(struct _taskInfo* ti, uint8_t* tiCount)
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

bool createSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, setting PSP, ASP bit, and PC
void startRtos()
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

    taskStartTime = TIMER1_TAV_R;

    setPsp((uint32_t)tcb[taskCurrent].sp);
    setPspMode();
    _fn fn = (_fn)tcb[taskCurrent].pid;
    // Mark the task as ready because we are going to run it
    tcb[taskCurrent].state = STATE_READY;
    fn();
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
