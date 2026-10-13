/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-22     kurisaw      initial commit
 */

#ifndef __HWTIMER_CONFIG_H__
#define __HWTIMER_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "drv_hwtimer.h"

#ifdef BSP_USING_HWTIMER0
#ifndef HWTIMER0_CONFIG
#define HWTIMER0_CONFIG                                 \
    {                                                   \
        "timer0",                                       \
            {                                           \
                TIMER0,                                 \
                TIMER0_UP_IRQn,                         \
                RCU_TIMER0,                             \
            },                                          \
            { 0 },                                      \
            {                                           \
                10000000, /* max frequency */           \
                1000, /* min frequency */               \
                0xFFFF, /* 16-bit counter */            \
                HWTIMER_CNTMODE_UP, /* count up mode */ \
            },                                          \
    }
#endif /* HWTIMER0_CONFIG */
#endif /* BSP_USING_HWTIMER0 */

#ifdef BSP_USING_HWTIMER1
#ifndef HWTIMER1_CONFIG
#define HWTIMER1_CONFIG                          \
    {                                            \
        "timer1",                                \
            {                                    \
                TIMER1,                          \
                TIMER1_IRQn,                     \
                RCU_TIMER1,                      \
            },                                   \
            { 0 },                               \
            {                                    \
                10000000,                        \
                1000,                            \
                0xFFFFFFFF, /* 32-bit counter */ \
                HWTIMER_CNTMODE_UP,              \
            },                                   \
    }
#endif /* HWTIMER1_CONFIG */
#endif /* BSP_USING_HWTIMER1 */

#ifdef BSP_USING_HWTIMER2
#ifndef HWTIMER2_CONFIG
#define HWTIMER2_CONFIG                      \
    {                                        \
        "timer2",                            \
            {                                \
                TIMER2,                      \
                TIMER2_IRQn,                 \
                RCU_TIMER2,                  \
            },                               \
            { 0 },                           \
            {                                \
                10000000,                    \
                1000,                        \
                0xFFFF, /* 16-bit counter */ \
                HWTIMER_CNTMODE_UP,          \
            },                               \
    }
#endif /* HWTIMER2_CONFIG */
#endif /* BSP_USING_HWTIMER2 */

#ifdef BSP_USING_HWTIMER3
#ifndef HWTIMER3_CONFIG
#define HWTIMER3_CONFIG                      \
    {                                        \
        "timer3",                            \
            {                                \
                TIMER3,                      \
                TIMER3_IRQn,                 \
                RCU_TIMER3,                  \
            },                               \
            { 0 },                           \
            {                                \
                10000000,                    \
                1000,                        \
                0xFFFF, /* 16-bit counter */ \
                HWTIMER_CNTMODE_UP,          \
            },                               \
    }
#endif /* HWTIMER3_CONFIG */
#endif /* BSP_USING_HWTIMER3 */

#ifdef BSP_USING_HWTIMER4
#ifndef HWTIMER4_CONFIG
#define HWTIMER4_CONFIG                          \
    {                                            \
        "timer4",                                \
            {                                    \
                TIMER4,                          \
                TIMER4_IRQn,                     \
                RCU_TIMER4,                      \
            },                                   \
            { 0 },                               \
            {                                    \
                10000000,                        \
                1000,                            \
                0xFFFFFFFF, /* 32-bit counter */ \
                HWTIMER_CNTMODE_UP,              \
            },                                   \
    }
#endif /* HWTIMER4_CONFIG */
#endif /* BSP_USING_HWTIMER4 */

#ifdef BSP_USING_HWTIMER5
#ifndef HWTIMER5_CONFIG
#define HWTIMER5_CONFIG                          \
    {                                            \
        "timer5",                                \
            {                                    \
                TIMER5,                          \
                TIMER5_IRQn,                     \
                RCU_TIMER5,                      \
            },                                   \
            { 0 },                               \
            {                                    \
                10000000,                        \
                1000,                            \
                0xFFFFFFFF, /* 32-bit counter */ \
                HWTIMER_CNTMODE_UP,              \
            },                                   \
    }
#endif /* HWTIMER5_CONFIG */
#endif /* BSP_USING_HWTIMER5 */

#ifdef BSP_USING_HWTIMER6
#ifndef HWTIMER6_CONFIG
#define HWTIMER6_CONFIG                          \
    {                                            \
        "timer6",                                \
            {                                    \
                TIMER6,                          \
                TIMER6_IRQn,                     \
                RCU_TIMER6,                      \
            },                                   \
            { 0 },                               \
            {                                    \
                10000000,                        \
                1000,                            \
                0xFFFFFFFF, /* 32-bit counter */ \
                HWTIMER_CNTMODE_UP,              \
            },                                   \
    }
#endif /* HWTIMER6_CONFIG */
#endif /* BSP_USING_HWTIMER6 */

#ifdef BSP_USING_HWTIMER7
#ifndef HWTIMER7_CONFIG
#define HWTIMER7_CONFIG                      \
    {                                        \
        "timer7",                            \
            {                                \
                TIMER7,                      \
                TIMER7_UP_IRQn,              \
                RCU_TIMER7,                  \
            },                               \
            { 0 },                           \
            {                                \
                10000000,                    \
                1000,                        \
                0xFFFF, /* 16-bit counter */ \
                HWTIMER_CNTMODE_UP,          \
            },                               \
    }
#endif /* HWTIMER7_CONFIG */
#endif /* BSP_USING_HWTIMER7 */

#ifdef __cplusplus
}
#endif

#endif /* __HWTIMER_CONFIG_H__ */
