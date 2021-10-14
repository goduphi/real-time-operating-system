/*
 * kernel.h
 *
 *  Created on: Sep 29, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef INCLUDE_KERNEL_H_
#define INCLUDE_KERNEL_H_

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

#include <stdint.h>

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

#endif /* INCLUDE_KERNEL_H_ */
