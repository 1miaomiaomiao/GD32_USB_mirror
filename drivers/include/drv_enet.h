/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-10     kurisaw      the first version
 */

#ifndef __DRV_ENET_H__
#define __DRV_ENET_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#if defined(RT_USING_SAL) && defined(RT_USING_NETDEV) && defined(RT_USING_LWIP)

#ifdef __cplusplus
extern "C" {
#endif

#include "gd32h7xx.h"
#include "synopsys_emac.h"

#define EMAC_RXBUFNB 5
#define EMAC_TXBUFNB 5

#define EMAC_PHY_AUTO    0
#define EMAC_PHY_10MBIT  1
#define EMAC_PHY_100MBIT 2

#define MAX_ADDR_LEN 6

/* ENET pin configuration structure */
struct enet_pin_config
{
    const char *pin_name;      /* Pin name in format "PxY" */
    const char *alternate;     /* Alternate function in format "AFx" */
};

/* ENET configuration structure */
struct enet_config
{
    const struct enet_pin_config *pins;
    uint32_t pin_count;
    uint32_t enet_periph;
    rcu_periph_enum enet_clk;
    rcu_periph_enum enet_tx_clk;
    rcu_periph_enum enet_rx_clk;
    uint32_t phy_interface;
    uint32_t speed;           /* GPIO speed - common for all pins */
    uint8_t otype;            /* Output type - common for all pins */
    uint8_t pupd;             /* Pull-up/pull-down - common for all pins */
};

struct gd32_emac
{
    /* inherit from Ethernet device */
    struct eth_device parent;

    __ALIGNED(4)
    rt_uint8_t phy_mode;
    /* interface address info. */
    __ALIGNED(4)
    rt_uint8_t dev_addr[MAX_ADDR_LEN];     /* hw address   */

    const struct enet_pin_config *pins;
    uint32_t pin_count;
    struct rt_synopsys_eth *ETHERNET_MAC;
    IRQn_Type ETHER_MAC_IRQ;

    EMAC_DMADESCTypeDef *DMATxDescToSet;
    EMAC_DMADESCTypeDef *DMARxDescToGet;

#pragma pack(4)
    EMAC_DMADESCTypeDef DMARxDscrTab[EMAC_RXBUFNB];
#pragma pack(4)
    EMAC_DMADESCTypeDef DMATxDscrTab[EMAC_TXBUFNB];
#pragma pack(4)
    rt_uint8_t Rx_Buff[EMAC_RXBUFNB][EMAC_MAX_PACKET_SIZE];
#pragma pack(4)
    rt_uint8_t Tx_Buff[EMAC_TXBUFNB][EMAC_MAX_PACKET_SIZE];

    struct rt_semaphore tx_buf_free;
};

/* Function declarations */
rt_err_t gd32_enet_gpio_init(struct gd32_emac *emac);
const struct enet_config *gd32_get_enet_config(void);

#ifdef __cplusplus
}
#endif

#endif /* RT_USING_SAL && RT_USING_NETDEV && RT_USING_LWIP */
#endif /* __DRV_ENET_H__ */
