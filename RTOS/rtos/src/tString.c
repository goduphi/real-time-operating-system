/*
 *  Created on: Sep 6, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#include <rtos/include/tString.h>

/** @brief  Compares two strings
 *  @param str0                 String to compare against
 *  @param str1                 String to compare
 *  @param maxCharsToCompare    Max number of characters to compare
 *  @return                     True if strings are equal, otherwise false
 */
bool stringCompare(const char str0[], const char str1[], uint8_t maxCharsToCompare)
{
    uint8_t index = 0;

    while((str0[index] != '\0') && (str1[index] != '\0') && (index < maxCharsToCompare))
    {
        if(str0[index] != str1[index])
        {
            return false;
        }

        index++;
    }

    return (!str0[index] && !str1[index]);
}

/** @brief  Copies the source string to the destination buffer
 *  @param src              Source string
 *  @param dest             Destination buffer
 *  @param maxCharsToCopy   Max number of characters to compare
 *  @return                 A pointer to the destination buffer
 */
char *stringCopy(const char *src, char *dest, uint32_t maxCharsToCopy)
{
    if ((src != 0) && (dest != 0))
    {
        uint8_t i = 0;

        for(i = 0; src[i] != '\0' && (i < maxCharsToCopy); i++)
        {
            dest[i] = src[i];
        }

        dest[i] = '\0';
    }

    return dest;
}

/** @brief  Calculates the length of a given string
 *  @param str  String who's length is to be computed
 *  @return     The length of the string
 */
uint32_t strLen(const char *str)
{
    uint32_t len = 0;

    while(str[len] != '\0')
    {
        len++;
    }

    return len;
}

