#pragma once

#include <stdint.h>

#define MPU_REGION_0            (0x00000000)    // Lowest priority
#define MPU_REGION_1            (0x00000001)
#define MPU_REGION_2            (0x00000002)
#define MPU_REGION_3            (0x00000003)
#define MPU_REGION_4            (0x00000004)
#define MPU_REGION_5            (0x00000005)
#define MPU_REGION_6            (0x00000006)    // Highest priority

#define MPU_REGION_SIZE_4GIB    (0x0000001F)    // SIZE = 31 in 2^(SIZE + 1) to obtain a 4GiB size
#define MPU_REGION_SIZE_256KIB  (0x00000011)    // SIZE = 17 in 2^(SIZE + 1) to obtain a 256kiB size
#define MPU_REGION_SIZE_8KIB    (0x0000000C)    // SIZE = 12 in 2^(SIZE + 1) to obtain a 8kiB size
#define MPU_REGION_SIZE_1KIB    (0x00000009)    // SIZE = 10 in 2^(SIZE + 1) to obtain a 1kiB size

#define SRAM_REGION_0_START     (SRAM_BASE)
#define SRAM_REGION_1_START     (SRAM_BASE + 0x2000)
#define SRAM_REGION_2_START     (SRAM_BASE + 0x4000)
#define SRAM_REGION_3_START     (SRAM_BASE + 0x6000)

void mpuEnableBackgroundRegionRule(void);
void mpuEnableFlashRule(void);
void mpuEnableSramRule(uint32_t startAddress, uint32_t size, uint32_t region);
void mpuSetSrdBits(uint32_t srdBits);
void mpuEnable(void);
void mpuDisable(void);
