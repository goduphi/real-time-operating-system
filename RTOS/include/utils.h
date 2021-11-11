/*
 * utils.h
 *
 *  Created on: Oct 14, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include <stdint.h>

void printUint8InDecimal(uint8_t n);
void printUint8InHex(uint8_t n);
void printUint32InHex(uint32_t n);
uint32_t hexStringToUint32(char* n);
void printUint32InBinary(uint32_t n);

#endif /* INCLUDE_UTILS_H_ */
