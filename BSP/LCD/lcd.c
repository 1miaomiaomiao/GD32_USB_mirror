#include "lcd.h"

static rt_uint16_t * const framebuffer=(rt_uint16_t *)LCD_FRAMEBUF_ADDR;

void lcd_draw_point(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color)
{
    if(x >= LCD_ACTIVE_WIDTH || y >= LCD_ACTIVE_HEIGHT) {
        return;
    }
    framebuffer[y * LCD_ACTIVE_WIDTH + x]=color;
}

void lcd_clear(rt_uint16_t color)
{
    rt_uint32_t i;
    for(i=0; i < (rt_uint32_t)LCD_ACTIVE_WIDTH * LCD_ACTIVE_HEIGHT; i++) {
        framebuffer[i]=color;
    }
    lcd_flush();
}

void lcd_flush(void)
{
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)LCD_FB_A_ADDR,
                                      LCD_ACTIVE_WIDTH * LCD_ACTIVE_HEIGHT * 2);
}

void lcd_flush_buf(void *fb)
{

    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)fb,
                                      LCD_ACTIVE_WIDTH * LCD_ACTIVE_HEIGHT * 2);
}

void lcd_flip(rt_uint32_t fb_addr)
{
    TLI_LXFBADDR(LAYER0)=fb_addr;
    tli_reload_config(TLI_FRAME_BLANK_RELOAD_EN);
}

void lcd_blit(rt_uint16_t x, rt_uint16_t y, rt_uint16_t w, rt_uint16_t h, const rt_uint16_t *src)
{
    rt_uint16_t row, col;

    if(src == RT_NULL) {
        return;
    }
    if(x >= LCD_ACTIVE_WIDTH || y >= LCD_ACTIVE_HEIGHT) {
        return;
    }
    if(x + w > LCD_ACTIVE_WIDTH)  w=LCD_ACTIVE_WIDTH  - x;
    if(y + h > LCD_ACTIVE_HEIGHT) h=LCD_ACTIVE_HEIGHT - y;

    for(row=0; row < h; row++) {
        rt_uint16_t *dst=&framebuffer[(y + row) * LCD_ACTIVE_WIDTH + x];
        const rt_uint16_t *s=&src[row * w];
        for(col=0; col < w; col++) {
            dst[col]=s[col];
        }
    }
}

rt_err_t lcd_init(void)
{

    lcd_clear(0x0000);
    return RT_EOK;
}
