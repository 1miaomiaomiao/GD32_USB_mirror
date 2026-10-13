/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#ifndef USB_CONF_H
#define USB_CONF_H

#include <board.h>

#if defined(BSP_USING_USB)

#if defined(SOC_SERIES_GD32H75E)
#include "gd32h75e.h"
#elif defined(SOC_SERIES_GD32H7xx)
#include "gd32h7xx.h"
#endif
/* USB Core and PHY interface configuration */

/* USB HS PHY CONFIGURATION */

/* on-chip full-speed USB PHY */
#ifdef USE_USB_FS
#define OC_FS_PHY
#endif

/* on-chip high-speed USB PHY */
#ifdef USE_USB_HS
#define OC_HS_PHY
#endif /* USE_USB_HS */

#if defined(RT_USING_USB_DEVICE)

#define USB_SOF_OUTPUT 0U

/* USB FIFO size config
 * TX1 must hold one full 512 B bulk packet (EP1 IN, WinUSB) + status. */
#define RX_FIFO_SIZE  512U
#define TX0_FIFO_SIZE 128U
#define TX1_FIFO_SIZE 512U
#define TX2_FIFO_SIZE 0U
#define TX3_FIFO_SIZE 0U
#define TX4_FIFO_SIZE 0U
#define TX5_FIFO_SIZE 0U
#define TX6_FIFO_SIZE 0U
#define TX7_FIFO_SIZE 0U

#elif defined(RT_USING_USB_HOST)

#define USBH_USE_RTOS
#define USB_SOF_OUTPUT      1U

/* USB FIFO size config */
#define USB_RX_FIFO_SIZE    512
#define USB_HTX_NPFIFO_SIZE 256
#define USB_HTX_PFIFO_SIZE  256
#endif

/* General USB Configuration */
#define USB_LOW_POWER 0U

/* if uncomment it, need jump to USB JP */
//#define VBUS_SENSING_ENABLED

//#define USB_INTERNAL_DMA_ENABLED
//#define USB_DEDICATED_EP1_ENABLED
/* End General USB Configuration */

#ifdef USE_ULPI_PHY
#define USB_EXTERNAL_ULPI_PHY_ENABLED
#else
#ifdef OC_FS_PHY
#define USB_EMBEDDED_FS_PHY_ENABLED
#elif defined(OC_HS_PHY)
#define USB_EMBEDDED_HS_PHY_ENABLED
#else
#error "PHY is not selected"
#endif /* OC_FS_PHY */
#endif /* USE_ULPI_PHY */

#if defined(RT_USING_USB_HOST)
#define USE_HOST_MODE
#elif defined(RT_USING_USB_DEVICE)
#define USE_DEVICE_MODE
#else
#warning "Please select the USB mode as USBD or USBH; otherwise, the USE_OTG_MODE will be enabled by default."
#define USE_OTG_MODE
#endif

#ifndef OC_FS_PHY
#ifndef OC_HS_PHY
#error "OC_FS_PHY or OC_HS_PHY should be defined!"
#endif
#endif /* OC_FS_PHY */

#ifndef RT_USING_USB_DEVICE
#ifndef RT_USING_USB_HOST
#error "RT_USING_USB_DEVICE or RT_USING_USB_HOST should be defined!"
#endif
#endif /* USE_DEVICE_MODE */

#ifndef USE_USB_HS
#ifndef USE_USB_FS
#error "USE_USB_HS or USE_USB_FS should be defined!"
#endif
#endif /* USE_USB_HS */

/* all variables and data structures during the transaction process should be 4-bytes aligned */
#if defined(__GNUC__)         /* GNU Compiler */
#define __ALIGN_END __attribute__((aligned(4)))
#define __ALIGN_BEGIN
#else
#define __ALIGN_END

#if defined(__CC_ARM)     /* ARM Compiler */
#define __ALIGN_BEGIN __align(4)
#elif defined(__ICCARM__) /* IAR Compiler */
#define __ALIGN_BEGIN
#elif defined(__TASKING__)/* TASKING Compiler */
#define __ALIGN_BEGIN __align(4)
#endif /* __CC_ARM */
#endif /* __GNUC__ */

/* __packed keyword used to decrease the data type alignment to 1-byte */
#if defined(__GNUC__)       /* GNU Compiler */
#ifndef __packed
#define __packed __unaligned
#endif
#elif defined(__TASKING__)    /* TASKING Compiler */
#define __packed __unaligned
#endif /* __GNUC__ */

#endif /* BSP_USING_USB */

#endif /* USB_CONF_H */
