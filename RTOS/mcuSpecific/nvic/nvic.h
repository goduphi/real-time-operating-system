#pragma once

#include <stdint.h>
#include <mcuSpecific/common_includes/tm4c123gh6pm.h>

void nvicEnableExceptionHandlers(uint32_t exceptionType);

inline void nvicTriggerPendSv(void)
{
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

inline void nvicSystemResetRequest(void)
{
    NVIC_APINT_R = (NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ);
}
