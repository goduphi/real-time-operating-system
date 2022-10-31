/*
 * Description: This file contains APIs used to configure the memory protection unit
 * for the M4F memory accesses.
 *
 *  Created on: Oct 30, 2022
 *      Author: Sarker Nadir Afridi Azmi
 */

#include <mcuSpecific/include/tm4c123gh6pm.h>
#include <mcuSpecific/include/tm4c123gh6pmMemoryMap.h>
#include <mcuSpecific/mpu/mpu.h>

#define MPU_AP_FIELD_OFFSET         (24)
#define MPU_AP_ACCESS_PRIVILEGE     (1 << MPU_AP_FIELD_OFFSET)
#define MPU_AP_ACCESS_FULL          (3 << MPU_AP_FIELD_OFFSET)

// This macro is used for obtaining the SIZE field for a region: size in bytes = 2^(SIZE + 1)
#define MPU_REGION_SIZE_FIELD(SIZE) (SIZE << 1)

inline void sramSubregionDisable(uint8_t region, uint32_t subregionBits)
{
    if((region < MPU_REGION_2) || (region > MPU_REGION_5))
        return;

    NVIC_MPU_NUMBER_R = region;
    NVIC_MPU_ATTR_R &= ~NVIC_MPU_ATTR_SRD_M;
    // Disable the subregions
    NVIC_MPU_ATTR_R |= (subregionBits << 8);
}

// This will be the background region that gives access to the entire memory range
// Remove Executable privilege since we do not want everyone to be able to execute bit-banded memory for example
void mpuEnableBackgroundRegionRule(void)
{
    // Region 0 will be used to give RWX access for both privilege and unprivilege
    NVIC_MPU_BASE_R = FLASH_BASE | NVIC_MPU_BASE_VALID | MPU_REGION_0;
    // Region size = 2^(SIZE + 1)
    // For the entire memory range, the SIZE would equal 0x1F, page 192
    NVIC_MPU_ATTR_R = NVIC_MPU_ATTR_XN | MPU_AP_ACCESS_FULL | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE
                    | NVIC_MPU_ATTR_BUFFRABLE | MPU_REGION_SIZE_FIELD(MPU_REGION_SIZE_4GIB) | NVIC_MPU_ATTR_ENABLE;
}

void mpuEnableFlashRule(void)
{
    // Set up the address for the MPU base
    // If I want to cover the entire Flash memory region of 256K,
    // the starting address needs to be 0x00000000

    // We will also use region 0 due to the relative prioritization
    // of the 8 memory regions
    // Base address | Valid Bit | Region Number
    NVIC_MPU_BASE_R = FLASH_BASE | NVIC_MPU_BASE_VALID | MPU_REGION_1;
    // Region size = 2^(SIZE + 1)
    // For 256K, it will be 2^18, where SIZE = 17
    NVIC_MPU_ATTR_R = MPU_AP_ACCESS_FULL | NVIC_MPU_ATTR_CACHEABLE | MPU_REGION_SIZE_FIELD(MPU_REGION_SIZE_256KIB) | NVIC_MPU_ATTR_ENABLE;
}

// This rule takes away RW privilege for unprivileged code
// Parameters: Starting address of the region, SIZE parameter of 2^(SIZE + 1), Region number
void mpuEnableSramRule(uint32_t startAddress, uint32_t size, uint32_t region)
{
    // Base address | Valid Bit | Region Number
    NVIC_MPU_BASE_R = startAddress | NVIC_MPU_BASE_VALID | region;
    // Region size = 2^(SIZE + 1)
    NVIC_MPU_ATTR_R = MPU_AP_ACCESS_PRIVILEGE | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | MPU_REGION_SIZE_FIELD(size) | NVIC_MPU_ATTR_ENABLE;
}

void mpuSetSrdBits(uint32_t srdBits)
{
    sramSubregionDisable(MPU_REGION_2, (srdBits >> 0) & 0x000000FF);
    sramSubregionDisable(MPU_REGION_3, (srdBits >> 8) & 0x000000FF);
    sramSubregionDisable(MPU_REGION_4, (srdBits >> 16) & 0x000000FF);
    sramSubregionDisable(MPU_REGION_5, (srdBits >> 24) & 0x000000FF);
}

void mpuEnable(void)
{
    // NVIC_MPU_CTRL_PRIVDEFEN - Use this for default memory region
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE;
}

void mpuDisable(void)
{
    // NVIC_MPU_CTRL_PRIVDEFEN - Use this for default memory region
    NVIC_MPU_CTRL_R &= ~NVIC_MPU_CTRL_ENABLE;
}
