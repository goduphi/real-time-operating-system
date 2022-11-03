#pragma once

typedef enum _gptRegisterOffsets
{
    GPTMCFG  = 0x00,
    GPTMTAMR = 0x04,
    GPTMTBMR = 0x08,
    GPTMCTL  = 0x0C,
    GPTMTAV  = 0x50,
    GPTMTBV  = 0x54
} gptRegisterOffsets;
