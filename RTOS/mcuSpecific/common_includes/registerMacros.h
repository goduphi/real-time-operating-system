/*
 *  Created on: Nov 2, 2022
 *      Author: Sarker Nadir Afridi Azmi
 */

#pragma once

#include <stdint.h>

#define GET_REG32(address, offset)   (*((volatile uint32_t *)(((uint32_t)address) + (uint32_t)offset)))
