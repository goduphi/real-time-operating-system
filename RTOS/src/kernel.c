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

#define SRAM_BASE   0x20000000

extern void pushR4ToR11Psp();
extern void popR4ToR11Psp();
extern void pushPsp(uint32_t r0);

/*
 * Global Variables
 */

// The first 4KiB will be used by the OS. All other threads will get stack space
// starting at 0x20001400
uint32_t* heap = (uint32_t*)0x20001400;

uint8_t taskCurrent = 0;   // index of last dispatched task
uint8_t taskCount = 0;     // total number of valid tasks

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
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr()
{
    // Trigger a PendSV ISR call
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
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

void printR4ToR11()
{

}

void PendSVISR()
{
    putsUart0("\nPendSVIsr in process N\n");

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
    // Get a new task to run
    taskCurrent = (uint8_t)rtosScheduler();
    if(tcb[taskCurrent].state == STATE_READY)
    {
        // Put into PSP the task current to switch task
        setPsp((uint32_t)tcb[taskCurrent].sp);

        // Pop R4 - R11
        popR4ToR11Psp();
    }
    // The current state of the task is unrun
    else
    {
        // If the task is Unrun, change the state to ready
        tcb[taskCurrent].state = STATE_READY;
        // Put into PSP the task current to switch task
        setPsp((uint32_t)tcb[taskCurrent].spInit);
        // Push the values of xPSR - R0 on the stack because hardware will pop it automatically
        // We want to push the values corresponding to the task we want to run
        pushPsp(0x61000000);                        // xPSR
        pushPsp((uint32_t)tcb[taskCurrent].pid);    // PC
        // This value is very important
        // INSERT COMMENT ON HOW LR works. Section 2.5
        pushPsp(0xFFFFFFFD);                        // LR
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
            // Save the number of SRD bits
            tmp = nSrd;
            tcb[i].priority = priority;
            tcb[i].srd = 0;
            // Sets all required SRD bits
            while((tcb[i].srd |= 1) && --nSrd && (tcb[i].srd <<= 1));
            // Put the SRD bits in the correct bit positions
            // Find the offset to the end of the stack space for a thread
            // That is where the SRD bit for that thread begins
            tcb[i].srd <<= ((uint32_t)tcb[i].spInit - stackBytes) / 0x400;
            stringCopy(name, tcb[i].name);
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
}

// REQUIRED: modify this function to destroy a thread
// REQUIRED: remove any pending semaphore waiting
// NOTE: see notes in class for strategies on whether stack is freed or not
void destroyThread(_fn fn)
{
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
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
    taskCurrent = (uint8_t)rtosScheduler();
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
