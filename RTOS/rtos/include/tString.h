/*
 * tString.h
 *
 *  Created on: Sep 6, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef RTOS_INCLUDE_TSTRING_H_
#define RTOS_INCLUDE_TSTRING_H_

#include <stdbool.h>
#include <stdint.h>

// Compares to strings to see if they are equal or not
bool stringCompare(const char string1[], const char string2[], uint8_t size);
void stringCopy(const char src[], char dest[], uint32_t size);
uint32_t strLen(const char* str);

#endif /* RTOS_INCLUDE_TSTRING_H_ */
