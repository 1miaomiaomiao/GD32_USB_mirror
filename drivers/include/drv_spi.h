/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-20     BruceOu      first implementation
 * 2025-10-10     WangShun     compatible with RT-Studio
 */

#ifndef __DRV_SPI_H__
#define __DRV_SPI_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RT_SPI_USING_DMA

#if !defined(SOC_SERIES_GD32H7xx) && !defined(SOC_SERIES_GD32H75E)
    #undef RT_SPI_USING_DMA 
#endif

struct gd32_spi_cs
{
    uint32_t GPIOx;
    uint32_t GPIO_Pin;
};

#ifdef RT_SPI_USING_DMA

typedef struct
{
    /* dma peripheral */
    uint32_t dma_periph;
    /* dma txchannel */
    dma_channel_enum txdma_ch;
    /* dma rxchannel */
    dma_channel_enum rxdma_ch;
    /* tx dma request */
    uint32_t dma_mux_req_tx;
    /* rx dma request */
    uint32_t dma_mux_req_rx;
    /* dma flag */
    uint32_t rx_flag;
    /* setting receive len */
    rt_size_t setting_recv_len;

} gd32_spi_dma;
#endif

/* gd32 spi dirver class */
struct gd32_spi
{
    uint32_t spi_periph;
    char *bus_name;
    rcu_periph_enum spi_clk;
    struct rt_spi_bus *spi_bus;
    const char *sck_pin_name;          /* sck pin name */
    const char *miso_pin_name;         /* miso pin name */
    const char *mosi_pin_name;         /* mosi pin name */
    const char *alternate;             /* afio mode */
#ifdef RT_SPI_USING_DMA
    gd32_spi_dma *spi_dma;
#endif
};

rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_SPI_H__ */

