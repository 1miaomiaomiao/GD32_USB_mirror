#include "usb2screen.h"
#include "lcd.h"
#include "touch.h"
#include "camera.h"
#include "gd32h7xx.h"
#include <rtdevice.h>
#include <rthw.h>

#define DBG_TAG "usb2scr"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define SCR_CHUNK_BYTES   (102u * 512u)
#define SCR_CHUNKS        5u
#define SCR_FB_ADDR       LCD_FB_A_ADDR
#define SCR_FB_SIZE       (480u * 272u * 2u)

#define SCR_STALL_TIMEOUT_MS  100u
#define SCR_STALL_RESIGNS     10u

#define SCR_EXIT_X0       410u
#define SCR_EXIT_X1       480u
#define SCR_EXIT_Y0       0u
#define SCR_EXIT_Y1       56u

#define CAM_HDR0          'C'
#define CAM_HDR1          'A'
#define CAM_HDR2          'M'
#define CAM_HDR_BYTES     8u
#define CAM_HDR_TX_TO_MS  500u
#define CAM_DAT_TX_TO_MS  3000u

static void scr_tli_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOH);
    rcu_periph_clock_enable(RCU_GPIOG);

    gpio_af_set(GPIOE, GPIO_AF_10, GPIO_PIN_15);
    gpio_af_set(GPIOA, GPIO_AF_14, GPIO_PIN_7);
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_7);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_15);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_7);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_7);

    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_6);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_12);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_11);
    gpio_af_set(GPIOA, GPIO_AF_14, GPIO_PIN_5);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_9);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_8);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_3);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_2);

    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_6);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_12);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_11);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_5);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_9);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_8);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_3);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_2);

    gpio_af_set(GPIOD, GPIO_AF_14, GPIO_PIN_3);
    gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_7);
    gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_1);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_15);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_14);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_13);
    gpio_af_set(GPIOB, GPIO_AF_14, GPIO_PIN_0);
    gpio_af_set(GPIOB, GPIO_AF_14, GPIO_PIN_1);

    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_3);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_7);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_1);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_15);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_15);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_14);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_14);
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_13);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_1);

    gpio_af_set(GPIOB, GPIO_AF_14, GPIO_PIN_9);
    gpio_af_set(GPIOB, GPIO_AF_14, GPIO_PIN_8);
    gpio_af_set(GPIOB, GPIO_AF_3,  GPIO_PIN_5);
    gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_11);
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_11);
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_10);
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_12);
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_14);

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_9);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_8);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_5);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_11);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_11);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_10);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_12);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_14);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_14);

    gpio_af_set(GPIOF, GPIO_AF_14, GPIO_PIN_10);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_10);

    gpio_mode_set(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_13);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_13);
    gpio_bit_set(GPIOG, GPIO_PIN_13);
}

static rt_err_t scr_tli_init(void)
{
    rcu_periph_clock_enable(RCU_TLI);
    scr_tli_gpio_config();

    rcu_pll_input_output_clock_range_config(IDX_PLL2, RCU_PLL2RNG_1M_2M,
                                            RCU_PLL2VCO_150M_420M);
    if(ERROR == rcu_pll2_config(25, 120, 3, 3, 3)) {
        return -RT_ERROR;
    }
    rcu_pll_clock_output_enable(RCU_PLL2R);
    rcu_tli_clock_div_config(RCU_PLL2R_DIV8);
    rcu_osci_on(RCU_PLL2_CK);
    if(ERROR == rcu_osci_stab_wait(RCU_PLL2_CK)) {
        return -RT_ERROR;
    }

    static tli_parameter_struct tli_initstruct;
    tli_initstruct.signalpolarity_hs=TLI_HSYN_ACTLIVE_LOW;
    tli_initstruct.signalpolarity_vs=TLI_VSYN_ACTLIVE_LOW;
    tli_initstruct.signalpolarity_de=TLI_DE_ACTLIVE_LOW;
    tli_initstruct.signalpolarity_pixelck=TLI_PIXEL_CLOCK_TLI;
    tli_initstruct.synpsz_hpsz=40;
    tli_initstruct.synpsz_vpsz=9;
    tli_initstruct.backpsz_hbpsz=42;
    tli_initstruct.backpsz_vbpsz=11;
    tli_initstruct.activesz_hasz=522;
    tli_initstruct.activesz_vasz=283;
    tli_initstruct.totalsz_htsz=524;
    tli_initstruct.totalsz_vtsz=285;
    tli_initstruct.backcolor_red=0;
    tli_initstruct.backcolor_green=0;
    tli_initstruct.backcolor_blue=0;
    tli_init(&tli_initstruct);

    static tli_layer_parameter_struct l0;
    l0.layer_window_leftpos=43;
    l0.layer_window_rightpos=(43 + 480 - 1);
    l0.layer_window_toppos=12;
    l0.layer_window_bottompos=(12 + 272 - 1);
    l0.layer_ppf=LAYER_PPF_RGB565;
    l0.layer_sa=255;
    l0.layer_default_blue=0;
    l0.layer_default_green=0;
    l0.layer_default_red=0;
    l0.layer_default_alpha=0xFF;
    l0.layer_acf1=LAYER_ACF1_PASA;
    l0.layer_acf2=LAYER_ACF1_PASA;
    l0.layer_frame_bufaddr=SCR_FB_ADDR;
    l0.layer_frame_line_length=((480 * 2) + 3);
    l0.layer_frame_buf_stride_offset=(480 * 2);
    l0.layer_frame_total_line_number=272;
    tli_layer_init(LAYER0, &l0);

    tli_dither_config(TLI_DITHER_ENABLE);

    tli_layer_enable(LAYER0);
    tli_reload_config(TLI_REQUEST_RELOAD_EN);
    tli_enable();
    return RT_EOK;
}

static rt_sem_t      scr_sem=RT_NULL;
static rt_mutex_t    scr_stats_mtx=RT_NULL;
static rt_device_t   scr_usb_dev=RT_NULL;

static volatile rt_uint8_t  req_armed=0;
static volatile rt_uint8_t  chunk_done=0;
static volatile rt_uint8_t  stall_count=0;
static volatile rt_uint32_t fb_offset=0;
static volatile rt_size_t   done_size=0;
static volatile rt_uint8_t  scr_active=0;
static volatile rt_uint8_t  scr_link=0;
static volatile rt_uint8_t  scr_mode_req=USB2SCR_MODE_STOP;
static rt_tick_t            last_rx_tick=0;

static volatile rt_uint8_t  scr_link_resync=0;

static usb2scr_stats_t scr_stats;

static rt_sem_t      cam_frame_sem=RT_NULL;
static rt_sem_t      cam_tx_sem=RT_NULL;
static rt_mutex_t    scr_in_mtx=RT_NULL;
static volatile rt_uint32_t cam_frame_addr=0;
static volatile rt_size_t   cam_frame_len=0;

static rt_err_t scr_rx_indicate(rt_device_t dev, rt_size_t size)
{
    RT_UNUSED(dev);
    done_size=size;
    chunk_done=1;
    req_armed=0;
    rt_sem_release(scr_sem);
    return RT_EOK;
}

static void scr_disp_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);

    scr_usb_dev=rt_device_find("winusb");
    if(scr_usb_dev == RT_NULL) {
        LOG_E("winusb device not found -- usbd core did not enumerate?");
        return;
    }
    if(rt_device_open(scr_usb_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
        LOG_E("winusb open failed");
        return;
    }
    scr_usb_dev->rx_indicate=scr_rx_indicate;

    while(1) {
        (void)rt_sem_take(scr_sem, rt_tick_from_millisecond(SCR_STALL_TIMEOUT_MS));

        if(chunk_done) {
            rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,
                                 (void *)(SCR_FB_ADDR + fb_offset),
                                 done_size);
            rt_mutex_take(scr_stats_mtx, RT_WAITING_FOREVER);
            scr_stats.rx_bytes += done_size;
            rt_mutex_release(scr_stats_mtx);

            if(!scr_link) {
                scr_link=1;
                LOG_I("display stream link up");
            }
            if(!scr_active) {
                scr_active=1;
            }

            fb_offset += done_size;
            if(fb_offset >= SCR_FB_SIZE) {

                fb_offset -= SCR_FB_SIZE;
                last_rx_tick=rt_tick_get();
                rt_mutex_take(scr_stats_mtx, RT_WAITING_FOREVER);
                scr_stats.rx_frames++;
                rt_mutex_release(scr_stats_mtx);
            }
            chunk_done=0;
        }

        if(req_armed) {
            stall_count++;
            if(scr_mode_req == USB2SCR_MODE_STOP ||
               stall_count >= SCR_STALL_RESIGNS) {
                if(scr_active) {
                    LOG_W("chunk stall x%u (mode %u) -- releasing panel",
                          stall_count, scr_mode_req);
                    req_armed=0;
                    fb_offset=0;
                    scr_link=0;
                    if(scr_mode_req == USB2SCR_MODE_STOP) {
                        rt_memset((void *)SCR_FB_ADDR, 0, SCR_FB_SIZE);
                        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,
                                             (void *)SCR_FB_ADDR, SCR_FB_SIZE);
                    }
                    scr_active=0;
                }
                stall_count=0;
                continue;
            }
        } else {
            stall_count=0;
        }

        {
            rt_size_t s=rt_device_read(scr_usb_dev, 0,
                                         (void *)(SCR_FB_ADDR + fb_offset),
                                         SCR_CHUNK_BYTES);
            if(s > 0) {
                req_armed=1;
                last_rx_tick=rt_tick_get();
            } else {
                if(scr_active) {
                    rt_thread_mdelay(10);
                } else {
                    rt_mutex_take(scr_stats_mtx, RT_WAITING_FOREVER);
                    scr_stats.read_stalls++;
                    rt_mutex_release(scr_stats_mtx);
                    rt_thread_mdelay(200);
                }
            }
        }
    }
}

static void scr_usb_enable_cb(void)
{
    scr_link_resync=1;
}

static void scr_ctl_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);
    extern void (*g_winusb_enable_cb)(void);
    rt_uint8_t down=0;
    rt_uint8_t resends=0;
    rt_uint8_t last_sent=USB2SCR_MODE_STOP;
    static rt_uint8_t mode_pkt;

    g_winusb_enable_cb=scr_usb_enable_cb;

    while(1) {
        rt_thread_mdelay(20);

        {
            rt_uint8_t send_now=0;
            if(scr_link_resync) {
                scr_link_resync=0;
                last_sent=scr_mode_req;
                resends=2;
                send_now=1;
            } else if(scr_mode_req != last_sent) {
                last_sent=scr_mode_req;
                resends=2;
                send_now=1;
            } else if(resends > 0) {
                resends--;
                send_now=1;
            }
            if(send_now && scr_usb_dev != RT_NULL) {
                mode_pkt=scr_mode_req;

                rt_mutex_take(scr_in_mtx, RT_WAITING_FOREVER);
                (void)rt_device_write(scr_usb_dev, 0, &mode_pkt, 1);
                rt_mutex_release(scr_in_mtx);
            }
        }

        if(!scr_active) {
            down=0;
            continue;
        }

        if(gt911_handler() == ERROR) {
            continue;
        }

        rt_uint8_t pressed=(gt911.pressed > 1);

        if(pressed && !down &&
           gt911.x[0] >= SCR_EXIT_X0 && gt911.x[0] < SCR_EXIT_X1 &&
           gt911.y[0] >= SCR_EXIT_Y0 && gt911.y[0] < SCR_EXIT_Y1) {
            usb2scr_user_exit();
        }

        down=pressed;
    }
}

static void scr_cam_frame_hook(rt_uint32_t addr, rt_size_t len)
{
    cam_frame_addr=addr;
    cam_frame_len=len;
    rt_sem_release(cam_frame_sem);
}

static rt_err_t scr_tx_complete(rt_device_t dev, void *buf)
{
    RT_UNUSED(dev);
    RT_UNUSED(buf);
    rt_sem_release(cam_tx_sem);
    return RT_EOK;
}

static rt_err_t scr_cam_in_send(const void *buf, rt_size_t len,
                                rt_uint32_t timeout_ms)
{
    rt_size_t r;

    rt_sem_control(cam_tx_sem, RT_IPC_CMD_RESET, RT_NULL);
    r=rt_device_write(scr_usb_dev, 0, buf, len);
    if(r == 0) {
        return -RT_EBUSY;
    }
    if(rt_sem_take(cam_tx_sem, rt_tick_from_millisecond(timeout_ms))
       != RT_EOK) {
        return -RT_ETIMEOUT;
    }
    return RT_EOK;
}

static void scr_cam_thread_entry(void *parameter)
{
    rt_uint8_t was_cam=0;
    rt_uint8_t seq=0;
    static rt_uint8_t hdr[CAM_HDR_BYTES];

    RT_UNUSED(parameter);

    while(scr_usb_dev == RT_NULL) {
        rt_thread_mdelay(50);
    }
    scr_usb_dev->tx_complete=scr_tx_complete;
    camera_set_frame_hook(scr_cam_frame_hook);

    while(1) {
        if(scr_mode_req != USB2SCR_MODE_CAMERA) {
            if(was_cam) {
                was_cam=0;
                camera_stream_set_active(0);
                LOG_I("camera stream OFF");
            }
            rt_thread_mdelay(100);
            continue;
        }

        if(camera_capture_start() != RT_EOK) {
            LOG_E("camera capture start failed -- requesting STOP");
            usb2scr_request_mode(USB2SCR_MODE_STOP);
            continue;
        }
        if(!was_cam) {
            was_cam=1;
            rt_sem_control(cam_frame_sem, RT_IPC_CMD_RESET, RT_NULL);
            camera_stream_set_active(1);
            LOG_I("camera stream ON");
        }

        if(rt_sem_take(cam_frame_sem, rt_tick_from_millisecond(3000))
           != RT_EOK) {
            continue;
        }

        {
            rt_uint32_t addr=cam_frame_addr;
            rt_size_t   len=cam_frame_len;
            rt_err_t    e1, e2;

            hdr[0]=(rt_uint8_t)CAM_HDR0;
            hdr[1]=(rt_uint8_t)CAM_HDR1;
            hdr[2]=(rt_uint8_t)CAM_HDR2;
            hdr[3]=seq;
            hdr[4]=(rt_uint8_t)(len & 0xFFu);
            hdr[5]=(rt_uint8_t)((len >> 8) & 0xFFu);
            hdr[6]=(rt_uint8_t)((len >> 16) & 0xFFu);
            hdr[7]=(rt_uint8_t)((len >> 24) & 0xFFu);

            rt_mutex_take(scr_in_mtx, RT_WAITING_FOREVER);
            e1=scr_cam_in_send(hdr, CAM_HDR_BYTES, CAM_HDR_TX_TO_MS);
            e2=-RT_ERROR;
            if(e1 == RT_EOK) {
                e2=scr_cam_in_send((const void *)addr, len,
                                     CAM_DAT_TX_TO_MS);
            }
            rt_mutex_release(scr_in_mtx);

            if(e1 == RT_EOK && e2 == RT_EOK) {
                seq++;
                rt_mutex_take(scr_stats_mtx, RT_WAITING_FOREVER);
                scr_stats.tx_frames++;
                rt_mutex_release(scr_stats_mtx);
            } else {
                LOG_W("camera frame %u dropped (hdr=%d dat=%d)", seq, e1, e2);
                rt_thread_mdelay(50);
            }
        }
    }
}

void usb2scr_request_mode(rt_uint8_t mode)
{
    if(mode > USB2SCR_MODE_CAMERA) {
        mode=USB2SCR_MODE_STOP;
    }
    scr_mode_req=mode;

    if((mode == USB2SCR_MODE_MONITOR || mode == USB2SCR_MODE_MIRROR)
       && !scr_active) {
        rt_memset((void *)SCR_FB_ADDR, 0, SCR_FB_SIZE);
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,
                             (void *)SCR_FB_ADDR, SCR_FB_SIZE);
        scr_active=1;
    }
}

void usb2scr_user_exit(void)
{
    usb2scr_request_mode(USB2SCR_MODE_STOP);
}

void usb2scr_get_stats(usb2scr_stats_t *out)
{
    if(out == RT_NULL) {
        return;
    }
    if(scr_stats_mtx == RT_NULL) {
        rt_memset(out, 0, sizeof(*out));
        return;
    }
    rt_mutex_take(scr_stats_mtx, RT_WAITING_FOREVER);
    *out=scr_stats;
    out->active=scr_active;
    out->link_alive=scr_link;
    out->mode_req=scr_mode_req;
    rt_mutex_release(scr_stats_mtx);
}

rt_uint8_t usb2scr_is_active(void)
{
    return scr_active;
}

void usb2screen_init(void)
{
    rt_thread_t tid;

    if(scr_tli_init() != RT_EOK) {
        LOG_E("TLI init failed");
        return;
    }

    scr_sem=rt_sem_create("scr_rx", 0, RT_IPC_FLAG_FIFO);
    scr_stats_mtx=rt_mutex_create("scr_st", RT_IPC_FLAG_PRIO);
    scr_in_mtx=rt_mutex_create("scr_in", RT_IPC_FLAG_PRIO);
    cam_tx_sem=rt_sem_create("cam_tx", 0, RT_IPC_FLAG_FIFO);
    cam_frame_sem=rt_sem_create("cam_rdy", 0, RT_IPC_FLAG_FIFO);
    if(scr_sem == RT_NULL || scr_stats_mtx == RT_NULL ||
       scr_in_mtx == RT_NULL || cam_tx_sem == RT_NULL ||
       cam_frame_sem == RT_NULL) {
        LOG_E("create scr IPC failed");
        return;
    }
    rt_memset(&scr_stats, 0, sizeof(scr_stats));

    tid=rt_thread_create("scr_disp", scr_disp_thread_entry, RT_NULL,
                           2048, 16, 20);
    if(tid != RT_NULL) {
        rt_thread_startup(tid);
    } else {
        LOG_E("create scr_disp thread failed");
    }

    tid=rt_thread_create("scr_ctl", scr_ctl_thread_entry, RT_NULL,
                           2048, 18, 20);
    if(tid != RT_NULL) {
        rt_thread_startup(tid);
    } else {
        LOG_E("create scr_ctl thread failed");
    }

    tid=rt_thread_create("scr_cam", scr_cam_thread_entry, RT_NULL,
                           2048, 17, 20);
    if(tid != RT_NULL) {
        rt_thread_startup(tid);
    } else {
        LOG_E("create scr_cam thread failed");
    }
}
