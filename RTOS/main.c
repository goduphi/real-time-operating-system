#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "kernel.h"
#include "cli.h"

#define N   256

uint32_t ThreadStack[N];

void initHw()
{
    initSystemClockTo40Mhz();
}

int testFn()
{
    int x = 12;
    int y = 1;
    int z = 4;
    int result = x + y + z;
    return result;
}

int main(void)
{
    initHw();
    setPsp((uint32_t)&ThreadStack[N - 1]);
    setPspMode();
    enablePrivilegeMode();
    shell();
    // testFn();
}
