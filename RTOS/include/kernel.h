/*
 * kernel.h
 *
 *  Created on: Sep 29, 2021
 *      Author: afrid
 */

#ifndef INCLUDE_KERNEL_H_
#define INCLUDE_KERNEL_H_

#include <stdint.h>

void setPspMode();
void setPsp(uint32_t address);
void enablePrivilegeMode();

#endif /* INCLUDE_KERNEL_H_ */
