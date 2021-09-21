/* Common Terminal Interface Library
 * Sarker Nadir Afridi Azmi
 *
 * This file contains the implementation of the functions prototypes defined in
 * common_terminal_interface.h.
 */

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "gpio.h"
#include "cli.h"
#include "tString.h"

#include <stdio.h>  // This will be removed from the future version as it takes 4096 Bytes of stack space to call

#define MAX_HISTORY_NUMBER              5
#define MAX_HISTORY_COMMAND_LENGTH      10

// The commands will be stored in a queue
char history[MAX_HISTORY_NUMBER][MAX_HISTORY_COMMAND_LENGTH];
uint8_t historyReadPtr = 0;
uint8_t historyWritePtr = 0;

void updateHistory(const char* command)
{
    // Copy over the command to the history buffer
    uint8_t i = 0;
    for(i = 0; command[i] != '\0' && i < MAX_HISTORY_COMMAND_LENGTH; i++)
        history[historyWritePtr][i] = command[i];
    historyWritePtr = (historyWritePtr + 1) % MAX_HISTORY_NUMBER;
}

char* getCommandFromHistory()
{
    return 0;
}

// Gets a user defined string using the serial peripheral Uart0
static void getsUart0(USER_DATA* data)
{
    // Keeps count of how many characters we currently have in the buffer
    // Also serves to provide us with the position of the last character input
    uint8_t count = 0;

    while(true)
    {
        char c = getcUart0();

        // If the character is a backspace
        // Ctrl-H or Ctrl-?
        if(c == 8 || c == 127)
        {
            // If the count is greater than zero, we decrement the count
            // This means we are essentially erasing the previous character
            // Else, we just look for new characters
            if(count > 0)
                count--;
            else
                continue;
        }
        // If the character is a carriage return (13) or a line feed (10)
        else if(c == 13 || c == 10)
        {
            data->buffer[count] = 0;
            // Store the current command in the history buffer
            updateHistory(data->buffer);
            return;
        }
        else if(c < 32)
            continue;
        // If the character is anything greater than a space
        else if(c >= 32)
        {
            data->buffer[count++] = c;
            if(count == MAX_CHARS)
            {
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}

// Tokenizes the string in place
static void parseField(USER_DATA* data)
{
    data->fieldCount = 0;
    bool isNewToken = false;
    uint8_t i = 0;
    for(i = 0; (data->buffer[i] != '\0') && (data->fieldCount < MAX_FIELDS); i++)
    {
        // Only tokenize alpha numeric characters
        if(((data->buffer[i] >= 'a' && data->buffer[i] <= 'z') ||
            (data->buffer[i] >= '0' && data->buffer[i] <= '9') ||
            (data->buffer[i] >= 'A' && data->buffer[i] <= 'Z') ||
             data->buffer[i] >= '&') &&
            !isNewToken)
        {
            isNewToken = true;
            // Record where this new token starts
            data->fieldPosition[data->fieldCount] = i;
            if(data->buffer[i] >= '0' && data->buffer[i] <= '9')
                data->fieldType[data->fieldCount++] = 'n';
            else
                data->fieldType[data->fieldCount++] = 'a';
        }
        else if(!(data->buffer[i] >= 'a' && data->buffer[i] <= 'z') &&
                !(data->buffer[i] >= '0' && data->buffer[i] <= '9') &&
                !(data->buffer[i] >= 'A' && data->buffer[i] <= 'Z') &&
                !(data->buffer[i] >= '&'))
        {
            isNewToken = false;
            // Replace the delimeters with null terminators
            data->buffer[i] = '\0';
        }
    }
    return;
}

// Checks to see if a particular command is valid or not
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    // Only count the arguments
    if(data->fieldCount - 1 < minArguments) return false;
    if(stringCompare(getFieldString(data, 0), strCommand, MAX_CHARS)) return true;
    return false;
}

// Get a pointer to the requested string field
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if((fieldNumber < MAX_FIELDS) &&
       (fieldNumber < data->fieldCount) &&
        (data->fieldType[fieldNumber] == 'a'))
        return data->buffer + data->fieldPosition[fieldNumber];
    return 0;
}

// Returns a 32-bit signed integer
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    int32_t signedInteger32bits = 0;
    if((fieldNumber < MAX_FIELDS) &&
       (fieldNumber < data->fieldCount) &&
       (data->fieldType[fieldNumber] == 'n'))
    {
         char* numberString = data->buffer + data->fieldPosition[fieldNumber];
         uint8_t i;
         for(i = 0; numberString[i] != '\0'; i++)
             signedInteger32bits = (signedInteger32bits * 10) + (numberString[i] - '0');
    }
    return signedInteger32bits;
}

// To be removed later
void initLed()
{
    enablePort(PORTF);
    selectPinPushPullOutput(PORTF, 1);
}

void rebootSystem()
{
    NVIC_APINT_R = (0x05FA0000 | NVIC_APINT_SYSRESETREQ);
}

// Displays the process (thread) information
void ps()
{
    putsUart0("PS called\n");
}

// Displays the inter-process (thread) communication state
void ipcs()
{
    putsUart0("IPCS called\n");
}

// Kills the process (thread) with matching PID
void kill(int32_t pid)
{
    char buffer[16];
    sprintf(buffer, "pid %d killed\n", pid);
    putsUart0(buffer);
}

// Turns priority inheritance on or off
void pi(bool on)
{
    char buffer[16];
    sprintf(buffer, "PI %s\n", (on) ? "on" : "off");
    putsUart0(buffer);
}

// Turns preemption on or off
void preempt(bool on)
{
    char buffer[16];
    sprintf(buffer, "preempt %s\n", (on) ? "on" : "off");
    putsUart0(buffer);
}

// Selected priority or round-robin scheduling
void sched(bool prioOn)
{
    char buffer[16];
    sprintf(buffer, "sched %s\n", (prioOn) ? "prio" : "rr");
    putsUart0(buffer);
}

// Displays the PID of the process (thread)
void pidof(char name[])
{
    char buffer[32];
    sprintf(buffer, "%s launched\n", name);
    putsUart0(buffer);
}

void shell(void)
{
    initUart0();
    setUart0BaudRate(115200, 40e6);
    initLed();

    USER_DATA data;

    while(true)
    {
        putcUart0('>');
        getsUart0(&data);
        parseField(&data);

        if(isCommand(&data, "reboot", 0))
        {
            putsUart0("Rebooting...\n");
            rebootSystem();
        }
        else if(isCommand(&data, "ps", 0))
        {
            ps();
        }
        else if(isCommand(&data, "ipcs", 0))
        {
            ipcs();
        }
        else if(isCommand(&data, "kill", 1))
        {
            int32_t pid = getFieldInteger(&data, 1);
            kill(pid);
        }
        else if(isCommand(&data, "pi", 1))
        {
            char* arg = getFieldString(&data, 1);
            pi(stringCompare(arg, "ON", 2));
        }
        else if(isCommand(&data, "preempt", 1))
        {
            char* arg = getFieldString(&data, 1);
            preempt(stringCompare(arg, "ON", 2));
        }
        else if(isCommand(&data, "sched", 1))
        {
            char* arg = getFieldString(&data, 1);
            sched(stringCompare(arg, "prio", 4));
        }
        else if(isCommand(&data, "pidof", 1))
        {
            char* arg = getFieldString(&data, 1);
            pidof(arg);
        }
        else if(isCommand(&data, "procName", 1))
        {
            char* arg = getFieldString(&data, 1);
            if(stringCompare(arg, "&", 1))
            {
                putsUart0("Running proc_name in the background\n");
                setPinValue(PORTF, 1, 1);
            }
            else
                putsUart0("Invalid argument to proc_name\n");
        }
    }
}
