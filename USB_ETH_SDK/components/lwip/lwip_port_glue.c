/* SPDX-License-Identifier: MIT */

/*
 * lwIP OS-port glue that ST's vendor port (../../libs/stm32-mw-lwip/
 * system/OS/sys_arch.c, CMSIS-OS2 based) does not provide: sys_now().
 *
 * sys_now() is a required lwIP port function (see
 * libs/stm32-mw-lwip/src/include/lwip/sys.h) used by lwip core
 * (src/core/timeouts.c) to time its internal timers (ARP, DHCP, TCP
 * retransmit, etc). It must return a monotonically increasing
 * millisecond tick count.
 *
 * HAL_GetTick() is driven by the HAL time base (Core/Src/
 * stm32h5xx_hal_timebase_tim.c, TIM-based, 1 ms resolution) and is
 * safe to call both before the RTOS scheduler starts and from any
 * task afterwards - unlike osKernelGetTickCount(), which requires the
 * kernel to already be running.
 */

#include "lwip/sys.h"
#include "stm32h5xx_hal.h"
#include "logger.h"

u32_t sys_now(void)
{
    return (u32_t)HAL_GetTick();
}

void lwip_diag_log(const char *frmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, frmt);
    vsnprintf(buf, sizeof(buf), frmt, args);
    va_end(args);
    LOGD("lwip", "%s", buf);
}