/*
 *  Created on: Oct 30, 2022
 *      Author: Sarker Nadir Afridi Azmi
 */

#pragma once

#include <stdint.h>

#define SYSTICK_LOAD_VALUE_1KHZ (39999)
#define SYSTICK_TWO_SECONDS     (2000)

void sysTickTimerInit(uint32_t loadValue);
