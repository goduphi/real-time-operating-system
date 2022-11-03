#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <mcuSpecific/common_includes/registerMacros.h>
#include <mcuSpecific/timer/timerRegisterMap.h>

#define TIMER_MAX_VALUE     (0xFFFFFFFF)

typedef enum _gptTimer
{
    GPT_TIMERA = 0x00,
    GPT_TIMERB = 0x01
} gptTimer;

typedef enum _gptBaseAddress
{
    _16_32_BIT_TIMER0_BASE = 0x40030000,
    _16_32_BIT_TIMER1_BASE = 0x40031000,
    _16_32_BIT_TIMER2_BASE = 0x40032000,
    _16_32_BIT_TIMER3_BASE = 0x40033000,
    _16_32_BIT_TIMER4_BASE = 0x40034000,
    _16_32_BIT_TIMER5_BASE = 0x40035000,
} gptBaseAddress;

bool gptInit(gptBaseAddress base, gptTimer timer, uint8_t configuration, uint8_t timerMode, uint8_t countDirection);

inline uint32_t gptGetValue(gptBaseAddress base, gptTimer timer)
{
    const gptRegisterOffsets offset = ((timer == GPT_TIMERA) ? GPTMTAV : GPTMTBV);
    return GET_REG32(base, offset);
}

inline void gptSetValue(gptBaseAddress base, gptTimer timer, uint32_t value)
{
    const gptRegisterOffsets offset = ((timer == GPT_TIMERA) ? GPTMTAV : GPTMTBV);
    GET_REG32(base, offset) = value;
}
