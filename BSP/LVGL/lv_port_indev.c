#include "lv_port_indev.h"
#include "lv_port_disp.h"
#include "touch.h"
#include <BSP/USB2SCR/usb2screen.h>
#include "lvgl.h"

static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    LV_UNUSED(indev);

    if(usb2scr_is_active()) {
        data->state=LV_INDEV_STATE_RELEASED;
        return;
    }

    gt911_handler();

    if(gt911.pressed > 0) {
        data->state=LV_INDEV_STATE_PRESSED;
        data->point.x=gt911.x[0];
        data->point.y=gt911.y[0];
    }
    else {
        data->state=LV_INDEV_STATE_RELEASED;
    }
}

rt_err_t lv_port_indev_init(void)
{

    if(gt911_init() != SUCCESS) {
        rt_kprintf("GT911 init failed: no ACK on I2C bus\n");
        return -RT_ERROR;
    }
    rt_kprintf("GT911 touch id: 0x%08x\n", gt911_read_id());

    lv_indev_t * indev=lv_indev_create();
    if(indev == NULL) {
        return -RT_ENOMEM;
    }
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read_cb);
    lv_indev_set_display(indev, lv_display_get_default());

    return RT_EOK;
}
