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

uint32_t* getPsp()
{
    return 0;
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
    if(region < (REGION_2 & 0xFF))
        return;
    // Base address | Valid Bit | Region Number
    NVIC_MPU_NUMBER_R = region;
    // Disable the sub-regions
    NVIC_MPU_ATTR_R |= subregion << 8;
}

void sRAMSubregionEnable(uint8_t region, uint32_t subregion)
{
    if(region < (REGION_2 & 0xFF))
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

void MPUFaultHandler()
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

void BusFaultHandler()
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

void UsageFaultHandler()
{
    //
    // Enter an infinite loop.
    //
    putsUart0("Usage fault occurred\n");
    while(1)
    {
    }
}
