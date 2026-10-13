/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-15     LZerro     compatible with RT-Studio
 * 2025-10-22     kurisaw    optimize multi-channel GPIO configuration
 */

#ifndef __ADC_CONFIG_H__
#define __ADC_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "drv_adc.h"

#ifdef RT_USING_ADC
#define MAX_EXTERN_ADC_CHANNEL 20
#endif

#ifdef BSP_USING_ADC0
#ifdef BSP_ADC0_CH0_PIN
#define ADC0_CH0_PIN BSP_ADC0_CH0_PIN
#else
#define ADC0_CH0_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH1_PIN
#define ADC0_CH1_PIN BSP_ADC0_CH1_PIN
#else
#define ADC0_CH1_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH2_PIN
#define ADC0_CH2_PIN BSP_ADC0_CH2_PIN
#else
#define ADC0_CH2_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH3_PIN
#define ADC0_CH3_PIN BSP_ADC0_CH3_PIN
#else
#define ADC0_CH3_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH4_PIN
#define ADC0_CH4_PIN BSP_ADC0_CH4_PIN
#else
#define ADC0_CH4_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH5_PIN
#define ADC0_CH5_PIN BSP_ADC0_CH5_PIN
#else
#define ADC0_CH5_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH6_PIN
#define ADC0_CH6_PIN BSP_ADC0_CH6_PIN
#else
#define ADC0_CH6_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH7_PIN
#define ADC0_CH7_PIN BSP_ADC0_CH7_PIN
#else
#define ADC0_CH7_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH8_PIN
#define ADC0_CH8_PIN BSP_ADC0_CH8_PIN
#else
#define ADC0_CH8_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH9_PIN
#define ADC0_CH9_PIN BSP_ADC0_CH9_PIN
#else
#define ADC0_CH9_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH10_PIN
#define ADC0_CH10_PIN BSP_ADC0_CH10_PIN
#else
#define ADC0_CH10_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH11_PIN
#define ADC0_CH11_PIN BSP_ADC0_CH11_PIN
#else
#define ADC0_CH11_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH12_PIN
#define ADC0_CH12_PIN BSP_ADC0_CH12_PIN
#else
#define ADC0_CH12_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH13_PIN
#define ADC0_CH13_PIN BSP_ADC0_CH13_PIN
#else
#define ADC0_CH13_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH14_PIN
#define ADC0_CH14_PIN BSP_ADC0_CH14_PIN
#else
#define ADC0_CH14_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH15_PIN
#define ADC0_CH15_PIN BSP_ADC0_CH15_PIN
#else
#define ADC0_CH15_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH16_PIN
#define ADC0_CH16_PIN BSP_ADC0_CH16_PIN
#else
#define ADC0_CH16_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH17_PIN
#define ADC0_CH17_PIN BSP_ADC0_CH17_PIN
#else
#define ADC0_CH17_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH18_PIN
#define ADC0_CH18_PIN BSP_ADC0_CH18_PIN
#else
#define ADC0_CH18_PIN RT_NULL
#endif
#ifdef BSP_ADC0_CH19_PIN
#define ADC0_CH19_PIN BSP_ADC0_CH19_PIN
#else
#define ADC0_CH19_PIN RT_NULL
#endif

#ifndef ADC0_CONFIG
#define ADC0_CONFIG         \
    {                       \
        ADC0,               \
        RCU_ADC0,           \
        {                   \
            ADC0_CH0_PIN,   \
            ADC0_CH1_PIN,   \
            ADC0_CH2_PIN,   \
            ADC0_CH3_PIN,   \
            ADC0_CH4_PIN,   \
            ADC0_CH5_PIN,   \
            ADC0_CH6_PIN,   \
            ADC0_CH7_PIN,   \
            ADC0_CH8_PIN,   \
            ADC0_CH9_PIN,   \
            ADC0_CH10_PIN,  \
            ADC0_CH11_PIN,  \
            ADC0_CH12_PIN,  \
            ADC0_CH13_PIN,  \
            ADC0_CH14_PIN,  \
            ADC0_CH15_PIN,  \
            ADC0_CH16_PIN,  \
            ADC0_CH17_PIN,  \
            ADC0_CH18_PIN,  \
            ADC0_CH19_PIN,  \
        },                  \
        &adc0,              \
        "adc0",             \
    }
#endif /* ADC0_CONFIG */
#endif /* BSP_USING_ADC0 */

#ifdef BSP_USING_ADC1
#ifdef BSP_ADC1_CH0_PIN
#define ADC1_CH0_PIN BSP_ADC1_CH0_PIN
#else
#define ADC1_CH0_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH1_PIN
#define ADC1_CH1_PIN BSP_ADC1_CH1_PIN
#else
#define ADC1_CH1_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH2_PIN
#define ADC1_CH2_PIN BSP_ADC1_CH2_PIN
#else
#define ADC1_CH2_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH3_PIN
#define ADC1_CH3_PIN BSP_ADC1_CH3_PIN
#else
#define ADC1_CH3_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH4_PIN
#define ADC1_CH4_PIN BSP_ADC1_CH4_PIN
#else
#define ADC1_CH4_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH5_PIN
#define ADC1_CH5_PIN BSP_ADC1_CH5_PIN
#else
#define ADC1_CH5_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH6_PIN
#define ADC1_CH6_PIN BSP_ADC1_CH6_PIN
#else
#define ADC1_CH6_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH7_PIN
#define ADC1_CH7_PIN BSP_ADC1_CH7_PIN
#else
#define ADC1_CH7_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH8_PIN
#define ADC1_CH8_PIN BSP_ADC1_CH8_PIN
#else
#define ADC1_CH8_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH9_PIN
#define ADC1_CH9_PIN BSP_ADC1_CH9_PIN
#else
#define ADC1_CH9_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH10_PIN
#define ADC1_CH10_PIN BSP_ADC1_CH10_PIN
#else
#define ADC1_CH10_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH11_PIN
#define ADC1_CH11_PIN BSP_ADC1_CH11_PIN
#else
#define ADC1_CH11_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH12_PIN
#define ADC1_CH12_PIN BSP_ADC1_CH12_PIN
#else
#define ADC1_CH12_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH13_PIN
#define ADC1_CH13_PIN BSP_ADC1_CH13_PIN
#else
#define ADC1_CH13_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH14_PIN
#define ADC1_CH14_PIN BSP_ADC1_CH14_PIN
#else
#define ADC1_CH14_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH15_PIN
#define ADC1_CH15_PIN BSP_ADC1_CH15_PIN
#else
#define ADC1_CH15_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH16_PIN
#define ADC1_CH16_PIN BSP_ADC1_CH16_PIN
#else
#define ADC1_CH16_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH17_PIN
#define ADC1_CH17_PIN BSP_ADC1_CH17_PIN
#else
#define ADC1_CH17_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH18_PIN
#define ADC1_CH18_PIN BSP_ADC1_CH18_PIN
#else
#define ADC1_CH18_PIN RT_NULL
#endif
#ifdef BSP_ADC1_CH19_PIN
#define ADC1_CH19_PIN BSP_ADC1_CH19_PIN
#else
#define ADC1_CH19_PIN RT_NULL
#endif

#ifndef ADC1_CONFIG
#define ADC1_CONFIG         \
    {                       \
        ADC1,               \
        RCU_ADC1,           \
        {                   \
            ADC1_CH0_PIN,   \
            ADC1_CH1_PIN,   \
            ADC1_CH2_PIN,   \
            ADC1_CH3_PIN,   \
            ADC1_CH4_PIN,   \
            ADC1_CH5_PIN,   \
            ADC1_CH6_PIN,   \
            ADC1_CH7_PIN,   \
            ADC1_CH8_PIN,   \
            ADC1_CH9_PIN,   \
            ADC1_CH10_PIN,  \
            ADC1_CH11_PIN,  \
            ADC1_CH12_PIN,  \
            ADC1_CH13_PIN,  \
            ADC1_CH14_PIN,  \
            ADC1_CH15_PIN,  \
            ADC1_CH16_PIN,  \
            ADC1_CH17_PIN,  \
            ADC1_CH18_PIN,  \
            ADC1_CH19_PIN,  \
        },                  \
        &adc1,              \
        "adc1",             \
    }
#endif /* ADC1_CONFIG */
#endif /* BSP_USING_ADC1 */

#ifdef BSP_USING_ADC2
#ifdef BSP_ADC2_CH0_PIN
#define ADC2_CH0_PIN BSP_ADC2_CH0_PIN
#else
#define ADC2_CH0_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH1_PIN
#define ADC2_CH1_PIN BSP_ADC2_CH1_PIN
#else
#define ADC2_CH1_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH2_PIN
#define ADC2_CH2_PIN BSP_ADC2_CH2_PIN
#else
#define ADC2_CH2_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH3_PIN
#define ADC2_CH3_PIN BSP_ADC2_CH3_PIN
#else
#define ADC2_CH3_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH4_PIN
#define ADC2_CH4_PIN BSP_ADC2_CH4_PIN
#else
#define ADC2_CH4_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH5_PIN
#define ADC2_CH5_PIN BSP_ADC2_CH5_PIN
#else
#define ADC2_CH5_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH6_PIN
#define ADC2_CH6_PIN BSP_ADC2_CH6_PIN
#else
#define ADC2_CH6_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH7_PIN
#define ADC2_CH7_PIN BSP_ADC2_CH7_PIN
#else
#define ADC2_CH7_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH8_PIN
#define ADC2_CH8_PIN BSP_ADC2_CH8_PIN
#else
#define ADC2_CH8_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH9_PIN
#define ADC2_CH9_PIN BSP_ADC2_CH9_PIN
#else
#define ADC2_CH9_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH10_PIN
#define ADC2_CH10_PIN BSP_ADC2_CH10_PIN
#else
#define ADC2_CH10_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH11_PIN
#define ADC2_CH11_PIN BSP_ADC2_CH11_PIN
#else
#define ADC2_CH11_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH12_PIN
#define ADC2_CH12_PIN BSP_ADC2_CH12_PIN
#else
#define ADC2_CH12_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH13_PIN
#define ADC2_CH13_PIN BSP_ADC2_CH13_PIN
#else
#define ADC2_CH13_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH14_PIN
#define ADC2_CH14_PIN BSP_ADC2_CH14_PIN
#else
#define ADC2_CH14_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH15_PIN
#define ADC2_CH15_PIN BSP_ADC2_CH15_PIN
#else
#define ADC2_CH15_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH16_PIN
#define ADC2_CH16_PIN BSP_ADC2_CH16_PIN
#else
#define ADC2_CH16_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH17_PIN
#define ADC2_CH17_PIN BSP_ADC2_CH17_PIN
#else
#define ADC2_CH17_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH18_PIN
#define ADC2_CH18_PIN BSP_ADC2_CH18_PIN
#else
#define ADC2_CH18_PIN RT_NULL
#endif
#ifdef BSP_ADC2_CH19_PIN
#define ADC2_CH19_PIN BSP_ADC2_CH19_PIN
#else
#define ADC2_CH19_PIN RT_NULL
#endif

#ifndef ADC2_CONFIG
#define ADC2_CONFIG         \
    {                       \
        ADC2,               \
        RCU_ADC2,           \
        {                   \
            ADC2_CH0_PIN,   \
            ADC2_CH1_PIN,   \
            ADC2_CH2_PIN,   \
            ADC2_CH3_PIN,   \
            ADC2_CH4_PIN,   \
            ADC2_CH5_PIN,   \
            ADC2_CH6_PIN,   \
            ADC2_CH7_PIN,   \
            ADC2_CH8_PIN,   \
            ADC2_CH9_PIN,   \
            ADC2_CH10_PIN,  \
            ADC2_CH11_PIN,  \
            ADC2_CH12_PIN,  \
            ADC2_CH13_PIN,  \
            ADC2_CH14_PIN,  \
            ADC2_CH15_PIN,  \
            ADC2_CH16_PIN,  \
            ADC2_CH17_PIN,  \
            ADC2_CH18_PIN,  \
            ADC2_CH19_PIN,  \
        },                  \
        &adc2,              \
        "adc2",             \
    }
#endif /* ADC2_CONFIG */
#endif /* BSP_USING_ADC2 */

#ifdef __cplusplus
}
#endif

#endif // __ADC_CONFIG_H__
