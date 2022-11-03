/*
 * Fix me: Update file with all the register mappings
 * Fix me: Update init function to configure 32/64-bit wide timer
 *
 *  Created on: Nov 2, 2022
 *      Author: Sarker Nadir Afridi Azmi
 */

#include <mcuSpecific/common_includes/tm4c123gh6pm.h>
#include <mcuSpecific/timer/timer.h>

bool gptInit(gptBaseAddress base, gptTimer timer, uint8_t configuration, uint8_t timerMode, uint8_t countDirection)
{
    const gptRegisterOffsets offset      = ((timer == GPT_TIMERA) ? GPTMTAMR : GPTMTBMR);
    const uint32_t baseAddressDifference = (_16_32_BIT_TIMER1_BASE - _16_32_BIT_TIMER0_BASE);

    if ((base > _16_32_BIT_TIMER5_BASE) || (timer > GPT_TIMERB))
    {
        return false;
    }

    // Enable clocks
    SYSCTL_RCGCTIMER_R |= (1 << (((uint32_t)base - (uint32_t)_16_32_BIT_TIMER0_BASE) / baseAddressDifference));
    _delay_cycles(3);

    GET_REG32(base, GPTMCTL) &= ~TIMER_CTL_TAEN;
    GET_REG32(base, GPTMCFG) |= configuration;
    GET_REG32(base, offset) |= timerMode | countDirection;
    GET_REG32(base, GPTMCTL) |= TIMER_CTL_TAEN;

    return true;
}
