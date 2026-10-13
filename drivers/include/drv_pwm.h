/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2025-10-11     kurisaw           compatible with RT-Studio
 */

#ifndef __DRV_PWM_H__
#define __DRV_PWM_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RT_USING_PWM)

/* gd32 pwm driver class */
struct gd32_pwm
{
    struct rt_device_pwm pwm_device;
    uint32_t timer_periph;
    char *name;
    rcu_periph_enum timer_clk;
    const char *pin_name;
    rt_int16_t channel;
    const char *alternate;
};

rt_err_t rt_hw_pwm_init(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __DRV_PWM_H__ */
