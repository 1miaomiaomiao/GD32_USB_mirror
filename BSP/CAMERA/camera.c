#include "camera.h"
#define DBG_TAG "cam"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "gd32h7xx.h"
#include <rthw.h>
#include "exmc_sdram.h"
#include "dci_ov2640.h"

#define CAMERA_CAPTURE0_ADDR    ((rt_uint32_t)0xC0000000)
#define CAMERA_CAPTURE1_ADDR    ((rt_uint32_t)0xC0030000)

static const rt_uint32_t cam_raw_addr[2]={ CAMERA_CAPTURE0_ADDR,
                                             CAMERA_CAPTURE1_ADDR };

static rt_sem_t   cam_frame_sem=RT_NULL;
static volatile rt_uint8_t cam_dma_dst=0;
static volatile rt_uint8_t cam_filled=0;
static volatile rt_uint8_t cam_stream_active=0;
static volatile rt_uint8_t cam_started=0;
static camera_frame_hook_t cam_hook=RT_NULL;

void DMA1_Channel7_IRQHandler(void)
{
    if(dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INT_FLAG_FTF)) {
        rt_uint8_t filled;

        dma_channel_disable(DMA1, DMA_CH7);

        filled=cam_dma_dst;
        cam_dma_dst ^= 1;
        cam_filled=filled;

        dma_memory_address_config(DMA1, DMA_CH7, DMA_MEMORY_0,
                                  cam_raw_addr[cam_dma_dst]);
        dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_INT_FLAG_FTF);
        dma_channel_enable(DMA1, DMA_CH7);

        rt_sem_release(cam_frame_sem);
    }
}

static void camera_capture_loop(void)
{
    while(1) {
        if(rt_sem_take(cam_frame_sem, RT_WAITING_FOREVER) != RT_EOK) {
            continue;
        }
        if(cam_stream_active && cam_hook != RT_NULL) {
            rt_uint32_t addr=cam_raw_addr[cam_filled];

            rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE,
                                 (void *)addr, CAM_FRAME_BYTES);
            cam_hook(addr, CAM_FRAME_BYTES);
        }
    }
}

static void camera_thread_entry(void *parameter)
{
    rt_uint8_t rc;

    RT_UNUSED(parameter);

    exmc_synchronous_dynamic_ram_init(EXMC_SDRAM_DEVICE0);
    rt_thread_mdelay(50);

    cam_frame_sem=rt_sem_create("cam_frame", 0, RT_IPC_FLAG_FIFO);
    if(cam_frame_sem == RT_NULL) {
        LOG_E("create camera semaphore failed");
        cam_started=0;
        return;
    }

    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    nvic_irq_enable(DMA1_Channel7_IRQn, 0U, 1U);

    rc=dci_ov2640_init();

    if(rc != 0) {
        LOG_E("ov2640 init failed -- camera unavailable");
        cam_started=0;
        return;
    }

    dma_interrupt_enable(DMA1, DMA_CH7, DMA_INT_FTF);
    dma_channel_enable(DMA1, DMA_CH7);
    dci_enable();
    dci_capture_enable();
    rt_thread_mdelay(100);

    rt_sem_control(cam_frame_sem, RT_IPC_CMD_RESET, RT_NULL);
    camera_capture_loop();
}

rt_err_t camera_capture_start(void)
{
    rt_thread_t tid;

    if(cam_started) {
        return RT_EOK;
    }
    cam_started=1;

    tid=rt_thread_create("camera", camera_thread_entry, RT_NULL,
                           2048, 12, 20);
    if(tid == RT_NULL) {
        LOG_E("create camera thread failed");
        cam_started=0;
        return -RT_ENOMEM;
    }
    rt_thread_startup(tid);
    return RT_EOK;
}

void camera_set_frame_hook(camera_frame_hook_t hook)
{
    cam_hook=hook;
}

void camera_stream_set_active(rt_uint8_t on)
{
    cam_stream_active=on;
}
