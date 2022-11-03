/*
 *  Created on: Nov 1, 2022
 *      Author: Sarker Nadir Afridi Azmi
 */
#include <mcuSpecific/nvic/nvic.h>

void nvicEnableExceptionHandlers(uint32_t exceptionType)
{
    NVIC_SYS_HND_CTRL_R |= exceptionType;
}
