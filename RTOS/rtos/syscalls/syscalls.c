/*
 *  Created on: Sep 22, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#include <rtos/syscalls/syscalls.h>

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm(" SVC  #7");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm(" SVC  #8");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm(" SVC  #9");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm(" SVC  #10");
}

// Turns priority inheritance on or off
void pi(bool on)
{
    return;
}

// Selected priority or round-robin scheduling
void sched(bool prioOn)
{
    __asm(" SVC #11");
}

// Turns preemption on or off
void preempt(bool on)
{
    __asm(" SVC #12");
}

// Requests a hard reset of the microcontroller unit
void rebootSystem(void)
{
    __asm(" SVC #13");
}

// Displays the PID of the process (thread)
void pidof(uint32_t *pid, const char name[])
{
    __asm(" SVC #14");
}

// Kills the process (thread) with matching PID
void kill(uint32_t pid)
{
    __asm(" SVC #15");
}

// Insert proc_name &
void resume(const char name[])
{
    __asm(" SVC #16");
}

// Displays the inter-process (thread) communication state
void ipcs(semaphoreInfo *semInfo)
{
    __asm(" SVC #17");
}

// Displays the process (thread) information
void ps(taskInfo *ti, uint8_t *tiCount)
{
    __asm(" SVC #18");
}
