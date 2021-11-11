/*
 * utils.c
 *
 *  Created on: Oct 14, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#include "uart0.h"
#include "utils.h"
#include "tString.h"

void printUint8InDecimal(uint8_t n)
{
    /*
     * Depending how many digits n has, set the divider
     * If n has only one digit, this will prevent leading
     * zeros from being printed.
     */
    uint16_t divider = 0;
    if(n < 10)
        divider = 1;
    else if(n < 100)
        divider = 10;
    else if(n < 1000)
        divider = 100;

    while(divider)
    {
        uint8_t currentDigit = n / divider;
        divider /= 10;
        putcUart0((currentDigit % 10) + '0');
    }
}

void printUint8InHex(uint8_t n)
{
    // Print 4 bits at a time
    int8_t i = 0;
    for(i = 1; i >= 0; i--)
    {
        uint8_t currentNumber = (n >> (i << 2)) & 0xF;
        if(currentNumber > 9)
        {
            switch(currentNumber)
            {
            case 10:
                putcUart0('A');
                break;
            case 11:
                putcUart0('B');
                break;
            case 12:
                putcUart0('C');
                break;
            case 13:
                putcUart0('D');
                break;
            case 14:
                putcUart0('E');
                break;
            case 15:
                putcUart0('F');
                break;
            }
        }
        else
            printUint8InDecimal(currentNumber);
    }
}

// This power function is very limited and not the best implementation.
// Runs in O(b)
uint32_t power32(uint8_t n, uint8_t b)
{
    uint32_t res = 1;
    for(; b; b--)
        res *= n;
    return res;
}

uint32_t hexStringToUint32(const char* n)
{
    if(n == 0)
        return 0;

    // 0x00002A
    int8_t len = (uint8_t)strLen(n), p = 0, endingIndex = 0;

    // Skip the 0x or 0X if they exist in the hex string
    endingIndex = (len >= 3 && n[0] == '0' && (n[1] == 'x' || n[1] == 'X')) ? 2 : 0;

    // Decrease the length by 1 because indexing starts at 0
    len--;

    uint32_t res = 0;
    while(len >= endingIndex)
    {
        if(n[len] >= '0' && n[len] <= '9')
            res += (n[len] - '0') * power32(16, p);
        else if((n[len] >= 'A' && n[len] <= 'F') || (n[len] >= 'a' && n[len] <= 'f'))
        {
            switch(n[len])
            {
            case 'A':
            case 'a':
                res += 10 * power32(16, p);
                break;
            case 'B':
            case 'b':
                res += 11 * power32(16, p);
                break;
            case 'C':
            case 'c':
                res += 12 * power32(16, p);
                break;
            case 'D':
            case 'd':
                res += 13 * power32(16, p);
                break;
            case 'E':
            case 'e':
                res += 14 * power32(16, p);
                break;
            case 'F':
            case 'f':
                res += 15 * power32(16, p);
                break;
            }
        }
        else
            break;
        len--;
        p++;
    }
    return (len < endingIndex) ? res : 0;
}

void printUint32InHex(uint32_t n)
{
    printUint8InHex((n >> 24) & 0xFF);
    printUint8InHex((n >> 16) & 0xFF);
    printUint8InHex((n >> 8) & 0xFF);
    printUint8InHex(n & 0xFF);
}

void printUint32InBinary(uint32_t n)
{
    uint32_t i = 0x80000000;
    putsUart0("0b");
    while(i)
    {
        putcUart0((n & i) ? '1' : '0');
        i >>= 1;
    }
}
