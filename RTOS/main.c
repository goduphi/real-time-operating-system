#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "kernel.h"
#include "cli.h"

#define N   256

uint32_t ThreadStack[N];

void initHw()
{
    initSystemClockTo40Mhz();
}

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

void readBlah()
{
    SYSCTL_RCGCGPIO_R = 1;
}

int main(void)
{
    initHw();
    // Enable the Memory Management Fault Handler
    enableExceptionHandler(NVIC_SYS_HND_CTRL_MEM);
    setPsp((uint32_t)&ThreadStack[N]);
    setPspMode();
    enableFlashRule();
    enableSRAMRule();
    enableMPU();
    enablePrivilegeMode();

    // Read Flash
    readFlash();
    // Test to see if the SRAM is accessible or not
    testFn();
    // Read a peripheral
    readBlah();
    // shell();
}
