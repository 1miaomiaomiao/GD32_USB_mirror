/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-11     kurisaw      first version
 */

#ifndef __DAC_CONFIG_H__
#define __DAC_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSP_USING_DAC0

#ifndef DAC0_CONFIG
#define DAC0_CONFIG                                  \
    {                                                \
        .dac_periph = DAC0,                          \
        .name = "dac0",                              \
        .pin_name = BSP_DAC0_PIN,                    \
        .channel = BSP_DAC0_CHANNEL,                 \
    }
#endif /* DAC0_CONFIG */
#endif /* BSP_USING_DAC0 */

#ifdef BSP_USING_DAC1

#ifndef DAC1_CONFIG
#define DAC1_CONFIG                                  \
    {                                                \
        .dac_periph = DAC0,                          \
        .name = "dac1",                              \
        .pin_name = BSP_DAC1_PIN,                    \
        .channel = BSP_DAC1_CHANNEL,                 \
    }
#endif /* DAC1_CONFIG */
#endif /* BSP_USING_DAC1 */

#ifdef __cplusplus
}
#endif

#endif /*__DAC_CONFIG_H__ */
