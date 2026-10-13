/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-10     kurisaw      ENET configuration
 */

#ifndef __ENET_CONFIG_H__
#define __ENET_CONFIG_H__

#include "drv_enet.h"
#if defined(RT_USING_SAL) && defined(RT_USING_NETDEV) && defined(RT_USING_LWIP)

#ifdef __cplusplus
extern "C" {
#endif

/* ENET pin configuration table for RMII interface */
const struct enet_pin_config enet0_pins[] =
{
    /* ETH_RMII_REF_CLK */ {"PA1", "AF11"},
    /* ETH_MDIO */         {"PA2", "AF11"},
    /* ETH_RMII_CRS_DV */  {"PA7", "AF11"},
    /* ETH_RMII_TX_EN */   {"PG11", "AF11"},
    /* ETH_RMII_TXD0 */    {"PB12", "AF11"},
    /* ETH_RMII_TXD1 */    {"PG12", "AF11"},
    /* ETH_MDC */          {"PC1", "AF11"},
    /* ETH_RMII_RXD0 */    {"PC4", "AF11"},
    /* ETH_RMII_RXD1 */    {"PC5", "AF11"},
};

/* Clock output pin for PHY */
const struct enet_pin_config enet_clock_pin = {
    "PA8", "AF0"  /* Clock output for PHY */
};

/* ENET configuration */
const struct enet_config enet0_cfg = {
    .pins = enet0_pins,
    .pin_count = sizeof(enet0_pins) / sizeof(enet0_pins[0]),
    .enet_periph = ENET0,
    .enet_clk = RCU_ENET0,
    .enet_tx_clk = RCU_ENET0TX,
    .enet_rx_clk = RCU_ENET0RX,
    .phy_interface = SYSCFG_ENET_PHY_RMII,
    .speed = GPIO_OSPEED_100_220MHZ,
    .otype = GPIO_OTYPE_PP,
    .pupd = GPIO_PUPD_NONE,
};

#ifdef __cplusplus
}
#endif

#endif /* RT_USING_SAL && RT_USING_NETDEV && RT_USING_LWIP */
#endif /* __ENET_CONFIG_H__ */
