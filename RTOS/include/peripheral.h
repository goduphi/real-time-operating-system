/*
 * peripheral.h
 *
 *  Created on: Nov 4, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef INCLUDE_PERIPHERAL_H_
#define INCLUDE_PERIPHERAL_H_

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"

#define SYSTIC_1KHZ     39999

void initSysTick(uint32_t loadValue);
void initLedPb();

#endif /* INCLUDE_PERIPHERAL_H_ */
