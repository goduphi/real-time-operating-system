#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "kernel.h"
#include "syscalls.h"
#include "cli.h"
#include "uart0.h"
#include "wait.h"
#include "peripheral.h"

// REQUIRED: correct these bitbanding references for the off-board LEDs
#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) // on-board blue LED
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4))) // off-board red LED
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 0*4))) // off-board green LED
#define YELLOW_LED   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 4*4))) // off-board yellow LED
#define ORANGE_LED   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 3*4))) // off-board orange LED

void initHw()
{
    initSystemClockTo40Mhz();
    initLedPb();
    initSysTick(SYSTIC_1KHZ);
    initTimer1();

    // Turn off all the LEDs
    BLUE_LED = 0;
    RED_LED = 1;
    GREEN_LED = 1;
    YELLOW_LED = 1;
    ORANGE_LED = 1;

    // This will be removed later
    initUart0();
}

//-----------------------------------------------------------------------------
// YOUR UNIQUE CODE
// REQUIRED: add any custom code in this space
//-----------------------------------------------------------------------------

/*
int testFn()
{
    int x = 12;
    int y = 1;
    int z = 4;
    int result = x + y + z;
    return result;
}

void readFlash()
{
    (*((volatile uint32_t *)0x0003FFFC));
}

void testPeripheralBitband()
{
    // Test peripheral region - 0x40000000
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    GPIO_PORTF_DIR_R = 2;
    GPIO_PORTF_DEN_R = 2;

    // Test bit-band region - 0x42000000
    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4))) = 1;
}
*/

// ------------------------------------------------------------------------------
//  Task functions
// ------------------------------------------------------------------------------

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle()
{
    while(true)
    {
        /*
        // Debug code
        __asm(" MOV R4, #69");
        __asm(" MOV R5, #0x19");
        __asm(" MOV R6, #0x12");
        __asm(" MOV R7, #0x96");
        __asm(" MOV R8, #69");
        __asm(" MOV R9, #0x69");
        __asm(" MOV R10, #69");
        __asm(" MOV R11, #0x277");
        */

        ORANGE_LED = 0;
        waitMicrosecond(1000);
        ORANGE_LED = 1;
        yield();
    }
}

void flash4Hz()
{
    while(true)
    {
        GREEN_LED ^= 1;
        sleep(125);
    }
}

void oneshot()
{
    while(true)
    {
        wait(flashReq);
        YELLOW_LED = 0;
        sleep(1000);
        YELLOW_LED = 1;
    }
}

void partOfLengthyFn()
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn()
{
    uint16_t i;
    while(true)
    {
        wait(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        RED_LED ^= 1;
        post(resource);
    }
}

void readKeys()
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            YELLOW_LED ^= 1;
            RED_LED = 0;
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            RED_LED = 1;
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            destroyThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce()
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative()
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant()
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important()
{
    while(true)
    {
        wait(resource);
        BLUE_LED = 1;
        sleep(1000);
        BLUE_LED = 0;
        post(resource);
    }
}

int main(void)
{
    /*
    // MPU Test
    enableBackgroundRegionRule();
    enableFlashRule();

    enableSRAMRule(SRAM_REGION_0, SIZE_8KIB, REGION_2);
    enableSRAMRule(SRAM_REGION_1, SIZE_8KIB, REGION_3);
    enableSRAMRule(SRAM_REGION_2, SIZE_8KIB, REGION_4);
    enableSRAMRule(SRAM_REGION_3, SIZE_8KIB, REGION_5);

    sRAMSubregionEnable(REGION_2, 0xFF);
    sRAMSubregionDisable(REGION_3, 0x02);
    sRAMSubregionEnable(REGION_4, 0xFF);
    sRAMSubregionEnable(REGION_5, 0xFF);

    enableMPU();

    disablePrivilegeMode();

    // Causes Bus Fault
    // GPIO_PORTE_DATA_R |= 2;

    // Read Flash
    readFlash();
    // Read a peripheral
    testPeripheralBitband();
    // Test to see if the SRAM is accessible or not
    *((volatile uint32_t *)0x20002400);
    *((volatile uint32_t *)0x20002000);
    *((volatile uint32_t *)0x20000000);
    testFn();
    // shell();
    */

    bool ok;

    // Initialize hardware
    initHw();

    // Enable the Memory Management Fault Handler
    enableExceptionHandler(NVIC_SYS_HND_CTRL_USAGE | NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_MEM);

    initRtos();

    // Power-up flash
    GREEN_LED = 0;
    waitMicrosecond(250000);
    GREEN_LED = 1;
    waitMicrosecond(250000);

    // Initialize semaphores
    createSemaphore(keyPressed, 1);
    createSemaphore(keyReleased, 0);
    createSemaphore(flashReq, 5);
    createSemaphore(resource, 1);

    // Add required idle process at lowest priority
    ok = createThread(idle, "Idle", 7, 1024);

    // Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 6, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 4, 1024);
    ok &= createThread(oneshot, "OneShot", 2, 1024);
    ok &= createThread(readKeys, "ReadKeys", 6, 1024);
    ok &= createThread(debounce, "Debounce", 6, 1024);
    ok &= createThread(important, "Important", 0, 1024);
    ok &= createThread(uncooperative, "Uncoop", 6, 1024);
    ok &= createThread(errant, "Errant", 6, 1024);
    ok &= createThread(shell, "Shell", 6, 1024);

    // Loads up the task indices from the tcb in order of priority
    initTaskNextPriorities();

#ifdef DEBUG
    infoTcb();
#endif

    // Start up RTOS
    if (ok)
        startRtos(); // never returns
    else
        RED_LED = 0;

    return 0;
}
