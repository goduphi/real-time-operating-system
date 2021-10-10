/*
 * kernel.h
 *
 *  Created on: Sep 29, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#ifndef INCLUDE_KERNEL_H_
#define INCLUDE_KERNEL_H_

#include <stdint.h>

// Set the processor to use the Process Stack Pointer for thread mode
void setPspMode();
// Set the starting address of the process stack. Make sure to pass in
// &Stack[N] instead of &Stack[N - 1]. This is a fully descending stack.
// So, the SP is decrement first to N - 1 then data is written.
void setPsp(uint32_t address);
// Puts the processor into privilege mode
void enablePrivilegeMode();
void enableExceptionHandler(uint32_t type);
void enableFlashRule();
void enableSRAMRule();
void enableMPU();

#endif /* INCLUDE_KERNEL_H_ */
