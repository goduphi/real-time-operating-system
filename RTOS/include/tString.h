/*
 * tString.h
 *
 *  Created on: Sep 6, 2021
 *      Author: afrid
 */

#ifndef INCLUDE_TSTRING_H_
#define INCLUDE_TSTRING_H_

#include <stdbool.h>
#include <stdint.h>

// Compares to strings to see if they are equal or not
bool stringCompare(const char string1[], const char string2[], uint8_t size);

#endif /* INCLUDE_TSTRING_H_ */
