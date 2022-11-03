#pragma once

#include <stdint.h>

void printUint8InDecimal(uint8_t n);
void printUint8InHex(uint8_t n);
void printUint32InHex(uint32_t n);
uint32_t hexStringToUint32(const char* n);
void printUint32InDecimal(uint32_t n);
void printUint32InBinary(uint32_t n);
// void printfString(uint8_t spaceToReserve, char* s);
uint8_t numberOfDigitsInInteger(uint32_t n);
void printfInteger(const char* format, int8_t spaceToReserve, uint32_t n);
int rtosPrintf(const char *format, ...);

inline uint32_t roundUp(uint32_t numToRound, uint32_t multiple)
{
    return ((numToRound + multiple - 1) / multiple) * multiple;
}
