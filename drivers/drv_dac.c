/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-11     kurisaw      first version
 */

#include "drv_dac.h"
#include "dac_config.h"

#ifdef RT_USING_DAC

#define DBG_TAG "drv.dac"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct gd32_dac gd32_dac_obj[] = {

#ifdef BSP_USING_DAC0
    DAC0_CONFIG,
#endif

#ifdef BSP_USING_DAC1
    DAC1_CONFIG,
#endif

};

/* DAC initialization status */
typedef struct
{
    uint32_t dac_periph;
    rt_bool_t initialized;
} DAC_INIT_STATUS;

static DAC_INIT_STATUS dac_init_status[] = {
#if defined(DAC0)
    { DAC0, RT_FALSE },
#elif defined(DAC1)
    { DAC1, RT_FALSE },
#endif
};

/* Private functions */
static void gd32_dac_hw_init(struct gd32_dac *gd32_dac)
{
    rt_uint32_t dac_port, dac_pin;
    rcu_periph_enum dac_periph;

    /* Get pin configuration */
    if (get_pin_config(gd32_dac->pin_name, &dac_port, &dac_pin, &dac_periph) == -RT_ERROR)
    {
        LOG_E("Failed to get pin config for %s", gd32_dac->pin_name);
        return;
    }

    /* Enable clocks */
    rcu_periph_clock_enable(RCU_DAC);
    rcu_periph_clock_enable(dac_periph);

    /* Configure GPIO as analog mode for DAC output */
    gpio_mode_set(dac_port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, dac_pin);

    LOG_D("DAC GPIO %s configured", gd32_dac->pin_name);
}

static void dac_initialize(uint32_t dac_periph)
{
    /* Check if DAC is already initialized */
    for (int i = 0; i < sizeof(dac_init_status) / sizeof(dac_init_status[0]); i++)
    {
        if (dac_init_status[i].dac_periph == dac_periph && !dac_init_status[i].initialized)
        {
            /* Initialize DAC */
            dac_deinit(dac_periph);

            /* DAC trigger disable for both channels */
            dac_trigger_disable(dac_periph, gd32_dac_obj[i].channel);

            /* DAC wave mode disable for both channels */
            dac_wave_mode_config(dac_periph, gd32_dac_obj[i].channel, DAC_WAVE_DISABLE);

#if defined(SOC_SERIES_GD32F10x) || defined(SOC_SERIES_GD32F20x) || defined(SOC_SERIES_GD32F30x) || defined(SOC_SERIES_GD32F4xx) || defined(SOC_SERIES_GD32F5xx) || defined(SOC_SERIES_GD32E50x)
            /* DAC output buffer enable */
            dac_output_buffer_enable(dac_periph, gd32_dac_obj[i].channel);
#endif

#if defined(SOC_SERIES_GD32H7xx) || defined(SOC_SERIES_GD32H75E)
            /* DAC mode configuration - normal mode with buffer enabled */
            dac_mode_config(dac_periph, gd32_dac_obj[i].channel, NORMAL_PIN_BUFFON);
#endif

            dac_init_status[i].initialized = RT_TRUE;
            break;
        }
    }
}

static rt_err_t drv_dac_enabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    struct gd32_dac *dac_dev = (struct gd32_dac *)device;

    /* Validate channel */
    if (channel != DAC_OUT0 && channel != DAC_OUT1)
    {
        LOG_E("Invalid DAC channel: %d", channel);
        return -RT_EINVAL;
    }

    /* Enable DAC channel */
    dac_enable(dac_dev->dac_periph, (uint8_t)channel);
    LOG_D("DAC %s channel %d enabled", dac_dev->name, channel);

    return RT_EOK;
}

static rt_err_t drv_dac_disabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    struct gd32_dac *dac_dev = (struct gd32_dac *)device;

    /* Validate channel */
    if (channel != DAC_OUT0 && channel != DAC_OUT1)
    {
        LOG_E("Invalid DAC channel: %d", channel);
        return -RT_EINVAL;
    }

    /* Disable DAC channel */
    dac_disable(dac_dev->dac_periph, (uint8_t)channel);
    LOG_D("DAC %s channel %d disabled", dac_dev->name, channel);

    return RT_EOK;
}

static rt_err_t drv_dac_convert(struct rt_dac_device *device, rt_uint32_t channel, rt_uint32_t *value)
{
    struct gd32_dac *dac_dev = (struct gd32_dac *)device;

    if (value == RT_NULL)
    {
        LOG_E("DAC value pointer is NULL");
        return -RT_EINVAL;
    }

    /* Validate channel */
    if (channel != DAC_OUT0 && channel != DAC_OUT1)
    {
        LOG_E("Invalid DAC channel: %d", channel);
        return -RT_EINVAL;
    }

    /* Check value range for 12-bit DAC */
    if (*value > 4095)
    {
        LOG_E("DAC value %u exceeds maximum 4095", *value);
        return -RT_EINVAL;
    }

    /* Set DAC data with 12-bit right aligned */
    dac_data_set(dac_dev->dac_periph, (uint8_t)channel, DAC_ALIGN_12B_R, (uint16_t)(*value));
    LOG_D("DAC %s channel %d set to value %u", dac_dev->name, channel, *value);

    return RT_EOK;
}

static rt_err_t drv_dac_set_mode(struct rt_dac_device *device, rt_uint32_t channel, rt_uint32_t mode)
{
    struct gd32_dac *dac_dev = (struct gd32_dac *)device;

    if (channel != DAC_OUT0 && channel != DAC_OUT1)
    {
        return -RT_EINVAL;
    }

    dac_mode_config(dac_dev->dac_periph, channel, mode);
    return RT_EOK;
}

static struct rt_dac_ops drv_ops = {
    .disabled = drv_dac_disabled,
    .enabled = drv_dac_enabled,
    .convert = drv_dac_convert,
};

static int drv_dac_init(void)
{
    int i, result = RT_EOK;

    for (i = 0; i < sizeof(gd32_dac_obj) / sizeof(gd32_dac_obj[0]); i++)
    {
        /* Initialize DAC hardware - GPIO configuration */
        gd32_dac_hw_init(&gd32_dac_obj[i]);

        /* Initialize DAC peripheral if not already initialized */
        dac_initialize(gd32_dac_obj[i].dac_periph);

        /* Register DAC device */
        if (rt_hw_dac_register(&gd32_dac_obj[i].dac_device, gd32_dac_obj[i].name,
                               &drv_ops, RT_NULL) == RT_EOK)
        {
            LOG_D("%s register success (channel %d)",
                  gd32_dac_obj[i].name, gd32_dac_obj[i].channel);
        }
        else
        {
            LOG_E("%s register failed", gd32_dac_obj[i].name);
            result = -RT_ERROR;
        }
    }

    return result;
}
INIT_DEVICE_EXPORT(drv_dac_init);

#endif /* RT_USING_DAC */
