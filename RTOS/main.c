#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "cli.h"
#include "clock.h"
#include "uart0.h"

void initHw()
{
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);
}

int main(void)
{
    initHw();

    USER_DATA data;
    data.fieldCount = 0;

	while(true)
	{
	    putsUart0("Enter command: ");
        getsUart0(&data);
        putsUart0("Original string: ");
        putsUart0(data.buffer);
        putcUart0('\n');
	    parseField(&data);

	    if(isCommand(&data, "set", 2))
	    {
	        char* command = getFieldString(&data, 0);
	        int32_t arg1 = getFieldInteger(&data, 1);
	        int32_t arg2 = getFieldInteger(&data, 2);

	        if(arg1 == 1)
	            putsUart0("1 received\n");

	        if(arg2 == 2)
	            putsUart0("2 received\n");
	    }
	}
}
