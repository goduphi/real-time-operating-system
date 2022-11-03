#include <stdint.h>
#include <stdbool.h>
#include <mcuSpecific/common_includes/tm4c123gh6pm.h>
#include <mcuSpecific/clock/clock.h>
#include <mcuSpecific/uart/uart0.h>
#include <mcuSpecific/gpio/gpio.h>
#include <rtos/kernel/kernel.h>
#include <rtos/syscalls/syscalls.h>
#include <rtos/cli/cli.h>
#include <rtos/include/wait.h>

#define keyPressed      (1)
#define keyReleased     (2)
#define flashReq        (3)
#define resource        (4)

// REQUIRED: correct these bitbanding references for the off-board LEDs
#define BLUE_LED        (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) // on-board blue LED
#define RED_LED         (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4))) // off-board red LED
#define GREEN_LED       (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 0*4))) // off-board green LED
#define YELLOW_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 4*4))) // off-board yellow LED
#define ORANGE_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 3*4))) // off-board orange LED

void initHw(void)
{
    initSystemClockTo40Mhz();
    initUart0();

    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    // PB2 is connected to the anode of the LED
    selectPinPushPullOutput(PORTB, 2);
    setPinValue(PORTB, 2, 1);

    selectPinPushPullOutput(PORTA, 2);
    selectPinPushPullOutput(PORTA, 3);
    selectPinPushPullOutput(PORTA, 4);
    selectPinPushPullOutput(PORTE, 0);
    selectPinPushPullOutput(PORTF, 2);

    setPinCommitControl(PORTD, 7);

    // Configure all the push buttons pins to be inputs
    selectPinDigitalInput(PORTC, 4);
    selectPinDigitalInput(PORTC, 5);
    selectPinDigitalInput(PORTC, 6);
    selectPinDigitalInput(PORTC, 7);
    selectPinDigitalInput(PORTD, 6);
    selectPinDigitalInput(PORTD, 7);

    // Configure pull-ups for all the push buttons
    enablePinPullup(PORTC, 4);
    enablePinPullup(PORTC, 5);
    enablePinPullup(PORTC, 6);
    enablePinPullup(PORTC, 7);
    enablePinPullup(PORTD, 6);
    enablePinPullup(PORTD, 7);

    // Turn off all the LEDs
    BLUE_LED = 0;
    RED_LED = 1;
    GREEN_LED = 1;
    YELLOW_LED = 1;
    ORANGE_LED = 1;
}

//-----------------------------------------------------------------------------
// YOUR UNIQUE CODE
// REQUIRED: add any custom code in this space
//-----------------------------------------------------------------------------
uint8_t readPbs(void)
{
    uint8_t sum = 0;
    if(!getPinValue(PORTC, 4))
        sum += 1;
    if(!getPinValue(PORTC, 5))
        sum += 2;
    if(!getPinValue(PORTC, 6))
        sum += 4;
    if(!getPinValue(PORTC, 7))
        sum += 8;
    if(!getPinValue(PORTD, 6))
        sum += 16;
    if(!getPinValue(PORTD, 7))
        sum += 32;
    return sum;
}

// ------------------------------------------------------------------------------
//  Task functions
// ------------------------------------------------------------------------------
// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle()
{
    while(true)
    {
        ORANGE_LED = 0;
        waitMicrosecond(1000000);
        ORANGE_LED = 1;
        yield();
    }
}

void flash4Hz()
{
    while(true)
    {
        GREEN_LED ^= 1;
        sleep(125);
    }
}

void oneshot()
{
    while(true)
    {
        wait(flashReq);
        YELLOW_LED = 0;
        sleep(1000);
        YELLOW_LED = 1;
    }
}

void partOfLengthyFn()
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn()
{
    uint16_t i;
    while(true)
    {
        wait(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        RED_LED ^= 1;
        post(resource);
    }
}

void readKeys()
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            YELLOW_LED ^= 1;
            RED_LED = 0;
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            RED_LED = 1;
        }
        if ((buttons & 4) != 0)
        {
            resume("Flash4Hz");
        }
        if ((buttons & 8) != 0)
        {
            kill((uint32_t)flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            // setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce()
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative()
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant()
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important()
{
    while(true)
    {
        wait(resource);
        BLUE_LED = 1;
        sleep(1000);
        BLUE_LED = 0;
        post(resource);
    }
}

int main(void)
{
    bool ok = true;

    // Initialize hardware
    initHw();
    initRtos();

    // Power-up flash
    GREEN_LED = 0;
    waitMicrosecond(250000);
    GREEN_LED = 1;
    waitMicrosecond(250000);

    // Initialize semaphores
    createSemaphore(keyPressed, 1);
    createSemaphore(keyReleased, 0);
    createSemaphore(flashReq, 5);
    createSemaphore(resource, 1);

    // Add required idle process at lowest priority
    ok = createThread(idle, "Idle", 7, 1024);

    // Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 6, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 4, 1024);
    ok &= createThread(oneshot, "OneShot", 2, 1024);
    ok &= createThread(readKeys, "ReadKeys", 6, 1024);
    ok &= createThread(debounce, "Debounce", 6, 1024);
    ok &= createThread(important, "Important", 0, 1024);
    ok &= createThread(uncooperative, "Uncoop", 6, 1024);
    ok &= createThread(errant, "Errant", 6, 1024);
    ok &= createThread(shell, "Shell", 6, 3000);

    // Loads up the task indices from the tcb in order of priority
    initTaskNextPriorities();

#ifdef DEBUG
    infoTcb();
#endif

    // Start up RTOS
    if (ok)
    {
        startRtos(); // never returns
    }
    else
    {
        RED_LED = 0;
    }
}
