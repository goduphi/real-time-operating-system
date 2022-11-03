/*
 *  Created on: Oct 30, 2022
 *      Author: Sarker Nadir Afridi Azmi
 */

#include <mcuSpecific/common_includes/tm4c123gh6pm.h>
#include "sysTickTimer.h"

void sysTickTimerInit(uint32_t loadValue)
{
    NVIC_ST_CTRL_R |= (NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE);
    NVIC_ST_RELOAD_R = (loadValue & 0x00FFFFFF);
}
