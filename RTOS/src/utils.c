/*
 * utils.c
 *
 *  Created on: Oct 14, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#include "uart0.h"
#include "utils.h"

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

int mSprinf(const char * buffer, const char * format, ...)
{
    return -1;
}
