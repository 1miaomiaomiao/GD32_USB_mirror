/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-22     WangShun     compatible with RT-Studio
 */

#ifndef __CAN_CONFIG_H__
#define __CAN_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_USING_CAN0)
#ifndef BSP_CAN0_RX_PIN
#define BSP_CAN0_RX_PIN "PD0"
#define BSP_CAN0_TX_PIN "PD1"
#define BSP_CAN0_AFIO   "AF9"
#endif /* BSP_CAN0_RX_PIN */
#ifndef CAN0_CONFIG
#define CAN0_CONFIG                     \
    {                                   \
        .name = "can0",                 \
        .can_periph = CAN0,             \
        .tx_pin_name = BSP_CAN0_TX_PIN, \
        .rx_pin_name = BSP_CAN0_RX_PIN, \
        .alternate = BSP_CAN0_AFIO,     \
        .device = &dev_can0,            \
    }
#endif /* CAN0_CONFIG */
#endif /* BSP_USING_CAN0 */

#if defined(BSP_USING_CAN1)
#ifndef BSP_CAN1_RX_PIN
#define BSP_CAN1_RX_PIN "PB5"
#define BSP_CAN1_TX_PIN "PB6"
#define BSP_CAN1_AFIO   "AF9"
#endif /* BSP_CAN0_RX_PIN */
#ifndef CAN1_CONFIG
#define CAN1_CONFIG                     \
    {                                   \
        .name = "can1",                 \
        .can_periph = CAN1,             \
        .tx_pin_name = BSP_CAN1_TX_PIN, \
        .rx_pin_name = BSP_CAN1_RX_PIN, \
        .alternate = BSP_CAN1_AFIO,     \
        .device = &dev_can1,            \
    }
#endif /* CAN1_CONFIG */
#endif /* BSP_USING_CAN1 */

#if defined(BSP_USING_CAN2)
#ifndef BSP_CAN2_RX_PIN
#define BSP_CAN2_RX_PIN "PD12"
#define BSP_CAN2_TX_PIN "PD13"
#define BSP_CAN2_AFIO   "AF5"
#endif /* BSP_CAN2_RX_PIN */
#ifndef CAN2_CONFIG
#define CAN2_CONFIG                     \
    {                                   \
        .name = "can2",                 \
        .can_periph = CAN2,             \
        .tx_pin_name = BSP_CAN2_TX_PIN, \
        .rx_pin_name = BSP_CAN2_RX_PIN, \
        .alternate = BSP_CAN2_AFIO,     \
        .device = &dev_can2,            \
    }
#endif /* CAN2_CONFIG */
#endif /* BSP_USING_CAN2 */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_CONFIG_H__ */

