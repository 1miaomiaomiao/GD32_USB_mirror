#include "lvgl_app.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lcd.h"
#include "touch.h"
#include <BSP/USB2SCR/usb2screen.h>
#include "lvgl.h"
#include <rtthread.h>

#define DBG_TAG "lvgl_app"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define LVGL_THREAD_STACK      4096
#define LVGL_THREAD_PRIO       15
#define LVGL_THREAD_TIMESLICE  10

#define SCR_W   LCD_ACTIVE_WIDTH
#define SCR_H   LCD_ACTIVE_HEIGHT

enum {
    PAGE_MENU=0,
    PAGE_COUNT
};

static lv_obj_t *pages[PAGE_COUNT];
static int current_page=PAGE_MENU;

static void nav_to_page(int page)
{
    int i;
    for(i=0; i < PAGE_COUNT; i++) {
        if(i == page) {
            lv_obj_remove_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    current_page=page;
}

static void btn_subscr_cb(lv_event_t *e)
{
    rt_uint8_t mode=(rt_uint8_t)(intptr_t)lv_event_get_user_data(e);
    usb2scr_request_mode(mode);
}

static void btn_camera_cb(lv_event_t *e)
{
    usb2scr_stats_t st;
    LV_UNUSED(e);
    usb2scr_get_stats(&st);
    usb2scr_request_mode(st.mode_req == USB2SCR_MODE_CAMERA
                         ? USB2SCR_MODE_STOP : USB2SCR_MODE_CAMERA);
}

static lv_obj_t *page_create(int idx)
{
    lv_obj_t *p=lv_obj_create(lv_screen_active());
    lv_obj_set_size(p, SCR_W, SCR_H);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_pad_row(p, 0, 0);
    lv_obj_set_style_pad_column(p, 0, 0);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    pages[idx]=p;
    return p;
}

static lv_obj_t *make_btn(lv_obj_t *parent, int x, int y, int w, int h,
                          const char *text, lv_event_cb_t cb, void *data)
{
    lv_obj_t *btn=lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_pad_all(btn, 2, 0);
    if(cb != NULL) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, data);
    }
    lv_obj_t *lbl=lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    return btn;
}

static void page_menu_create(lv_obj_t *parent)
{

    lv_obj_t *title=lv_label_create(parent);
    lv_label_set_text(title, "USB Sub-Display & Camera");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    make_btn(parent, 40, 52, 400, 58, "PC Monitor\n(PC dashboard)",
             btn_subscr_cb, (void *)(intptr_t)USB2SCR_MODE_MONITOR);
    make_btn(parent, 40, 118, 400, 58, "Screen Mirror\n(PC screen)",
             btn_subscr_cb, (void *)(intptr_t)USB2SCR_MODE_MIRROR);
    make_btn(parent, 40, 184, 400, 58, "USB Camera\n(OV2640 to PC)",
             btn_camera_cb, RT_NULL);

    lv_obj_t *hint=lv_label_create(parent);
    lv_label_set_text(hint,
                      "Monitor/Mirror: X badge or PA0 to exit");
    lv_obj_set_pos(hint, 0, 252);
    lv_obj_set_width(hint, SCR_W);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(hint, lv_palette_main(LV_PALETTE_GREY), 0);
}

static void ui_create(void)
{
    lv_obj_t *scr=lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_set_style_pad_column(scr, 0, 0);
    lv_obj_set_style_radius(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    page_create(PAGE_MENU);
    page_menu_create(pages[PAGE_MENU]);

    nav_to_page(PAGE_MENU);
}

static void lvgl_thread_entry(void *param)
{
    rt_uint8_t scr_was_active=0;

    LV_UNUSED(param);

    while(1) {

        usb2scr_stats_t st;
        usb2scr_get_stats(&st);
        if(scr_was_active && !st.active) {
            nav_to_page(PAGE_MENU);
            lv_obj_invalidate(lv_screen_active());
        }
        scr_was_active=st.active;

        lv_timer_handler();
        rt_thread_mdelay(5);
    }
}

rt_err_t lvgl_app_init(void)
{
    rt_err_t err;
    rt_thread_t tid;

    err=lcd_init();
    if(err != RT_EOK) {
        LOG_E("lcd_init failed: %d", err);
        return err;
    }

    lv_init();

    lv_tick_set_cb((lv_tick_get_cb_t)rt_tick_get_millisecond);

    err=lv_port_disp_init();
    if(err != RT_EOK) {
        LOG_E("lv_port_disp_init failed: %d", err);
        return err;
    }

    err=lv_port_indev_init();
    if(err != RT_EOK) {
        LOG_W("lv_port_indev_init failed (%d): touch not available.", err);
    }

    ui_create();

    tid=rt_thread_create("lvgl", lvgl_thread_entry, RT_NULL,
                           LVGL_THREAD_STACK, LVGL_THREAD_PRIO,
                           LVGL_THREAD_TIMESLICE);
    if(tid == RT_NULL) {
        LOG_E("Failed to create LVGL thread.");
        return -RT_ENOMEM;
    }
    rt_thread_startup(tid);
    return RT_EOK;
}
