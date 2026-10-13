#ifndef __USB2SCREEN_H__
#define __USB2SCREEN_H__

#include <rtthread.h>

#define USB2SCR_MODE_STOP     0u
#define USB2SCR_MODE_MONITOR  1u
#define USB2SCR_MODE_MIRROR   2u
#define USB2SCR_MODE_CAMERA   3u

typedef struct
{
    rt_uint8_t  active;
    rt_uint8_t  link_alive;
    rt_uint8_t  mode_req;
    rt_uint8_t  reserved;
    rt_uint32_t rx_frames;
    rt_uint32_t rx_bytes;
    rt_uint32_t read_stalls;
    rt_uint32_t tx_frames;
} usb2scr_stats_t;

void usb2screen_init(void);

void usb2scr_get_stats(usb2scr_stats_t *out);

rt_uint8_t usb2scr_is_active(void);

void usb2scr_request_mode(rt_uint8_t mode);

void usb2scr_user_exit(void);

#endif
