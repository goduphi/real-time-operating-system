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
#include "syscalls.h"
#include "utils.h"

#define MAX_HISTORY_NUMBER              5
#define MAX_HISTORY_COMMAND_LENGTH      10

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
        // Treats & as a special character
        if(((data->buffer[i] >= 'a' && data->buffer[i] <= 'z') ||
            (data->buffer[i] >= '0' && data->buffer[i] <= '9') ||
            (data->buffer[i] >= 'A' && data->buffer[i] <= 'Z') ||
             data->buffer[i] == '&') &&
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
                !(data->buffer[i] == '&'))
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
        ((data->fieldType[fieldNumber] == 'a') || (data->fieldType[fieldNumber] == 'n')))
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

void shell(void)
{
    // initUart0();
    // setUart0BaudRate(115200, 40e6);

    USER_DATA data;

    while(true)
    {
        putsUart0("goduphi> ");
        getsUart0(&data);
        parseField(&data);

        if(isCommand(&data, "reboot", 0))
            rebootSystem();
        else if(isCommand(&data, "ps", 0))
        {
            taskInfo ti[MAX_TASKS_TASK_INFO];
            uint8_t tiCount = 0;
            ps(ti, &tiCount);
            uint32_t totalTime = 0;
            uint8_t i = 0;
            for(; i < tiCount; i++)
                totalTime += ti[i].time;

            putcUart0('\n');
            printfString(12, "Task Name");
            printfString(12, "PID");
            printfString(15, "CPU Usage (%)");
            printfString(12, "State");
            putsUart0("\n\n");
            for(i = 0; i < tiCount; i++)
            {
                printfString(12, ti[i].name);
                printfInteger("%u", 12, ti[i].pid);

                // This is very crucial. Multiplying by 100 * 100 causes a 32 bit uint overflow.
                // Use a uint64_t instead.
                uint64_t cpuUsage = (uint64_t)ti[i].time * 100 * 100;
                cpuUsage /= totalTime;

                // Emulate a floating point value
                char oneOverTen = cpuUsage % 10 + '0';
                cpuUsage /= 10;
                char oneOverHundred = cpuUsage % 10 + '0';
                cpuUsage /= 10;
                printfInteger("%-u", 2, (uint32_t)cpuUsage);
                putcUart0('.');
                putcUart0(oneOverTen);
                putcUart0(oneOverHundred);
                putsUart0("          ");

                // The state of the task should not be known by the user
                // This is just for demonstration purposes
                switch(ti[i].state)
                {
                case 0:
                    printfString(12, "INVALID");
                    break;
                case 1:
                    printfString(12, "UNRUN");
                    break;
                case 2:
                    printfString(12, "READY");
                    break;
                case 3:
                    printfString(12, "DELAYED");
                    break;
                case 4:
                    printfString(12, "BLOCKED");
                    break;
                case 5:
                    printfString(12, "KILLED");
                    break;
                }
                putcUart0('\n');
            }
            putcUart0('\n');
        }
        else if(isCommand(&data, "ipcs", 0))
        {
            semaphoreInfo semInfo[MAX_SEM_INFO_SIZE];
            ipcs(semInfo);
            printfString(14, "\nSemaphore");
            printfString(8, "Count");
            printfString(8, "Waiting");
            putsUart0("\n\n");
            uint8_t i = 0;
            for(; i < MAX_SEM_INFO_SIZE; i++)
            {
                printfString(14, semInfo[i].name);
                printfInteger("%u", 8, semInfo[i].count);
                uint8_t j = 0;
                for(; j < semInfo[i].waitingTasksNumber; j++)
                    printfInteger("%u", 8, semInfo[i].waitQueue[j]);
                putcUart0('\n');
            }
            putcUart0('\n');
        }
        else if(isCommand(&data, "kill", 1))
        {
            uint32_t pid = hexStringToUint32(getFieldString(&data, 1));
            if(pid == 0)
            {
                putsUart0("PID format is wrong bro!\n");
                continue;
            }
            kill(pid);
        }
        else if(isCommand(&data, "pi", 1))
        {
            // Will not be implemented
        }
        else if(isCommand(&data, "preempt", 1))
        {
            char* arg = getFieldString(&data, 1);
            bool ok = stringCompare(arg, "on", 4);
            if(ok)
                putsUart0("Preemption on\n");
            else
                putsUart0("Preemption off\n");
            preempt(ok);
        }
        else if(isCommand(&data, "sched", 1))
        {
            char* arg = getFieldString(&data, 1);
            bool ok = stringCompare(arg, "prio", 4);
            if(ok)
                putsUart0("Switching to Priority Scheduling\n");
            else
                putsUart0("Switching to Round Robin Scheduling\n");
            sched(ok);
        }
        else if(isCommand(&data, "pidof", 1))
        {
            char* taskName = getFieldString(&data, 1);
            uint32_t pid = 0;
            pidof(&pid, taskName);
            if(pid == 0)
            {
                putsUart0("Task not found\n");
                continue;
            }
            putsUart0("PID: 0x");
            printUint32InHex(pid);
            putcUart0('\n');
        }
        else
        {
            char* arg = getFieldString(&data, 1);
            if(stringCompare(arg, "&", 1) && data.fieldCount <= 2)
                resume(&data.buffer[data.fieldPosition[0]]);
            else
                putsUart0("Not a valid Goduphi command!\n");
        }
    }
}
