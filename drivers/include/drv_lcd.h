/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__

#include <board.h>

#if defined(SOC_SERIES_GD32H7xx)
#include "gd32h7xx_tli.h"
#include "gd32h7xx_ipa.h"
#endif

#define HORIZONTAL_SYNCHRONOUS_PULSE 41
#define HORIZONTAL_BACK_PORCH        2
#define ACTIVE_WIDTH                 480
#define HORIZONTAL_FRONT_PORCH       2

#define VERTICAL_SYNCHRONOUS_PULSE 10
#define VERTICAL_BACK_PORCH        2
#define ACTIVE_HEIGHT              272
#define VERTICAL_FRONT_PORCH       2

#define LCD_WIGHT       ACTIVE_WIDTH
#define LCD_HIGHT       ACTIVE_HEIGHT
#define PIXEL_FORMAT    LAYER_PPF_RGB565
#define PRE_PIXEL_BYTES 2

typedef struct _RGB_DATA
{
    uint8_t Alpha;
    uint8_t R;
    uint8_t G;
    uint8_t B;
} RGB;

extern RGB color_write;
extern RGB color_black;
extern RGB color_red;
extern RGB color_blue;
extern RGB color_green;
extern RGB color_yellow;
extern RGB color_cyan;
extern RGB color_magenta;
extern RGB color_orange;
extern RGB color_purple;
extern RGB color_gray;
extern RGB color_lightgray;
extern RGB color_darkgray;
extern RGB color_brown;
extern RGB color_pink;
extern RGB color_gold;
extern RGB color_silver;

void lcd_draw_picture(uint32_t picture_addr, uint32_t picture_width, uint32_t picture_height, uint32_t picture_x, uint32_t picture_y);
void lcd_draw_point(uint32_t point_x, uint32_t point_y, RGB *color);
uint32_t to_RGB565(RGB *color);

#endif // __DRV_LCD_H__
