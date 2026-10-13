#ifndef __LCD_H__
#define __LCD_H__

#include <rtthread.h>
#include "gd32h7xx.h"

#define LCD_ACTIVE_WIDTH      480
#define LCD_ACTIVE_HEIGHT     272

#define LCD_FB_A_ADDR         ((rt_uint32_t)(0x24000000UL + 512UL * 1024UL \
                                            - 480UL * 272UL * 2UL))
#define LCD_FB_B_ADDR         ((rt_uint32_t)0xC1800000)

#define LCD_FRAMEBUF_ADDR     LCD_FB_A_ADDR

rt_err_t lcd_init(void);

void lcd_draw_point(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color);

void lcd_clear(rt_uint16_t color);

void lcd_flush(void);

void lcd_flush_buf(void *fb);

void lcd_flip(rt_uint32_t fb_addr);

void lcd_blit(rt_uint16_t x, rt_uint16_t y, rt_uint16_t w, rt_uint16_t h, const rt_uint16_t *src);

#endif
