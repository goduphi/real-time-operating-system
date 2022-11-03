/*
 *  Created on: Oct 14, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#include <stdarg.h>
#include <mcuSpecific/uart/uart0.h>
#include <rtos/include/tString.h>
#include <rtos/include/utils.h>

inline void rtosPutc(const char c)
{
    putcUart0(c);
}

inline void rtosPuts(const char *string)
{
    putsUart0((char *)string);
}

inline uint8_t getNumberOfDigitsDecimal(uint32_t n)
{
    uint8_t res = 0;

    if(n == 0)
    {
        return 1;
    }

    while(n)
    {
        res++;
        n /= 10;
    }

    return res;
}

inline void rtosPrintReservedSpace(bool reserveSpace, uint8_t spaceCount, uint8_t spaceToSubtract)
{
    if (reserveSpace)
    {
        spaceCount -= spaceToSubtract;
        while (spaceCount--)
        {
            rtosPutc(' ');
        }
    }
}

void printU8Decimal(uint8_t n)
{
    // An 8-bit integer is always limited to 3 digits
    uint8_t _2ndDigit = (n % 10);   // 2nd digit
    n /= 10;
    uint8_t _1stDigit = (n % 10);   // 1st digit
    n /= 10;                        // Implied 0th digit

    if (n != 0)
    {
        rtosPutc(n + '0');
    }

    if (_1stDigit != 0)
    {
        rtosPutc(_1stDigit + '0');
    }

    // Print the last digit as the value passed in can be a 0
    rtosPutc(_2ndDigit + '0');
}

void printU8Hex(uint8_t n)
{
    uint8_t lowerNibble = (n & 0xF);
    char digit = '\0';

    n >>= 4;    // Implied upper nibble
    digit = ((n > 9) ? (n - 10 + 'A') : (n + '0'));

    rtosPutc(digit);

    digit = ((lowerNibble > 9) ? (lowerNibble - 10 + 'A') : (lowerNibble + '0'));

    rtosPutc(digit);
}

void printUint32InHex(uint32_t n)
{
    printU8Hex((n >> 24) & 0xFF);
    printU8Hex((n >> 16) & 0xFF);
    printU8Hex((n >> 8) & 0xFF);
    printU8Hex(n & 0xFF);
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

void printUint32InDecimal(uint32_t n)
{
    if(n == 0)
        putcUart0('0');
    // There can be a maximum of 10 character inside of a 32-bit integer
    char buffer[10] = { 0 };
    int8_t i = 9;
    while(n)
    {
        buffer[i] = (n % 10) + '0';
        n /= 10;
        i--;
    }
    if(i < 0)
        i = 0;
    for(; i < 10; i++)
        putcUart0(buffer[i]);
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

// The function is intentionally written with only 255 max space reservation
//void printfString(uint8_t spaceToReserve, char* s)
//{
//    putsUart0(s);
//    spaceToReserve -= strLen(s);
//    while(spaceToReserve--)
//        putcUart0(' ');
//}

// Only looks for one conversion specifier
void printfInteger(const char* format, int8_t spaceToReserve, uint32_t n)
{
    // ni represents the counter for printing the integer value and
    // the spaces to reserve
    uint8_t i = 0;
    // __%u
    while(format[i] != '%' && format[i] != '\0')
    {
        putcUart0(format[i]);
        i++;
    }

    i++;
    bool padLeft = (format[i] == '-');

    uint8_t nLen = getNumberOfDigitsDecimal(n);

    if(padLeft && spaceToReserve >= nLen)
    {
        spaceToReserve -= nLen;
        while(spaceToReserve--)
            putcUart0(' ');
    }
    // Do not use this counter anymore as it remembers where
    // we found the format specifier
    // Find out what the conversion specifier is
    i += (padLeft);

    switch(format[i])
    {
    case 'u':
        printUint32InDecimal(n);
        break;
    case 'x':
        break;
    case 'd':
        break;
    }

    if(!padLeft && spaceToReserve > nLen)
    {
        spaceToReserve -= nLen;
        while(spaceToReserve--)
            putcUart0(' ');
    }

    // Start right after the conversion specifier
    i++;
    while(format[i] != '\0')
    {
        putcUart0(format[i]);
        i++;
    }
}

int rtosPrintf(const char *format, ...)
{
    int ret = 0;

    va_list argList;
    va_start(argList, format);

    if (format == 0)
    {
        return -1;
    }

    while ((*format != '\0') && (*format != '%'))
    {
        rtosPutc(*format);
        format++;
    }

    while (*format != '\0')
    {
        if (*format == '%')
        {
            bool    reserveSpace    = false;
            bool    rightJustify    = false;
            uint8_t spaceCount = 0;
            uint8_t spaceToSubtract = 0;

            format++;

            // Special case: % may be the last character
            if (*format == '\0')
            {
                return ret;
            }

            // Special case: % may be followed by another %
            if (*format == '%')
            {
                rtosPutc('%');
            }

            if (*format == '-')
            {
                rightJustify = true;
                format++;
            }

            // Calculate space to reserve
            while ((*format >= '0') && (*format <= '9'))
            {
                reserveSpace = true;
                spaceCount = ((spaceCount * 10) + (*format - '0'));
                format++;
            }

            switch (*format)
            {
            case 'x':
            case 'X':
                {
                    const uint32_t arg = va_arg(argList, uint32_t);
                    printUint32InHex(arg);
                }
                break;
            case 'u':
            case 'U':
                {
                    const uint32_t arg = va_arg(argList, uint32_t);
                    spaceToSubtract = getNumberOfDigitsDecimal(arg);
                    rtosPrintReservedSpace((reserveSpace && rightJustify), spaceCount, spaceToSubtract);
                    printUint32InDecimal(arg);
                }
                break;
            case 'c':
                {
                    const char c = va_arg(argList, char);
                    rtosPrintReservedSpace((reserveSpace && rightJustify), spaceCount, 1);
                    rtosPutc(c);
                }
                break;
            case 's':
                {
                    const char *arg = va_arg(argList, const char *);
                    spaceToSubtract = strLen(arg);
                    rtosPrintReservedSpace((reserveSpace && rightJustify), spaceCount, spaceToSubtract);
                    rtosPuts(arg);
                }
                break;
            default:
                rtosPuts("Conversion specifier not handled");
                break;
            }

            rtosPrintReservedSpace((reserveSpace && !rightJustify), spaceCount, spaceToSubtract);
        }
        else
        {
            rtosPutc(*format);
        }

        format++;
    }

    va_end(argList);

    return ret;
}
