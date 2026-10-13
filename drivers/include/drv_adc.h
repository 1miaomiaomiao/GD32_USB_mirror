/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-03     BruceOu      first implementation
 * 2025-10-22     kurisaw           optimize multi-channel GPIO configuration
 */

#ifndef __DRV_ADC_H__
#define __DRV_ADC_H__

#include <rthw.h>
#include <rtthread.h>
#include <board.h>
#include "drv_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_EXTERN_ADC_CHANNEL 20

/* gd32 adc driver class */
struct gd32_adc
{
    uint32_t adc_periph;
    rcu_periph_enum adc_clk;
    const char *adc_pins[MAX_EXTERN_ADC_CHANNEL];
    struct rt_adc_device *adc;
    char *device_name;
};

#ifdef __cplusplus
}
#endif

#endif /* __DRV_ADC_H__ */
