/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#ifndef __DRV_CAN_H__
#define __DRV_CAN_H__

#include <rtdevice.h>
#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RT_USING_CAN
/* Aliases for __IRQn */
#if defined(BSP_USING_CAN0) || defined(BSP_USING_CAN1) || defined(BSP_USING_CAN2)
/* BSP_USING_CAN0 */
#define CAN0_TX_IRQn    CAN0_Message_IRQn
#define CAN0_RX_IRQn    CAN0_Message_IRQn
#define CAN0_ERROR_IRQn CAN0_Error_IRQn
/* BSP_USING_CAN1 */
#define CAN1_TX_IRQn    CAN1_Message_IRQn
#define CAN1_RX_IRQn    CAN1_Message_IRQn
#define CAN1_ERROR_IRQn CAN1_Error_IRQn
/* BSP_USING_CAN2 */
#define CAN2_TX_IRQn    CAN2_Message_IRQn
#define CAN2_RX_IRQn    CAN2_Message_IRQn
#define CAN2_ERROR_IRQn CAN2_Error_IRQn
#endif

struct gd32_baud_rate_tab
{
    uint32_t baud_rate;
    uint8_t resync_jump_width;
    uint8_t prop_time_segment;
    uint8_t time_segment_1;
    uint8_t time_segment_2;
    uint32_t prescaler;
};
#ifdef RT_CAN_USING_CANFD

typedef struct
{
    rt_uint32_t baud;
    rt_uint8_t sjw;
    rt_uint8_t prop;
    rt_uint8_t seg1;
    rt_uint8_t seg2;
    rt_uint16_t prescaler;
} gd32_fd_data_baud_t;

enum CANFD_BAUD
{
    CANFD_5MBaud = 5000UL * 1000,    /* 5 MBit/sec   */
    CANFD_4MBaud = 4000UL * 1000,    /* 4 MBit/sec   */
    CANFD_3MBaud = 3000UL * 1000,    /* 3 MBit/sec   */
    CANFD_2MBaud = 2000UL * 1000,    /* 2 MBit/sec   */
    CANFD_1MBaud = 1000UL * 1000,    /* 1 MBit/sec   */
    CANFD_800kBaud = 1000UL * 800,     /* 800 kBit/sec */
    CANFD_500kBaud = 1000UL * 500,     /* 500 kBit/sec */
    CANFD_250kBaud = 1000UL * 250,     /* 250 kBit/sec */
    CANFD_125kBaud = 1000UL * 125,     /* 125 kBit/sec */
    CANFD_100kBaud = 1000UL * 100,     /* 100 kBit/sec */
    CANFD_50kBaud = 1000UL * 50,      /* 50 kBit/sec  */
    CANFD_20kBaud = 1000UL * 20,      /* 20 kBit/sec  */
    CANFD_10kBaud = 1000UL * 10       /* 10 kBit/sec  */
};
#endif

/* gd32 can device */
typedef struct
{
    char *name;
    const char *tx_pin_name;        /* can pin name */
    const char *rx_pin_name;        /* rx pin name */
    const char *alternate;
    uint32_t can_periph;
    can_parameter_struct can_init;
    can_fifo_parameter_struct can_fifo;
    can_operation_modes_enum mode;
    can_mailbox_descriptor_struct tx_message[16];
    can_mailbox_descriptor_struct rx_message[16];
    rt_uint8_t rx_message_valid[16];
    void (*filter_cb)(uint32_t can_periph);
#ifdef RT_CAN_USING_CANFD
    can_fd_parameter_struct can_fd_init;
    rt_uint8_t fd_enabled;
    rt_uint8_t fd_rx_buf[16][64];
#endif
    struct rt_can_device *device; /* can device */
} gd32_can_struct;

void can_gpio_config(void);
int rt_hw_can_init(void);
#endif /* RT_USING_CAN */
#ifdef __cplusplus
}
#endif

#endif /* __DRV_CAN_H__ */

