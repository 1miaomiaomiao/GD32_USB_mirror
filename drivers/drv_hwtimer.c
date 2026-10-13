/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-25     iysheng      first version
 * 2025-10-22     kurisaw      optimized configuration
 */

#include <board.h>
#include <rtdevice.h>
#include "drv_hwtimer.h"
#include "hwtimer_config.h"

#ifdef RT_USING_HWTIMER

/*
 * static void __set_timerx_freq
 * Set freq with timerx
 *
 * @param timerx the pointer of TIMER_TypeDef
 * @param freq of the timer clock
 * @retval None
 */
static void __set_timerx_freq(uint32_t timerx, uint32_t freq)
{
    uint32_t ap1freq, ap2freq;
    uint16_t prescaler;
    uint32_t temp;
    uint32_t timesel;
    uint32_t APBPSC_limit;
    uint32_t multiple;
    timesel = RCU_CFG1 & RCU_CFG1_TIMERSEL;
    if (timesel == 0)
    {
        multiple = 2;
        APBPSC_limit = 0b100;
    }
    else
    {
        multiple = 4;
        APBPSC_limit = 0b101;
    }
    /* APB1 */
    if (!(timerx & (1 << 16)))
    {
        ap1freq = rcu_clock_freq_get(CK_APB1);
        /* Check if APB1 is divided */
        temp = RCU_CFG0 & RCU_CFG0_APB1PSC;
        temp >>= (__builtin_ctz(temp));
        /* When the division factor is less than expected, the clock frequency equals AHB clock, otherwise needs multiplication */
        prescaler = (temp <= APBPSC_limit) ? (rcu_clock_freq_get(CK_AHB) / freq - 1) : ((ap1freq * multiple) / freq - 1);
    }
    else /* APB2 */
    {
        ap2freq = rcu_clock_freq_get(CK_APB2);
        /* Check if APB2 is divided */
        temp = RCU_CFG0 & RCU_CFG0_APB2PSC;
        temp >>= (__builtin_ctz(temp));
        /* When the division factor is less than expected, the clock frequency equals AHB clock, otherwise needs multiplication */
        prescaler = (temp <= APBPSC_limit) ? (rcu_clock_freq_get(CK_AHB) / freq - 1) : ((ap2freq * multiple) / freq - 1);
    }
    timer_prescaler_config(timerx, prescaler, TIMER_PSC_RELOAD_NOW);
}

static void gd32_hwtimer_init(struct rt_hwtimer_device *timer, rt_uint32_t state)
{
    uint32_t timer_base = (uint32_t)timer->parent.user_data;
    timer_parameter_struct initpara;
    if (state)
    {
        /* Set internal clock source */
        timer_internal_clock_config(timer_base);
        /* Initialize timer configuration */
        timer_struct_para_init(&initpara);
        /* Set maximum timer period */
        initpara.period = timer->info->maxcnt;
        /* Configure timer */
        timer_init(timer_base, &initpara);
        /* Set timer input frequency */
        __set_timerx_freq(timer_base, timer->info->maxfreq);
    }
}

static rt_err_t gd32_hwtimer_start(struct rt_hwtimer_device *timer,
                                   rt_uint32_t cnt, rt_hwtimer_mode_t mode)
{
    uint32_t timer_base = (uint32_t)timer->parent.user_data;
    if (mode == HWTIMER_MODE_ONESHOT)
    {
        timer_single_pulse_mode_config(timer_base, TIMER_SP_MODE_SINGLE);
    }
    else if (mode == HWTIMER_MODE_PERIOD)
    {
        timer_single_pulse_mode_config(timer_base, TIMER_SP_MODE_REPETITIVE);
    }
    /* Reset timer count value */
    timer_counter_value_config(timer_base, 0);
    /* Set timer auto-reload value */
    timer_autoreload_value_config(timer_base, cnt - 1);
    /* Enable timer */
    timer_enable(timer_base);
    return 0;
}

static void gd32_hwtimer_stop(struct rt_hwtimer_device *timer)
{
    uint32_t timer_base = (uint32_t)timer->parent.user_data;
    timer_disable(timer_base);
}

static rt_uint32_t gd32_hwtimer_count_get(struct rt_hwtimer_device *timer)
{
    uint32_t timer_base = (uint32_t)timer->parent.user_data;
    rt_uint32_t count;
    count = timer_counter_read(timer_base);
    return count;
}

static rt_err_t gd32_hwtimer_control(struct rt_hwtimer_device *timer, rt_uint32_t cmd,
                                     void *args)
{
    int ret = RT_EOK;
    rt_int32_t freq;
    switch (cmd)
    {
    case HWTIMER_CTRL_FREQ_SET:
        freq = *(rt_uint32_t *)args;
        __set_timerx_freq((uint32_t)timer->parent.user_data, freq);
        break;
    default:
        rt_kprintf("invalid cmd:%x\n", cmd);
        ret = -RT_EINVAL;
        break;
    }
    return ret;
}

static const struct rt_hwtimer_ops g_gd32_hwtimer_ops = {
    gd32_hwtimer_init,
    gd32_hwtimer_start,
    gd32_hwtimer_stop,
    gd32_hwtimer_count_get,
    gd32_hwtimer_control,
};

static gd32_hwtimer_device g_gd32_hwtimer[] = {
#ifdef BSP_USING_HWTIMER0
    HWTIMER0_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER1
    HWTIMER1_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER2
    HWTIMER2_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER3
    HWTIMER3_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER4
    HWTIMER4_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER5
    HWTIMER5_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER6
    HWTIMER6_CONFIG,
#endif
#ifdef BSP_USING_HWTIMER7
    HWTIMER7_CONFIG,
#endif
};

#ifdef BSP_USING_HWTIMER0
void TIMER0_UP_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM0_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM0_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER1
void TIMER1_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM1_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM1_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER2
void TIMER2_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM2_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM2_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER3
void TIMER3_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM3_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM3_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER4
void TIMER4_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM4_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM4_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER5
void TIMER5_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM5_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM5_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER6
void TIMER6_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM6_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM6_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif
#ifdef BSP_USING_HWTIMER7
void TIMER7_UP_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_device_hwtimer_isr(&g_gd32_hwtimer[TIM7_INDEX].hwtimer_dev);
    timer_flag_clear((uint32_t)g_gd32_hwtimer[TIM7_INDEX].hwtimer_dev.parent.user_data, TIMER_INT_UP);
    rt_interrupt_leave();
}
#endif

static int rt_hwtimer_init(void)
{
    int ret = 0, i = 0;
    for (; i < sizeof(g_gd32_hwtimer) / sizeof(g_gd32_hwtimer[0]); i++)
    {
        g_gd32_hwtimer[i].hwtimer_dev.ops = &g_gd32_hwtimer_ops;
        g_gd32_hwtimer[i].hwtimer_dev.info = &g_gd32_hwtimer[i].hwtimer_info;
        /* Enable clock */
        rcu_periph_clock_enable(g_gd32_hwtimer[i].hw_data.rcu);
        /* Set timer interrupt priority */
        NVIC_SetPriority(g_gd32_hwtimer[i].hw_data.irqn, 0);
        /* Enable NVIC */
        NVIC_EnableIRQ(g_gd32_hwtimer[i].hw_data.irqn);
        /* Enable timer interrupt */
        timer_interrupt_enable(g_gd32_hwtimer[i].hw_data.reg_base, TIMER_INT_UP);
        /* Register device to system */
        ret = rt_device_hwtimer_register(&g_gd32_hwtimer[i].hwtimer_dev,
                                         g_gd32_hwtimer[i].dev_name, (void *)g_gd32_hwtimer[i].hw_data.reg_base);
        if (RT_EOK != ret)
        {
            rt_kprintf("failed register %s, err=%d\n", g_gd32_hwtimer[i].dev_name,
                       ret);
            break;
        }
    }
    return ret;
}
INIT_BOARD_EXPORT(rt_hwtimer_init);

#endif
