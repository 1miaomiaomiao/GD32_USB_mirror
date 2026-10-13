#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <rtthread.h>

#define CAM_FRAME_WIDTH     320u
#define CAM_FRAME_HEIGHT    240u
#define CAM_FRAME_BYTES     (CAM_FRAME_WIDTH * CAM_FRAME_HEIGHT * 2u)

typedef void (*camera_frame_hook_t)(rt_uint32_t addr, rt_size_t len);

rt_err_t camera_capture_start(void);

void camera_set_frame_hook(camera_frame_hook_t hook);

void camera_stream_set_active(rt_uint8_t on);

#endif
