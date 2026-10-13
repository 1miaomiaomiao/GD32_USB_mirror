/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-11     kurisaw      first version
 */

#ifndef __DRV_DAC_H__
#define __DRV_DAC_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RT_USING_DAC)

/* gd32 dac driver class */
struct gd32_dac
{
    struct rt_dac_device dac_device;
    uint32_t dac_periph;
    char *name;
    const char *pin_name;
    rt_int16_t channel;
};

#endif

#ifdef __cplusplus
}
#endif

#endif /* __DRV_DAC_H__ */
