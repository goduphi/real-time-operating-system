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

int main(void)
{
    initHw();

    // Enable the Memory Management Fault Handler
    enableExceptionHandler(NVIC_SYS_HND_CTRL_MEM);

    setPsp((uint32_t)&ThreadStack[N]);
    setPspMode();

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
}
