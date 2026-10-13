/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-11     kurisaw      compatible with RT-Studio
 */

#ifndef __PWM_CONFIG_H__
#define __PWM_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSP_USING_PWM0

#ifndef PWM0_CONFIG
#define PWM0_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER0,                      \
        .name = "pwm0",                              \
        .timer_clk = RCU_TIMER0,                     \
        .pin_name = BSP_PWM0_PIN,                    \
        .channel = BSP_PWM0_CHANNEL,                 \
        .alternate = BSP_PWM0_AFIO,                  \
    }
#endif /* PWM0_CONFIG */
#endif /* BSP_USING_PWM0 */

#ifdef BSP_USING_PWM1

#ifndef PWM1_CONFIG
#define PWM1_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER1,                      \
        .name = "pwm1",                              \
        .timer_clk = RCU_TIMER1,                     \
        .pin_name = BSP_PWM1_PIN,                    \
        .channel = BSP_PWM1_CHANNEL,                 \
        .alternate = BSP_PWM1_AFIO,                  \
    }
#endif /* PWM1_CONFIG */
#endif /* BSP_USING_PWM1 */

#ifdef BSP_USING_PWM2

#ifndef PWM2_CONFIG
#define PWM2_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER2,                      \
        .name = "pwm2",                              \
        .timer_clk = RCU_TIMER2,                     \
        .pin_name = BSP_PWM2_PIN,                    \
        .channel = BSP_PWM2_CHANNEL,                 \
        .alternate = BSP_PWM2_AFIO,                  \
    }
#endif /* PWM2_CONFIG */
#endif /* BSP_USING_PWM2 */

#ifdef BSP_USING_PWM3

#ifndef PWM3_CONFIG
#define PWM3_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER3,                      \
        .name = "pwm3",                              \
        .timer_clk = RCU_TIMER3,                     \
        .pin_name = BSP_PWM3_PIN,                    \
        .channel = BSP_PWM3_CHANNEL,                 \
        .alternate = BSP_PWM3_AFIO,                  \
    }
#endif /* PWM3_CONFIG */
#endif /* BSP_USING_PWM3 */

#ifdef BSP_USING_PWM4

#ifndef PWM4_CONFIG
#define PWM4_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER4,                      \
        .name = "pwm4",                              \
        .timer_clk = RCU_TIMER4,                     \
        .pin_name = BSP_PWM4_PIN,                    \
        .channel = BSP_PWM4_CHANNEL,                 \
        .alternate = BSP_PWM4_AFIO,                  \
    }
#endif /* PWM4_CONFIG */
#endif /* BSP_USING_PWM4 */

#ifdef BSP_USING_PWM5

#ifndef PWM5_CONFIG
#define PWM5_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER5,                      \
        .name = "pwm5",                              \
        .timer_clk = RCU_TIMER5,                     \
        .pin_name = BSP_PWM5_PIN,                    \
        .channel = BSP_PWM5_CHANNEL,                 \
        .alternate = BSP_PWM5_AFIO,                  \
    }
#endif /* PWM5_CONFIG */
#endif /* BSP_USING_PWM5 */

#ifdef BSP_USING_PWM6

#ifndef PWM6_CONFIG
#define PWM6_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER6,                      \
        .name = "pwm6",                              \
        .timer_clk = RCU_TIMER6,                     \
        .pin_name = BSP_PWM6_PIN,                    \
        .channel = BSP_PWM6_CHANNEL,                 \
        .alternate = BSP_PWM6_AFIO,                  \
    }
#endif /* PWM6_CONFIG */
#endif /* BSP_USING_PWM6 */

#ifdef BSP_USING_PWM7

#ifndef PWM7_CONFIG
#define PWM7_CONFIG                                  \
    {                                                \
        .timer_periph = TIMER7,                      \
        .name = "pwm7",                              \
        .timer_clk = RCU_TIMER7,                     \
        .pin_name = BSP_PWM7_PIN,                    \
        .channel = BSP_PWM7_CHANNEL,                 \
        .alternate = BSP_PWM7_AFIO,                  \
    }
#endif /* PWM7_CONFIG */
#endif /* BSP_USING_PWM7 */

#ifdef __cplusplus
}
#endif

#endif /*__PWM_CONFIG_H__ */
