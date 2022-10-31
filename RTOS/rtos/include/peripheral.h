/*
 * peripheral.h
 *
 *  Created on: Nov 4, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef RTOS_INCLUDE_PERIPHERAL_H_
#define RTOS_INCLUDE_PERIPHERAL_H_

#include <stdint.h>
#include "../../mcuSpecific/include/tm4c123gh6pm.h"
#include "../rtos/include/gpio.h"

#define SYSTIC_1KHZ         39999
#define ONE_SECOND_SYSTICK  1000
#define TWO_SECOND_SYSTICK  2000

void initSysTick(uint32_t loadValue);
void initTimer1();
void initLedPb();
uint8_t readPbs();

#endif /* RTOS_INCLUDE_PERIPHERAL_H_ */
