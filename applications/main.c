#include <rtthread.h>
#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include "key.h"
#include "lvgl_app.h"
#include "fault_screen.h"
#include <BSP/USB2SCR/usb2screen.h>

int main(void)
{
    rt_thread_t key_thread;
    key_thread = rt_thread_create("key", key_thread_entry, RT_NULL, 1024, 9, 20);
    if(key_thread != RT_NULL)
    {
        rt_thread_startup(key_thread);
    }

    usb2screen_init();
    fault_screen_init();
    lvgl_app_init();

    while (1)
    {
        rt_thread_mdelay(1000);
    }
    return RT_EOK;
}
