/*
 * tString.c
 *
 *  Created on: Sep 6, 2021
 *      Author: afrid
 */

#include "tString.h"

// Compares to strings to see if they are equal or not
bool stringCompare(const char string1[], const char string2[], uint8_t size)
{
    uint8_t index = 0;
    // The comparison with MAX_CHARS is a bit unnecessary
    while((string1[index] != '\0') && (string2[index] != '\0') && index < size)
    {
        if(string1[index] != string2[index])
            return false;
        index++;
    }
    return !string1[index] && !string2[index];
}
