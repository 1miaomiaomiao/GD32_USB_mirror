#include "key.h"
#include "gd32h7xx.h"
#include <drivers/pin.h>
#include <BSP/USB2SCR/usb2screen.h>

#define KEY_EVENT_WAKEUP     (1 << 0)

#define KEY_WAKEUP_PIN      ((0 <<8)|0)

static rt_event_t key_event=RT_NULL;

static void key_callback(void *args)
{
    rt_uint32_t pin=(rt_uint32_t)args;

    if(pin == KEY_WAKEUP_PIN)
    {
        rt_event_send(key_event, KEY_EVENT_WAKEUP);
    }
}

void key_exti_init(void)
{
    key_event=rt_event_create("key_event", RT_IPC_FLAG_PRIO);

    rt_pin_mode(KEY_WAKEUP_PIN, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_attach_irq(KEY_WAKEUP_PIN, PIN_IRQ_MODE_RISING, key_callback, (void*)KEY_WAKEUP_PIN);
    rt_pin_irq_enable(KEY_WAKEUP_PIN, PIN_IRQ_ENABLE);
}

void key_thread_entry(void *parameter)
{
    rt_uint32_t recv_event;

    while(1)
    {
        if(rt_event_recv(key_event,
                        KEY_EVENT_WAKEUP,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER,
                        &recv_event) == RT_EOK)
        {
            rt_thread_mdelay(20);

            if(recv_event & KEY_EVENT_WAKEUP)
            {
                usb2scr_stats_t st;
                usb2scr_get_stats(&st);
                if(usb2scr_is_active() || st.mode_req == USB2SCR_MODE_CAMERA)
                {
                    usb2scr_user_exit();
                }
            }
        }
    }
}
