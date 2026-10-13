#include "lv_port_disp.h"
#include "lcd.h"
#include <BSP/USB2SCR/usb2screen.h>
#include <rtthread.h>
#include <string.h>

#define DRAW_BUF_LINES   80
#define DRAW_BUF_SIZE    (LCD_ACTIVE_WIDTH * DRAW_BUF_LINES * 2)

static uint8_t draw_buf[DRAW_BUF_SIZE];

static void lv_port_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    RT_UNUSED(disp);

    if(usb2scr_is_active()) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t w=area->x2 - area->x1 + 1;
    int32_t h=area->y2 - area->y1 + 1;

    uint32_t mark=(uint32_t)area->y2 + 2;
    if(mark >= LCD_ACTIVE_HEIGHT) {
        mark=LCD_ACTIVE_HEIGHT - 1;
    }

    tli_interrupt_flag_clear(TLI_INT_FLAG_LM);
    tli_line_mark_set((uint16_t)mark);

    uint32_t tmo=30;
    while((tli_interrupt_flag_get(TLI_INT_FLAG_LM) == RESET) && --tmo) {
    }
    tli_interrupt_flag_clear(TLI_INT_FLAG_LM);

    lcd_blit((rt_uint16_t)area->x1, (rt_uint16_t)area->y1,
             (rt_uint16_t)w, (rt_uint16_t)h, (const rt_uint16_t *)px_map);

    lcd_flush();

    lv_display_flush_ready(disp);
}

rt_err_t lv_port_disp_init(void)
{
    lv_display_t * disp;

    disp=lv_display_create(LCD_ACTIVE_WIDTH, LCD_ACTIVE_HEIGHT);
    if(disp == NULL) {
        return -RT_ERROR;
    }

    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    lv_display_set_buffers(disp,
                           draw_buf,
                           RT_NULL,
                           DRAW_BUF_SIZE,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb(disp, lv_port_flush_cb);

    return RT_EOK;
}
