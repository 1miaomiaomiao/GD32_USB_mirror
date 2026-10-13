/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author          Notes
 * 2024-12-30     qingsong.yin    first version
 * 2024-03-07     qingsong.yin    for CAN20kBaud/CAN10kBaud
 * 2024-09-09     qingsong.yin    for CANFD and DMA
 * 2025-10-22     WangShun        compatible with RT-Studio
 */

#include <stdlib.h>
#include "drv_can.h"
#include "can_config.h"

#ifdef RT_USING_CAN
#if defined(BSP_USING_CAN0) || defined(BSP_USING_CAN1) || defined(BSP_USING_CAN2)

#define LOG_TAG "drv_can"
#include <drv_log.h>

#ifdef BSP_USING_CAN0
struct rt_can_device dev_can0;
#endif
#ifdef BSP_USING_CAN1
struct rt_can_device dev_can1;
#endif
#ifdef BSP_USING_CAN2
struct rt_can_device dev_can2;
#endif

static gd32_can_struct can_obj[] = {
#ifdef BSP_USING_CAN0
    CAN0_CONFIG,
#endif
#ifdef BSP_USING_CAN1
    CAN1_CONFIG,
#endif
#ifdef BSP_USING_CAN2
    CAN2_CONFIG,
#endif
};

/* baud calculation example */
#if defined(SOC_SERIES_GD32H75E) || defined(SOC_SERIES_GD32H7xx)  /* APB2 300MHz(max) */
static const struct gd32_baud_rate_tab can_baud_rate_tab[] = {
    /* baud,     sjw, prop, seg1, seg2, prescaler */
    { CAN1MBaud, 1, 20, 27, 12, 5 },
    { CAN800kBaud, 1, 2, 9, 3, 25 },
    { CAN500kBaud, 1, 2, 5, 2, 60 },
    { CAN250kBaud, 1, 2, 5, 2, 120 },
    { CAN125kBaud, 1, 2, 5, 2, 240 },
    { CAN100kBaud, 1, 2, 5, 2, 300 },
    { CAN50kBaud, 1, 2, 5, 2, 600 },
    { CAN20kBaud, 1, 14, 25, 10, 300 },
    { CAN10kBaud, 1, 14, 25, 10, 600 },
};

#ifdef RT_CAN_USING_CANFD
static const gd32_fd_data_baud_t can_fd_data_baud_tab[] = {
    /* baud,          sjw, prop, seg1, seg2, prescaler */
    { CANFD_5MBaud, 1, 2, 7, 2, 5 },   /* 5Mbps */
    { CANFD_4MBaud, 1, 3, 8, 3, 5 },   /* 4Mbps */
    { CANFD_3MBaud, 1, 4, 11, 4, 5 },   /* 3Mbps */
    { CANFD_2MBaud, 1, 6, 17, 6, 5 },   /* 2Mbps */
    { CANFD_1MBaud, 1, 20, 27, 12, 5 },   /* 1Mbps: Total Tq=60, SamplePointâ‰ˆ80% */
    { CANFD_800kBaud, 1, 2, 9, 3, 25 },  /* 800kbps */
    { CANFD_500kBaud, 1, 2, 5, 2, 60 },  /* 500kbps */
    { CANFD_250kBaud, 1, 2, 5, 2, 120 }, /* 250kbps */
    { CANFD_125kBaud, 1, 2, 5, 2, 240 }, /* 125kbps */
    { CANFD_100kBaud, 1, 2, 5, 2, 300 }, /* 100kbps */
    { CANFD_50kBaud, 1, 2, 5, 2, 600 }, /* 50kbps */
    { CANFD_20kBaud, 1, 14, 25, 10, 300 }, /* 20kbps */
    { CANFD_10kBaud, 1, 14, 25, 10, 600 }, /* 10kbps */
};
#endif
#endif

static rt_uint32_t get_can_baud_index(rt_uint32_t baud)
{
    rt_uint32_t len, index;

    len = sizeof(can_baud_rate_tab) / sizeof(can_baud_rate_tab[0]);
    for (index = 0; index < len; index++)
    {
        if (can_baud_rate_tab[index].baud_rate == baud)
        {
            return index;
        }
    }
    return 0; /* default baud is CAN1MBaud */
}

rt_weak void can_gpio_config(void)
{
    int i;
    rt_uint32_t tx_port, rx_port;
    rt_uint32_t tx_pin, rx_pin;
    rt_uint32_t pin_af;
    rcu_periph_enum tx_periph, rx_periph;

#ifdef BSP_USING_CAN0
    rcu_can_clock_config(IDX_CAN0, RCU_CANSRC_APB2);
    rcu_periph_clock_enable(RCU_CAN0);
#endif

#ifdef BSP_USING_CAN1
    rcu_can_clock_config(IDX_CAN1, RCU_CANSRC_APB2);
    rcu_periph_clock_enable(RCU_CAN1);
#endif

#ifdef BSP_USING_CAN2
    rcu_can_clock_config(IDX_CAN2, RCU_CANSRC_APB2);
    rcu_periph_clock_enable(RCU_CAN2);
#endif

    for (i = 0; i < sizeof(can_obj) / sizeof(can_obj[0]); i++)
    {
        get_pin_config(can_obj[i].tx_pin_name, &tx_port, &tx_pin, &tx_periph);
        get_pin_config(can_obj[i].rx_pin_name, &rx_port, &rx_pin, &rx_periph);
        pin_alternate_config(can_obj[i].alternate, &pin_af);

        rcu_periph_clock_enable(tx_periph);
        rcu_periph_clock_enable(rx_periph);

        gpio_output_options_set(tx_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, tx_pin);
        gpio_mode_set(tx_port, GPIO_MODE_AF, GPIO_PUPD_NONE, tx_pin);
        gpio_af_set(tx_port, pin_af, tx_pin);

        gpio_output_options_set(rx_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, rx_pin);
        gpio_mode_set(rx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, rx_pin);
        gpio_af_set(rx_port, pin_af, rx_pin);
    }
}

static rt_err_t gd32_can_configure(struct rt_can_device *can, struct can_configure *cfg)
{
    gd32_can_struct *drv_can = (gd32_can_struct *)can->parent.user_data;
    rt_uint32_t baud_index;

    RT_ASSERT(can);
    RT_ASSERT(cfg);
    RT_ASSERT(drv_can);

    /* init can gpio and enable can clock */
    can_gpio_config();

    /* initialize CAN register */
    can_deinit(drv_can->can_periph);
    /* initialize CAN */
    can_struct_para_init(CAN_INIT_STRUCT, &drv_can->can_init);
    /* initialize CAN parameters */
    drv_can->can_init.internal_counter_source = CAN_TIMER_SOURCE_BIT_CLOCK;
    drv_can->can_init.self_reception = DISABLE;
    drv_can->can_init.mb_tx_order = CAN_TX_HIGH_PRIORITY_MB_FIRST;
    drv_can->can_init.mb_tx_abort_enable = ENABLE;
    drv_can->can_init.local_priority_enable = DISABLE;
    drv_can->can_init.mb_rx_ide_rtr_type = CAN_IDE_RTR_FILTERED;
    drv_can->can_init.mb_remote_frame = CAN_STORE_REMOTE_REQUEST_FRAME;
    drv_can->can_init.rx_private_filter_queue_enable = ENABLE;
    drv_can->can_init.edge_filter_enable = DISABLE;
    drv_can->can_init.protocol_exception_enable = DISABLE;
    drv_can->can_init.rx_filter_order = CAN_RX_FILTER_ORDER_MAILBOX_FIRST;
    drv_can->can_init.memory_size = CAN_MEMSIZE_32_UNIT;
    /* filter configuration */
    drv_can->can_init.mb_public_filter = 0U;
    /* can baud config */
    baud_index = get_can_baud_index(cfg->baud_rate);
    drv_can->can_init.resync_jump_width = can_baud_rate_tab[baud_index].resync_jump_width;
    drv_can->can_init.prop_time_segment = can_baud_rate_tab[baud_index].prop_time_segment;
    drv_can->can_init.time_segment_1 = can_baud_rate_tab[baud_index].time_segment_1;
    drv_can->can_init.time_segment_2 = can_baud_rate_tab[baud_index].time_segment_2;
    drv_can->can_init.prescaler = can_baud_rate_tab[baud_index].prescaler;

    if (can_init(drv_can->can_periph, &drv_can->can_init) != SUCCESS)
    {
        LOG_E("can init error");
        return RT_ERROR;
    }

#ifdef RT_CAN_USING_CANFD
    /* Configure CANFD if enabled in user config */
    drv_can->fd_enabled = 0;
    can_struct_para_init(CAN_FD_INIT_STRUCT, &drv_can->can_fd_init);
    if (cfg->enable_canfd)
    {
        /* default: enable bitrate switch and ISO mode */
        drv_can->can_fd_init.bitrate_switch_enable = ENABLE;
        drv_can->can_fd_init.iso_can_fd_enable = ENABLE;
        /* select mailbox data size according to arbitration len requirement; default 64 bytes */
        drv_can->can_fd_init.mailbox_data_size = CAN_MAILBOX_DATA_SIZE_8_BYTES;
        drv_can->can_fd_init.tdc_enable = DISABLE;
        drv_can->can_fd_init.tdc_offset = 0U;
        if (cfg->use_bit_timing)
        {
            /* Arbitration timing has been already applied by classic init; now apply data phase */
            drv_can->can_fd_init.resync_jump_width = cfg->canfd_timing.num_sjw;
            drv_can->can_fd_init.prop_time_segment = cfg->canfd_timing.num_sspoff; /* heuristic if library expects prop vs seg1 */
            drv_can->can_fd_init.time_segment_1 = cfg->canfd_timing.num_seg1;
            drv_can->can_fd_init.time_segment_2 = cfg->canfd_timing.num_seg2;
            drv_can->can_fd_init.prescaler = cfg->canfd_timing.prescaler;
        }
        else if (cfg->baud_rate_fd)
        {
            /* Simple reuse arbitration timing table for data speed if user sets baud_rate_fd as multiple (rough placeholder) */
            /* NOTE: For precise FD data phase user should set use_bit_timing */
            drv_can->can_fd_init.resync_jump_width = 1;
            drv_can->can_fd_init.prop_time_segment = 2;
            drv_can->can_fd_init.time_segment_1 = 5;
            drv_can->can_fd_init.time_segment_2 = 2;
            drv_can->can_fd_init.prescaler = 15; /* placeholder from example */
        }
        can_fd_config(drv_can->can_periph, &drv_can->can_fd_init);
        drv_can->fd_enabled = 1;
    }
#else
#ifdef RT_CAN_USING_HDR
    /* Rxfifo configuration */
    drv_can->can_fifo.dma_enable = DISABLE;
    drv_can->can_fifo.filter_format_and_number = CAN_RXFIFO_FILTER_A_NUM_40;
    drv_can->can_fifo.fifo_public_filter = 0;
    can_rx_fifo_config(drv_can->can_periph, &drv_can->can_fifo);
#endif
#endif /* RT_CAN_USING_CANFD */
    for (uint8_t i = 0; i < 16; i++)
    {
        can_struct_para_init(CAN_MDSC_STRUCT, &drv_can->rx_message[i]);
        drv_can->rx_message[i].rtr = 0U;
        drv_can->rx_message[i].ide = 0U;
        drv_can->rx_message[i].code = CAN_MB_RX_STATUS_EMPTY;
        /* rx mailbox */
        drv_can->rx_message[i].id = 0x55U;
#if defined(SOC_SERIES_GD32H7xx)
        drv_can->rx_message[i].data = (uint32_t *)(drv_can->fd_rx_buf[i]);
#elif defined(SOC_SERIES_GD32H75E)
        memcpy(drv_can->rx_message[i].data, drv_can->fd_rx_buf[i], sizeof(drv_can->rx_message[i].data));
#endif
        can_mailbox_config(drv_can->can_periph, i, &drv_can->rx_message[i]);
    }

    /* clear msg16-31 for tx */
    CAN_STAT(drv_can->can_periph) = BITS(16, 31);

    /* can mode config */
    switch (cfg->mode)
    {
    case RT_CAN_MODE_NORMAL:
        drv_can->mode = CAN_NORMAL_MODE;
        break;
    case RT_CAN_MODE_LISTEN:
        drv_can->mode = CAN_MONITOR_MODE;
        break;
    case RT_CAN_MODE_LOOPBACK:
        drv_can->mode = CAN_LOOPBACK_SILENT_MODE;
        break;
    case RT_CAN_MODE_LOOPBACKANLISTEN:
        drv_can->mode = CAN_LOOPBACK_SILENT_MODE;
        break;
    default:
        drv_can->mode = CAN_NORMAL_MODE;
        break;
    }
    can_operation_mode_enter(drv_can->can_periph, drv_can->mode);
    return RT_EOK;
}

static rt_err_t gd32_can_control(struct rt_can_device *can, int cmd, void *arg)
{
    rt_uint32_t baud_index;
    rt_uint32_t argval;
    gd32_can_struct *drv_can = (gd32_can_struct *)can->parent.user_data;

    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(drv_can != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        argval = (rt_uint32_t)arg;
        if (argval == RT_DEVICE_FLAG_INT_RX)
        {
            CAN_INTEN(drv_can->can_periph) &= ~BITS(0, 15);
            if (CAN0 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN0_RX_IRQn);
            }
            else if (CAN1 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN1_RX_IRQn);
            }
            else if (CAN2 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN2_RX_IRQn);
            }
            else
            {
                /* not reach */
            }
        }
        else if (argval == RT_DEVICE_FLAG_INT_TX)
        {    //int tx
            CAN_INTEN(drv_can->can_periph) &= ~BITS(15, 31);
            if (CAN0 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN0_TX_IRQn);
            }
            else if (CAN1 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN1_TX_IRQn);
            }
            else if (CAN2 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN2_TX_IRQn);
            }
            else
            {
                /* not reach */
            }
        }
        else if (argval == RT_DEVICE_CAN_INT_ERR)
        {    //int error
            can_interrupt_disable(drv_can->can_periph, CAN_INT_ERR_SUMMARY);
            if (CAN0 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN0_Error_IRQn);
            }
            else if (CAN1 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN1_Error_IRQn);
            }
            else if (CAN2 == drv_can->can_periph)
            {
                nvic_irq_disable(CAN2_Error_IRQn);
            }
            else
            {
                /* not reach */
            }
        }
        break;
    case RT_DEVICE_CTRL_SET_INT:
        argval = (rt_uint32_t)arg;
        if (argval == RT_DEVICE_FLAG_INT_RX)
        {
            CAN_INTEN(drv_can->can_periph) |= BITS(0, 15);

            if (CAN0 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN0_RX_IRQn, 15, 0);
            }
            else if (CAN1 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN1_RX_IRQn, 15, 0);
            }
            else if (CAN2 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN2_RX_IRQn, 15, 0);
            }
            else
            {
                /* not reach */
            }
        }
        else if (argval == RT_DEVICE_FLAG_INT_TX)
        {
            CAN_INTEN(drv_can->can_periph) |= BITS(16, 31);
            if (CAN0 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN0_TX_IRQn, 15, 0);
            }
            else if (CAN1 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN1_TX_IRQn, 15, 0);
            }
            else if (CAN2 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN2_TX_IRQn, 15, 0);
            }
            else
            {
                /* not reach */
            }
        }
        else if (argval == RT_DEVICE_CAN_INT_ERR)
        {
            can_interrupt_enable(drv_can->can_periph, CAN_INT_ERR_SUMMARY);
            if (CAN0 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN0_ERROR_IRQn, 15, 0);
            }
            else if (CAN1 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN1_ERROR_IRQn, 15, 0);
            }
            else if (CAN2 == drv_can->can_periph)
            {
                nvic_irq_enable(CAN2_ERROR_IRQn, 15, 0);
            }
            else
            {
                /* not reach */
            }
        }
        break;
    case RT_CAN_CMD_SET_FILTER:
    {
        drv_can->filter_cb = (void (*)(uint32_t can_periph))arg;
        if (drv_can->filter_cb)
        {
            drv_can->filter_cb(drv_can->can_periph);
        }
#ifdef RT_CAN_USING_HDR
        struct rt_can_filter_config *filter_cfg;

        filter_cfg = (struct rt_can_filter_config *)arg;
        can_operation_mode_enter(drv_can->can_periph, CAN_INACTIVE_MODE);
        CAN_RFIFOPUBF(drv_can->can_periph) = 0xFFFFFFFF;
        for (i = 0; i < 16; i++)
        {
            can_private_filter_config(drv_can->can_periph, i, 0xFFFFFFFF);
        }

        for (i = 0; i < filter_cfg->count; i++)
        {
            if (filter_cfg->items[i].hdr_bank == -1)
            {
                can_private_filter_config(drv_can->can_periph, i, filter_cfg->items[i].mask);
                drv_can->fifo_element[i].extended_frame = filter_cfg->items[i].ide;
                drv_can->fifo_element[i].remote_frame = filter_cfg->items[i].rtr;
                drv_can->fifo_element[i].id = filter_cfg->items[i].id;
            }
            else
            {
                can_private_filter_config(drv_can->can_periph, filter_cfg->items[i].hdr_bank, filter_cfg->items[i].mask);
                drv_can->fifo_element[filter_cfg->items[i].hdr_bank].extended_frame = filter_cfg->items[i].ide;
                drv_can->fifo_element[filter_cfg->items[i].hdr_bank].remote_frame = filter_cfg->items[i].rtr;
                drv_can->fifo_element[filter_cfg->items[i].hdr_bank].id = filter_cfg->items[i].id;
            }
        }
        can_rx_fifo_filter_table_config(drv_can->can_periph, drv_can->fifo_element);
        can_rx_fifo_clear(drv_can->can_periph);
        can_operation_mode_enter(drv_can->can_periph, drv_can->mode);
#endif
    }
    break;
    case RT_CAN_CMD_SET_MODE:
        argval = (rt_uint32_t)arg;
        if (argval != drv_can->device->config.mode)
        {
            /* can mode config */
            switch (argval)
            {
            case RT_CAN_MODE_NORMAL:
                drv_can->mode = CAN_NORMAL_MODE;
                break;
            case RT_CAN_MODE_LISTEN:
                drv_can->mode = CAN_MONITOR_MODE;
                break;
            case RT_CAN_MODE_LOOPBACK:
                drv_can->mode = CAN_LOOPBACK_SILENT_MODE;
                break;
            case RT_CAN_MODE_LOOPBACKANLISTEN:
                drv_can->mode = CAN_LOOPBACK_SILENT_MODE;
                break;
            default:
                return RT_ERROR;
                break;
            }

            if (SUCCESS == can_operation_mode_enter(drv_can->can_periph, drv_can->mode))
            {
                return RT_EOK;
            }
            else
            {
                return RT_ERROR;
            }
        }
        break;
    case RT_CAN_CMD_SET_BAUD:
        argval = (rt_uint32_t)arg;
        if (argval != CAN1MBaud &&
            argval != CAN800kBaud &&
            argval != CAN500kBaud &&
            argval != CAN250kBaud &&
            argval != CAN125kBaud &&
            argval != CAN100kBaud &&
            argval != CAN50kBaud &&
            argval != CAN20kBaud &&
            argval != CAN10kBaud)
        {
            return -RT_ERROR;
        }

        if (argval != drv_can->device->config.baud_rate)
        {
            baud_index = get_can_baud_index(argval);
            drv_can->can_init.resync_jump_width = can_baud_rate_tab[baud_index].resync_jump_width;
            drv_can->can_init.prop_time_segment = can_baud_rate_tab[baud_index].prop_time_segment;
            drv_can->can_init.time_segment_1 = can_baud_rate_tab[baud_index].time_segment_1;
            drv_can->can_init.time_segment_2 = can_baud_rate_tab[baud_index].time_segment_2;
            drv_can->can_init.prescaler = can_baud_rate_tab[baud_index].prescaler;
            can_operation_mode_enter(drv_can->can_periph, CAN_INACTIVE_MODE);
            CAN_BT(drv_can->can_periph) &= ~(CAN_BT_BAUDPSC | CAN_BT_SJW | CAN_BT_PTS | CAN_BT_PBS1 | CAN_BT_PBS2);
            CAN_BT(drv_can->can_periph) |= (uint32_t)(BT_BAUDPSC(drv_can->can_init.prescaler - 1U) |
                                                      BT_SJW((uint32_t)drv_can->can_init.resync_jump_width - 1U) |
                                                      BT_PTS((uint32_t)drv_can->can_init.prop_time_segment - 1U) |
                                                      BT_PBS1((uint32_t)drv_can->can_init.time_segment_1 - 1U) |
                                                      BT_PBS2((uint32_t)drv_can->can_init.time_segment_2 - 1U));
            can_operation_mode_enter(drv_can->can_periph, drv_can->mode);
        }
        break;
    case RT_CAN_CMD_SET_PRIV:
    {
    }
    break;
    case RT_CAN_CMD_GET_STATUS:
    {
    }
    break;
#ifdef RT_CAN_USING_CANFD
    case RT_CAN_CMD_SET_CANFD:
    {
        /* enable or disable CANFD dynamically */
        rt_uint32_t enable = (rt_uint32_t)arg;
        can_operation_mode_enter(drv_can->can_periph, CAN_INACTIVE_MODE);
        if (enable)
        {
            drv_can->fd_enabled = 1;
            drv_can->can_fd_init.bitrate_switch_enable = ENABLE;
            drv_can->can_fd_init.iso_can_fd_enable = ENABLE;
            drv_can->can_fd_init.mailbox_data_size = CAN_MAILBOX_DATA_SIZE_16_BYTES;
            drv_can->can_fd_init.tdc_enable = DISABLE;
            drv_can->can_fd_init.tdc_offset = 0U;
            can_fd_config(drv_can->can_periph, &drv_can->can_fd_init);
        }
        else
        {
            /* no direct API to disable FD; re-init classic CAN */
            drv_can->fd_enabled = 0;
        }
        can_operation_mode_enter(drv_can->can_periph, drv_can->mode);
    }
    break;
    case RT_CAN_CMD_SET_BAUD_FD:
    {
        rt_uint32_t data_baud;
        int found;
        unsigned k;
        data_baud = (rt_uint32_t)arg;
        if (!drv_can->fd_enabled || data_baud == 0)
            break;
        found = -1;
        for (k = 0; k < (sizeof(can_fd_data_baud_tab) / sizeof(can_fd_data_baud_tab[0])); k++)
        {
            if (can_fd_data_baud_tab[k].baud == data_baud)
            {
                found = (int)k;
                break;
            }
        }
        if (found < 0)
            break; /* unsupported */
        can_operation_mode_enter(drv_can->can_periph, CAN_INACTIVE_MODE);
        drv_can->can_fd_init.resync_jump_width = can_fd_data_baud_tab[found].sjw;
        drv_can->can_fd_init.prop_time_segment = can_fd_data_baud_tab[found].prop;
        drv_can->can_fd_init.time_segment_1 = can_fd_data_baud_tab[found].seg1;
        drv_can->can_fd_init.time_segment_2 = can_fd_data_baud_tab[found].seg2;
        drv_can->can_fd_init.prescaler = can_fd_data_baud_tab[found].prescaler;
        can_fd_config(drv_can->can_periph, &drv_can->can_fd_init);
        can_operation_mode_enter(drv_can->can_periph, drv_can->mode);
    }
    break;
#endif /* RT_CAN_USING_CANFD */
    }

    return RT_EOK;
}

static rt_ssize_t gd32_can_sendmsg(struct rt_can_device *can, const void *buf, rt_uint32_t boxno)
{
    uint8_t i = 0;
    gd32_can_struct *drv_can = (gd32_can_struct *)can->parent.user_data;
    struct rt_can_msg *tx_msg = (struct rt_can_msg *)buf;

    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    RT_ASSERT(drv_can != RT_NULL);
    RT_ASSERT(tx_msg != RT_NULL);

    /* transmit message */
    for (i = 16; i < 31; i++)
    {
        if (0U == (CAN_STAT(drv_can->can_periph) & BIT(i)))
        {
            /* initialize transmit message */
            drv_can->tx_message[i].rtr = tx_msg->rtr;
            drv_can->tx_message[i].ide = tx_msg->ide;
            drv_can->tx_message[i].esi = 0U;
            drv_can->tx_message[i].code = CAN_MB_TX_STATUS_DATA;
            drv_can->tx_message[i].brs = tx_msg->brs;
            drv_can->tx_message[i].fdf = tx_msg->fd_frame;
            drv_can->tx_message[i].prio = tx_msg->priv;
            drv_can->tx_message[i].data_bytes = tx_msg->len; /* classic frame length or already set for FD */

#if defined(SOC_SERIES_GD32H7xx)
            drv_can->tx_message[i].data = (uint32_t *)tx_msg->data;
#elif defined(SOC_SERIES_GD32H75E)
            memcpy(drv_can->tx_message[i].data, tx_msg->data, sizeof(drv_can->tx_message[i].data));
#endif
            drv_can->tx_message[i].id = tx_msg->id;
            can_mailbox_config(drv_can->can_periph, i, &drv_can->tx_message[i]);
            return RT_EOK;
        }
    }
    return RT_EFULL;
}

static rt_ssize_t gd32_can_recvmsg(struct rt_can_device *can, void *buf, rt_uint32_t boxno)
{
    gd32_can_struct *drv_can = (gd32_can_struct *)can->parent.user_data;
    struct rt_can_msg *pmsg = (struct rt_can_msg *)buf;
    RT_ASSERT(can);
    RT_ASSERT(pmsg);

    uint8_t i;
    /* receive data */
    for (i = 0; i < 16; i++)
    {
        if (drv_can->rx_message_valid[i] == 1)
        {
            drv_can->rx_message_valid[i] = 0;
            pmsg->id = drv_can->rx_message[i].id;
            pmsg->ide = drv_can->rx_message[i].ide;
            pmsg->rtr = drv_can->rx_message[i].rtr;
            pmsg->len = drv_can->rx_message[i].data_bytes;
            pmsg->priv = 0;
            pmsg->hdr_index = i;
#ifdef RT_CAN_USING_CANFD
            pmsg->fd_frame = drv_can->rx_message[i].fdf;
            pmsg->brs = drv_can->rx_message[i].brs;
#endif
            rt_memcpy(pmsg->data, drv_can->rx_message[i].data, pmsg->len);
            return RT_EOK;
        }
    }
    return RT_EEMPTY;
}

static const struct rt_can_ops _can_ops = {
    gd32_can_configure,
    gd32_can_control,
    gd32_can_sendmsg,
    gd32_can_recvmsg,
};

static void _can_rx_tx_isr(struct rt_can_device *can)
{
    uint8_t i;
    gd32_can_struct *drv_can = (gd32_can_struct *)can->parent.user_data;
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(drv_can != RT_NULL);

    for (i = 0; i < 16; i++)
    {
        if (CAN_STAT(drv_can->can_periph) & BIT(i))
        {
            can_mailbox_receive_data_read(drv_can->can_periph, i, &drv_can->rx_message[i]);
            drv_can->rx_message_valid[i] = 1;
            rt_hw_can_isr(can, RT_CAN_EVENT_RX_IND);
            CAN_STAT(drv_can->can_periph) = BIT(i); /* Clear the status bit */
        }
    }

    for (i = 16; i < 31; i++)
    {
        if (0U != (CAN_STAT(drv_can->can_periph) & BIT(i)))
        {
            CAN_STAT(drv_can->can_periph) = BIT(i);
            rt_hw_can_isr(can, RT_CAN_EVENT_TX_DONE);
            return;
        }
    }
}

static void _can_sce_isr(struct rt_can_device *can)
{
    gd32_can_struct *drv_can = (gd32_can_struct *)can->parent.user_data;
    rt_uint32_t err_code;

    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(drv_can != RT_NULL);

    if (SET == can_interrupt_flag_get(drv_can->can_periph, CAN_INT_FLAG_ERR_SUMMARY))
    {
        can_interrupt_flag_clear(drv_can->can_periph, CAN_INT_FLAG_ERR_SUMMARY);
        err_code = GET_BITS(CAN_ERR1(drv_can->can_periph), 10U, 15U);

        if (err_code & CAN_ERR1_FMERR)
        {
            can->status.formaterrcnt++;
        }

        if (err_code & CAN_ERR1_CRCERR)
        {
            can->status.crcerrcnt++;
        }

        if (err_code & CAN_ERR1_ACKERR)
        {
            can->status.ackerrcnt++;
        }
        can->status.errcode = err_code;
    }
}

#ifdef BSP_USING_CAN0
void CAN0_Message_IRQHandler(void)
{
    rt_interrupt_enter();
    _can_rx_tx_isr(&dev_can0);
    rt_interrupt_leave();
}

void CAN0_Error_IRQHandler(void)
{
    rt_interrupt_enter();
    _can_sce_isr(&dev_can0);
    rt_interrupt_leave();
}
#endif /* BSP_USING_CAN0 */

#ifdef BSP_USING_CAN1
void CAN1_Message_IRQHandler(void)
{
    rt_interrupt_enter();
    _can_rx_tx_isr(&dev_can1);
    rt_interrupt_leave();
}

void CAN1_Error_IRQHandler(void)
{
    rt_interrupt_enter();
    _can_sce_isr(&dev_can1);
    rt_interrupt_leave();
}
#endif /* BSP_USING_CAN1 */

#ifdef BSP_USING_CAN2
void CAN2_Message_IRQHandler(void)
{
    rt_interrupt_enter();
    _can_rx_tx_isr(&dev_can2);
    rt_interrupt_leave();
}

void CAN2_Error_IRQHandler(void)
{
    rt_interrupt_enter();
    _can_sce_isr(&dev_can2);
    rt_interrupt_leave();
}
#endif /* BSP_USING_CAN2 */

int rt_hw_can_init(void)
{
    int i;
    struct can_configure config = CANDEFAULTCONFIG;
    config.privmode = RT_CAN_MODE_NOPRIV;
    config.ticks = 50;
#ifdef RT_CAN_USING_HDR
    config.maxhdr = 40;
#endif

    for (i = 0; i < sizeof(can_obj) / sizeof(can_obj[0]); i++)
    {
        can_obj[i].device->config = config;
        rt_hw_can_register(can_obj[i].device, can_obj[i].name, &_can_ops, &can_obj[i]);
    }

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_can_init);

#endif /* defined(BSP_USING_CAN0) || defined(BSP_USING_CAN1) || defined(BSP_USING_CAN2) */
#endif /*RT_USING_CAN*/
