/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2023-06-05     zengjianwei       first version
 * 2025-06-23     Yucai Liu         Support for non-complementary PWM output with advanced timers
 * 2025-10-11     kurisaw           compatible with RT-Studio
 */

#include "drv_pwm.h"
#include "pwm_config.h"

#ifdef RT_USING_PWM

#define DBG_TAG "drv.pwm"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define MAX_PERIOD 65535
#define MIN_PERIOD 3
#define MIN_PULSE  2

static struct gd32_pwm gd32_pwm_obj[] = {

#ifdef BSP_USING_PWM0
    PWM0_CONFIG,
#endif

#ifdef BSP_USING_PWM1
    PWM1_CONFIG,
#endif

#ifdef BSP_USING_PWM2
    PWM2_CONFIG,
#endif

#ifdef BSP_USING_PWM3
    PWM3_CONFIG,
#endif

#ifdef BSP_USING_PWM4
    PWM4_CONFIG,
#endif

#ifdef BSP_USING_PWM5
    PWM5_CONFIG,
#endif

#ifdef BSP_USING_PWM6
    PWM6_CONFIG,
#endif

#ifdef BSP_USING_PWM7
    PWM7_CONFIG,
#endif

};

typedef struct
{
    rt_uint32_t timer_periph;
    rt_bool_t initialized;
} TIMER_INIT_STATUS;

static TIMER_INIT_STATUS timer_init_status[] = {
    { TIMER0, RT_FALSE },
    { TIMER1, RT_FALSE },
    { TIMER2, RT_FALSE },
    { TIMER3, RT_FALSE },
    { TIMER4, RT_FALSE },
    { TIMER5, RT_FALSE },
    { TIMER6, RT_FALSE },
    { TIMER7, RT_FALSE },
};

static void gd32_pwm_init(struct gd32_pwm *gd32_pwm)
{
    rt_uint32_t pwm_port, pwm_pin;
    rcu_periph_enum pwm_periph;
    rt_uint32_t pin_af;

    if (get_pin_config(gd32_pwm->pin_name, &pwm_port, &pwm_pin, &pwm_periph) == -RT_ERROR)
    {
        return;
    }

    pin_alternate_config(gd32_pwm->alternate, &pin_af);

    /* enable timer clock */
    rcu_periph_clock_enable(gd32_pwm->timer_clk);
    rcu_periph_clock_enable(pwm_periph);

    /* GPIO pin configuration */
    gpio_af_set(pwm_port, pin_af, pwm_pin);
    gpio_mode_set(pwm_port, GPIO_MODE_AF, GPIO_PUPD_NONE, pwm_pin);
#if defined(SOC_SERIES_GD32H7xx) || defined(SOC_SERIES_GD32H75E)
    gpio_output_options_set(pwm_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, pwm_pin);
#else
    gpio_output_options_set(pwm_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pwm_pin);
#endif
}

static rt_err_t drv_pwm_enable(struct gd32_pwm *pwm_dev, struct rt_pwm_configuration *configuration, rt_bool_t enable)
{
    if (!enable)
    {
        timer_channel_output_state_config(pwm_dev->timer_periph, configuration->channel, TIMER_CCX_DISABLE);
    }
    else
    {
        if (configuration->complementary == RT_TRUE)
        {
            timer_channel_output_state_config(pwm_dev->timer_periph, configuration->channel - 1, TIMER_CCXN_ENABLE);
        }
        else
        {
            timer_channel_output_state_config(pwm_dev->timer_periph, configuration->channel, TIMER_CCX_ENABLE);
        }
    }

    return RT_EOK;
}

static rt_err_t drv_pwm_get(struct gd32_pwm *pwm_dev, struct rt_pwm_configuration *configuration)
{
    rt_uint64_t tim_clock;
    rt_uint16_t psc;
    rt_uint32_t chxcv;

    tim_clock = rcu_clock_freq_get(CK_SYS);

    psc = timer_prescaler_read(pwm_dev->timer_periph);
    if (psc == TIMER_CKDIV_DIV2)
    {
        tim_clock = tim_clock / 2;
    }
    else if (psc == TIMER_CKDIV_DIV4)
    {
        tim_clock = tim_clock / 4;
    }

    chxcv = timer_channel_capture_value_register_read(pwm_dev->timer_periph, configuration->channel);
    /* Convert nanosecond to frequency and duty cycle. 1s = 1 * 1000 * 1000 * 1000 ns */
    tim_clock /= 1000000UL;
    configuration->period = (TIMER_CAR(pwm_dev->timer_periph) + 1) * (psc + 1) * 1000UL / tim_clock;
    configuration->pulse = (chxcv + 1) * (psc + 1) * 1000UL / tim_clock;

    return RT_EOK;
}

static rt_err_t drv_pwm_set(struct gd32_pwm *pwm_dev, struct rt_pwm_configuration *configuration)
{
    rt_uint32_t period, pulse;
    rt_uint64_t tim_clock, psc;

    tim_clock = rcu_clock_freq_get(CK_SYS);

    /* Convert nanosecond to frequency and duty cycle. 1s = 1 * 1000 * 1000 * 1000 ns */
    tim_clock /= 1000000UL;
    period = (unsigned long long)configuration->period * tim_clock / 1000ULL;
    psc = period / MAX_PERIOD + 1;
    period = period / psc;

    timer_prescaler_config(pwm_dev->timer_periph, psc - 1, TIMER_PSC_RELOAD_NOW);

    if (period < MIN_PERIOD)
    {
        period = MIN_PERIOD;
    }

    timer_autoreload_value_config(pwm_dev->timer_periph, period - 1);

    pulse = (unsigned long long)configuration->pulse * tim_clock / psc / 1000ULL;
    if (pulse < MIN_PULSE)
    {
        pulse = MIN_PULSE;
    }
    else if (pulse > period)
    {
        pulse = period;
    }

    timer_channel_output_pulse_value_config(pwm_dev->timer_periph, configuration->channel, pulse);
    timer_counter_value_config(pwm_dev->timer_periph, 0);

    /* Update frequency value */
    timer_event_software_generate(pwm_dev->timer_periph, TIMER_EVENT_SRC_UPG);

    return RT_EOK;
}

static rt_err_t drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;
    struct gd32_pwm *pwm_dev = (struct gd32_pwm *)device;

    switch (cmd)
    {
    case PWM_CMD_ENABLE:
        return drv_pwm_enable(pwm_dev, configuration, RT_TRUE);
    case PWM_CMD_DISABLE:
        return drv_pwm_enable(pwm_dev, configuration, RT_FALSE);
    case PWM_CMD_SET:
        return drv_pwm_set(pwm_dev, configuration);
    case PWM_CMD_GET:
        return drv_pwm_get(pwm_dev, configuration);
    default:
        return -RT_EINVAL;
    }
}

static struct rt_pwm_ops drv_ops = {
    .control = drv_pwm_control
};

static void timer_initialize(uint32_t timer_periph)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    /* Check if timer is already initialized */
    for (int i = 0; i < sizeof(timer_init_status) / sizeof(timer_init_status[0]); i++)
    {
        if (timer_init_status[i].timer_periph == timer_periph && !timer_init_status[i].initialized)
        {
            /* TIMER configuration */
            timer_initpara.prescaler = 119;
            timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
            timer_initpara.counterdirection = TIMER_COUNTER_UP;
            timer_initpara.period = 15999;
            timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
            timer_initpara.repetitioncounter = 0;
            timer_init(timer_periph, &timer_initpara);

            /* CHX configuration in PWM mode */
            timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
            timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
            timer_ocintpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
            timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
            timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
            timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
            timer_channel_output_config(gd32_pwm_obj[i].timer_periph, gd32_pwm_obj[i].channel, &timer_ocintpara);

            timer_init_status[i].initialized = RT_TRUE;

            /* Configure channel output */
            timer_channel_output_pulse_value_config(gd32_pwm_obj[i].timer_periph,
                                                    gd32_pwm_obj[i].channel, 7999);
            timer_channel_output_mode_config(gd32_pwm_obj[i].timer_periph,
                                             gd32_pwm_obj[i].channel, TIMER_OC_MODE_PWM0);
            timer_channel_output_shadow_config(gd32_pwm_obj[i].timer_periph,
                                               gd32_pwm_obj[i].channel, TIMER_OC_SHADOW_DISABLE);

            /* Enable timer */
            timer_primary_output_config(timer_periph, ENABLE);
            /* auto-reload preload enable */
            timer_auto_reload_shadow_enable(gd32_pwm_obj[i].timer_periph);
            timer_enable(timer_periph);
            break;
        }
    }
}

static int drv_pwm_init(void)
{
    int i, result = RT_EOK;

    for (i = 0; i < sizeof(gd32_pwm_obj) / sizeof(gd32_pwm_obj[0]); i++)
    {
        /* Initialize PWM hardware */
        gd32_pwm_init(&gd32_pwm_obj[i]);

        /* Initialize timer if not already initialized */
        timer_initialize(gd32_pwm_obj[i].timer_periph);

        /* Register PWM device */
        if (rt_device_pwm_register(&gd32_pwm_obj[i].pwm_device, gd32_pwm_obj[i].name,
                                   &drv_ops, RT_NULL) == RT_EOK)
        {
            LOG_D("%s register success", gd32_pwm_obj[i].name);
        }
        else
        {
            LOG_E("%s register failed", gd32_pwm_obj[i].name);
            result = -RT_ERROR;
        }
    }

    return result;
}
INIT_DEVICE_EXPORT(drv_pwm_init);

#endif /* RT_USING_PWM */
