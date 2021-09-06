#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "cli.h"
#include "clock.h"
#include "uart0.h"

void initHw()
{
    initSystemClockTo40Mhz();
}

int main(void)
{
    initHw();
    shell();
}
