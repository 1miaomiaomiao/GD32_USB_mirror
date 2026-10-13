/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 * 2019-01-03     zylx         modify DMA support
 * 2025-10-10     WangShun     compatible with RT-Studio
 */

#ifndef __SPI_CONFIG_H__
#define __SPI_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef RT_SPI_USING_DMA
#define IF_SPI_DMA(x) x
#else
#define IF_SPI_DMA(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  BSP_USING_SPI0
#ifndef BSP_SPI0_SCK_PIN
#define BSP_SPI0_SCK_PIN   "PA5"
#define BSP_SPI0_MISO_PIN  "PA6"
#define BSP_SPI0_MOSI_PIN  "PA7"
#define BSP_SPI0_AFIO      "AF5"
#endif /* BSP_SPI0_SCK_PIN */

#ifndef SPI0_BUS_CONFIG
#define SPI0_BUS_CONFIG                             \
    {                                               \
        .spi_periph = SPI0,                         \
        .bus_name = "spi0",                         \
        .spi_clk = RCU_SPI0,                        \
        .spi_bus = &spi_bus0,                       \
        .sck_pin_name = BSP_SPI0_SCK_PIN,           \
        .miso_pin_name = BSP_SPI0_MISO_PIN,         \
        .mosi_pin_name = BSP_SPI0_MOSI_PIN,         \
        .alternate = BSP_SPI0_AFIO,                 \
        IF_SPI_DMA(.spi_dma = &spi_dma[0])          \
    }
#endif /* SPI0_BUS_CONFIG */
#endif /* BSP_USING_SPI0 */

#ifdef  BSP_USING_SPI1
#ifndef BSP_SPI1_SCK_PIN
#define BSP_SPI1_SCK_PIN   "PB13"
#define BSP_SPI1_MISO_PIN  "PB14"
#define BSP_SPI1_MOSI_PIN  "PB15"
#define BSP_SPI1_AFIO      "AF5"
#endif /* BSP_SPI1_SCK_PIN */

#ifndef SPI1_BUS_CONFIG
#define SPI1_BUS_CONFIG                             \
    {                                               \
        .spi_periph = SPI1,                         \
        .bus_name = "spi1",                         \
        .spi_clk = RCU_SPI1,                        \
        .spi_bus = &spi_bus1,                       \
        .sck_pin_name = BSP_SPI1_SCK_PIN,           \
        .miso_pin_name = BSP_SPI1_MISO_PIN,         \
        .mosi_pin_name = BSP_SPI1_MOSI_PIN,         \
        .alternate = BSP_SPI1_AFIO,                 \
        IF_SPI_DMA(.spi_dma = &spi_dma[1])          \
    }
#endif /* SPI1_BUS_CONFIG */
#endif /* BSP_USING_SPI1 */

#ifdef  BSP_USING_SPI2
#ifndef BSP_SPI2_SCK_PIN
#define BSP_SPI2_SCK_PIN   "PB3"
#define BSP_SPI2_MISO_PIN  "PB4"
#define BSP_SPI2_MOSI_PIN  "PB5"
#define BSP_SPI2_AFIO      "AF6"
#endif /* BSP_SPI1_SCK_PIN */

#ifndef SPI2_BUS_CONFIG
#define SPI2_BUS_CONFIG                             \
    {                                               \
        .spi_periph = SPI2,                         \
        .bus_name = "spi2",                         \
        .spi_clk = RCU_SPI2,                        \
        .spi_bus = &spi_bus2,                       \
        .sck_pin_name = BSP_SPI2_SCK_PIN,           \
        .miso_pin_name = BSP_SPI2_MISO_PIN,         \
        .mosi_pin_name = BSP_SPI2_MOSI_PIN,         \
        .alternate = BSP_SPI2_AFIO,                 \
        IF_SPI_DMA(.spi_dma = &spi_dma[2])          \
    }
#endif /* SPI2_BUS_CONFIG */
#endif /* BSP_USING_SPI2 */

#ifdef  BSP_USING_SPI3
#ifndef BSP_SPI3_SCK_PIN
#define BSP_SPI3_SCK_PIN   "PE12"
#define BSP_SPI3_MISO_PIN  "PE13"
#define BSP_SPI3_MOSI_PIN  "PE14"
#define BSP_SPI3_AFIO      "AF5"
#endif /* BSP_SPI3_SCK_PIN */

#ifndef SPI3_BUS_CONFIG
#define SPI3_BUS_CONFIG                             \
    {                                               \
        .spi_periph = SPI3,                         \
        .bus_name = "spi3",                         \
        .spi_clk = RCU_SPI3,                        \
        .spi_bus = &spi_bus3,                       \
        .sck_pin_name = BSP_SPI3_SCK_PIN,           \
        .miso_pin_name = BSP_SPI3_MISO_PIN,         \
        .mosi_pin_name = BSP_SPI3_MOSI_PIN,         \
        .alternate = BSP_SPI3_AFIO,                 \
        IF_SPI_DMA(.spi_dma = &spi_dma[3])          \
    }
#endif /* SPI3_BUS_CONFIG */
#endif /* BSP_USING_SPI3 */

#ifdef  BSP_USING_SPI4
#ifndef BSP_SPI4_SCK_PIN
#define BSP_SPI4_SCK_PIN   "PF7"
#define BSP_SPI4_MISO_PIN  "PF8"
#define BSP_SPI4_MOSI_PIN  "PF9"
#define BSP_SPI4_AFIO      "AF5"
#endif /* BSP_SPI4_SCK_PIN */

#ifndef SPI4_BUS_CONFIG
#define SPI4_BUS_CONFIG                             \
    {                                               \
        .spi_periph = SPI4,                         \
        .bus_name = "spi4",                         \
        .spi_clk = RCU_SPI4,                        \
        .spi_bus = &spi_bus4,                       \
        .sck_pin_name = BSP_SPI4_SCK_PIN,           \
        .miso_pin_name = BSP_SPI4_MISO_PIN,         \
        .mosi_pin_name = BSP_SPI4_MOSI_PIN,         \
        .alternate = BSP_SPI4_AFIO,                 \
        IF_SPI_DMA(.spi_dma = &spi_dma[4])          \
    }
#endif /* SPI4_BUS_CONFIG */
#endif /* BSP_USING_SPI4 */

#ifdef  BSP_USING_SPI5
#ifndef BSP_SPI5_SCK_PIN
#define BSP_SPI5_SCK_PIN   "PG13"
#define BSP_SPI5_MISO_PIN  "PG12"
#define BSP_SPI5_MOSI_PIN  "PG14"
#define BSP_SPI5_AFIO      "AF5"
#endif /* BSP_SPI5_SCK_PIN */

#ifndef SPI5_BUS_CONFIG
#define SPI5_BUS_CONFIG                             \
    {                                               \
        .spi_periph = SPI5,                         \
        .bus_name = "spi5",                         \
        .spi_clk = RCU_SPI5,                        \
        .spi_bus = &spi_bus5,                       \
        .sck_pin_name = BSP_SPI5_SCK_PIN,           \
        .miso_pin_name = BSP_SPI5_MISO_PIN,         \
        .mosi_pin_name = BSP_SPI5_MOSI_PIN,         \
        .alternate = BSP_SPI5_AFIO,                 \
        IF_SPI_DMA(.spi_dma = &spi_dma[5])          \
    }
#endif /* SPI5_BUS_CONFIG */
#endif /* BSP_USING_SPI5 */

#ifdef __cplusplus
}
#endif

#endif /*__SPI_CONFIG_H__ */

