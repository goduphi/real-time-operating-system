#pragma once

#include <stdbool.h>
#include <stdint.h>

bool stringCompare(const char string0[], const char string1[], uint8_t maxLengthToCompare);
char *stringCopy(const char *src, char *dest, uint32_t maxCharsToCopy);
uint32_t strLen(const char *str);
