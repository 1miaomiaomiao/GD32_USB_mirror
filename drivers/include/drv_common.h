/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-0      WangShun     first implementation
 */

#ifndef __DRV_COMMON_H__
#define __DRV_COMMON_H__

#include "drv_usart.h"
#include "drv_gpio.h"
#include <board.h>

#define GD32_FLASH_START_ADRESS       ROM_START
#define GD32_FLASH_SIZE               ROM_SIZE
#define GD32_FLASH_END_ADDRESS        ROM_END

#define GD32_SRAM_SIZE                RAM_SIZE
#define GD32_SRAM_START               RAM_START
#define GD32_SRAM_END                 RAM_END

#ifdef __ARMCC_VERSION
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define HEAP_BEGIN    (__segment_end("HEAP"))
#else
extern int __bss_end;
#define HEAP_BEGIN    (&__bss_end)
#endif

/* Heap ends where the TLI framebuffer reservation starts (see board.h) */
#define HEAP_END          (GD32_SRAM_END - LVGL_FB_SIZE)

#endif /* __DRV_COMMON_H__ */

