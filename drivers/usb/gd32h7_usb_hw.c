/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#include "drv_usb_hw.h"
#include "drv_usbd_int.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#if defined(BSP_USING_USB)

#define TIM_MSEC_DELAY 0x01U
#define TIM_USEC_DELAY 0x02U

#if defined(RT_USING_USB_HOST)
#ifndef USE_ULPI_PHY

#ifdef USE_USBHS0
#if defined(SOC_SERIES_GD32H75E)
#define HOST_POWERSW_PORT_RCC RCU_GPIOG
#define HOST_POWERSW_PORT     GPIOG
#define HOST_POWERSW_VBUS     GPIO_PIN_8
#elif defined(SOC_SERIES_GD32H7xx)
#warning "Please refer to the relevant sections in the chip manual to redefine the pins."
#define HOST_POWERSW_PORT_RCC RCU_GPIOB
#define HOST_POWERSW_PORT     GPIOB
#define HOST_POWERSW_VBUS     GPIO_PIN_13
#endif
#endif

#ifdef USE_USBHS1
#define HOST_POWERSW_PORT_RCC RCU_GPIOG
#define HOST_POWERSW_PORT     GPIOG
#define HOST_POWERSW_VBUS     GPIO_PIN_7
#endif

#else

#ifdef USE_USBHS0
#define HOST_POWERSW_PORT_RCC RCU_GPIOC
#define HOST_POWERSW_PORT     GPIOC
#define HOST_POWERSW_VBUS     GPIO_PIN_7
#endif

#ifdef USE_USBHS1
#define HOST_POWERSW_PORT_RCC RCU_GPIOC
#define HOST_POWERSW_PORT     GPIOC
#define HOST_POWERSW_VBUS     GPIO_PIN_9
#endif

#endif

#define HOST_SOF_OUTPUT_RCC RCC_APB2PERIPH_GPIOA
#define HOST_SOF_PORT       GPIOA
#define HOST_SOF_SIGNAL     GPIO_PIN_8
#endif

__IO uint32_t delay_time = 0U;
__IO uint16_t timer_prescaler = 23U;

/* local function prototypes ('static') */
static void hw_time_set(uint8_t unit);
static void hw_delay(uint32_t ntime, uint8_t unit);

void usb_gpio_config(void)
{
#ifdef USE_USBHS0
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    /* ULPI_STP(PC0) GPIO pin configuration */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_0);

    /* ULPI_CK(PA5) GPIO pin configuration */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_5);

    /* ULPI_NXT(PC3) GPIO pin configuration */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_3);

    /* ULPI_DIR(PC2) GPIO pin configuration */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_2);

    /* ULPI_D1(PB0), ULPI_D2(PB1), ULPI_D3(PB10), ULPI_D4(PB11) \
       ULPI_D5(PB12), ULPI_D6(PB13) and ULPI_D7(PB5) GPIO pin configuration */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO_PIN_0 |
                      GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ,
                            GPIO_PIN_12 |
                                GPIO_PIN_13 | GPIO_PIN_5);

    /* ULPI_D0(PA3) GPIO pin configuration */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_3);

    gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_5);
    gpio_af_set(GPIOB, GPIO_AF_10, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    gpio_af_set(GPIOC, GPIO_AF_10, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);

#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    rcu_periph_clock_enable(RCU_GPIOH);
    rcu_periph_clock_enable(RCU_GPIOG);

    /* ULPI_STP(PH2) GPIO pin configuration */
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_2);


    /* ULPI_CK(PH5) GPIO pin configuration */
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_5);

    /* ULPI_NXT(PH4) GPIO pin configuration */
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_4);

    /* ULPI_DIR(PH3) GPIO pin configuration */
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_3);

    /* ULPI_D0(PH6) GPIO pin configuration */
    /* ULPI_D1(PH7), ULPI_D2(PH8), ULPI_D3(PH9), ULPI_D4(PH10) \
       ULPI_D5(PH11), ULPI_D6(PH12) and ULPI_D7(PG5) GPIO pin configuration */
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                      GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ,
                            GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                                GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_5);

    gpio_af_set(GPIOG, GPIO_AF_8, GPIO_PIN_5);
    gpio_af_set(GPIOH, GPIO_AF_8, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
#endif /* USE_USBHS1 */
}

extern usb_core_driver gd_usb_core;

#ifdef USE_USBHS0
void USBHS0_IRQHandler(void)
{
    rt_interrupt_enter();

#if defined(RT_USING_USB_DEVICE)
    usbd_isr(&gd_usb_core);
#else
    usbh_isr(&gd_usb_core);
#endif

    rt_interrupt_leave();
}

#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
void USBHS1_IRQHandler(void)
{
    rt_interrupt_enter();

#if defined(RT_USING_USB_DEVICE)
    usbd_isr(&gd_usb_core);
#else
    usbh_isr(&gd_usb_core);
#endif

    rt_interrupt_leave();
}

#endif /* USE_USBHS1 */

static void resume_mcu_clk(void)
{
    /* enable HXTAL */
    rcu_osci_on(RCU_HXTAL);

    /* wait till HXTAL is ready */
    while (RESET == rcu_flag_get(RCU_FLAG_HXTALSTB))
    {
    }

    /* enable PLL */
    rcu_osci_on(RCU_PLL0_CK);

    /* wait till PLL is ready */
    while (RESET == rcu_flag_get(RCU_FLAG_PLL0STB))
    {
    }

    /* select PLL as system clock source */
    rcu_system_clock_source_config(RCU_CKSYSSRC_PLL0P);

    /* wait till PLL is used as system clock source */
    while (RCU_SCSS_PLL0P != rcu_system_clock_source_get())
    {
    }
}

#ifdef USE_USBHS0
void USBHS0_WKUP_IRQHandler(void)
{
    rt_interrupt_enter();

    if (gd_usb_core.bp.low_power)
    {
        resume_mcu_clk();

#ifndef USE_IRC48M
        rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_PLL0R);
#else
            /* enable IRC48M clock */
        rcu_osci_on(RCU_IRC48M);

            /* wait till IRC48M is ready */
        while (SUCCESS != rcu_osci_stab_wait(RCU_IRC48M))
        {
        }

        rcu_ck48m_clock_config(RCU_CK48MSRC_IRC48M);
#endif /* USE_IRC48M */

        rcu_periph_clock_enable(RCU_USBHS0);

        usb_clock_active(&gd_usb_core);
    }

    exti_interrupt_flag_clear(EXTI_31);

    rt_interrupt_leave();
}

#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
void USBHS1_WKUP_IRQHandler(void)
{
    rt_interrupt_enter();

    if (gd_usb_core.bp.low_power)
    {
        resume_mcu_clk();

#ifndef USE_IRC48M
        rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_PLL0R);
#else
            /* enable IRC48M clock */
        rcu_osci_on(RCU_IRC48M);

            /* wait till IRC48M is ready */
        while (SUCCESS != rcu_osci_stab_wait(RCU_IRC48M))
        {
        }

        rcu_ck48m_clock_config(RCU_CK48MSRC_IRC48M);
#endif /* USE_IRC48M */

        rcu_periph_clock_enable(RCU_USBHS1);

        usb_clock_active(&gd_usb_core);
    }

    exti_interrupt_flag_clear(EXTI_32);

    rt_interrupt_leave();
}

#endif /* USE_USBHS1 */

#ifdef USB_DEDICATED_EP1_ENABLED

#ifdef USE_USBHS0
void USBHS0_EP1_OUT_IRQHandler(void)
{
    usbd_int_dedicated_ep1out(&cdc_acm);
}

#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
void USBHS1_EP1_OUT_IRQHandler(void)
{
    usbd_int_dedicated_ep1out(&cdc_acm);
}

#endif /* USE_USBHS1 */

#ifdef USE_USBHS0
void USBHS0_EP1_IN_IRQHandler(void)
{
    usbd_int_dedicated_ep1in(&cdc_acm);
}

#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
void USBHS1_EP1_IN_IRQHandler(void)
{
    usbd_int_dedicated_ep1in(&cdc_acm);
}

#endif /* USE_USBHS1 */

#endif /* USB_DEDICATED_EP1_ENABLED */

void usb_rcu_config(void)
{
    pmu_usb_regulator_enable();
    pmu_usb_voltage_detector_enable();
    {
        uint32_t to = 0U;
        while ((SET != pmu_flag_get(PMU_FLAG_USB33RF)) && (++to < 1000000U))
        {
        }
        if (to >= 1000000U)
        {
            rt_kprintf("[usb] USB33R regulator (PMU_FLAG_USB33RF) not ready\r\n");
        }
    }

#ifdef USE_USB_FS

#ifndef USE_IRC48M

    /* configure the pll1 input and output clock range */
    rcu_pll_input_output_clock_range_config(IDX_PLL1, RCU_PLL1RNG_4M_8M, RCU_PLL1VCO_192M_836M);

    rcu_pll1_config(5, 96, 2, 10, 2);
    /* enable PLL1Q clock output */
    rcu_pll_clock_output_enable(RCU_PLL1Q);
    rcu_osci_on(RCU_PLL1_CK);

#ifdef USE_USBHS0
    rcu_usbhs_pll1qpsc_config(IDX_USBHS0, RCU_USBHSPSC_DIV1);

    rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_PLL1Q);
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    rcu_usbhs_pll1qpsc_config(IDX_USBHS1, RCU_USBHSPSC_DIV1);

    rcu_usb48m_clock_config(IDX_USBHS1, RCU_USB48MSRC_PLL1Q);
#endif /* USE_USBHS1 */

#else
    /* enable IRC48M clock */
    rcu_osci_on(RCU_IRC48M);

    /* wait till IRC48M is ready */
    while (SUCCESS != rcu_osci_stab_wait(RCU_IRC48M))
    {
    }

    rcu_ck48m_clock_config(RCU_CK48MSRC_IRC48M);

#ifdef USE_USBHS0
    rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_IRC48M);
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    rcu_usb48m_clock_config(IDX_USBHS1, RCU_USB48MSRC_IRC48M);
#endif /* USE_USBHS1 */

#endif /* USE_IRC48M */

#endif /* USE_USB_FS */

#ifdef USE_USBHS0
    rcu_periph_clock_enable(RCU_USBHS0);
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    rcu_periph_clock_enable(RCU_USBHS1);
#endif /* USE_USBHS1 */

#ifdef USE_ULPI_PHY
#ifdef USE_USBHS0
    rcu_periph_clock_enable(RCU_USBHS0ULPI);
#endif

#ifdef USE_USBHS1
    rcu_periph_clock_enable(RCU_USBHS1ULPI);
#endif
#endif /* USE_ULPI_PHY */
}

void usb_intr_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

#ifdef USE_USBHS0
    nvic_irq_enable((uint8_t)USBHS0_IRQn, 3U, 0U);
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    nvic_irq_enable((uint8_t)USBHS1_IRQn, 3U, 0U);
#endif /* USE_USBHS0 */

    /* enable the power module clock */
    rcu_periph_clock_enable(RCU_PMU);

#ifdef USE_USBHS0
    /* USB wakeup EXTI line configuration */
    exti_interrupt_flag_clear(EXTI_31);
    exti_init(EXTI_31, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_enable(EXTI_31);

    nvic_irq_enable((uint8_t)USBHS0_WKUP_IRQn, 1U, 0U);
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    /* USB wakeup EXTI line configuration */
    exti_interrupt_flag_clear(EXTI_32);
    exti_init(EXTI_32, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_enable(EXTI_32);

    nvic_irq_enable((uint8_t)USBHS1_WKUP_IRQn, 1U, 0U);
#endif /* USE_USBHS1 */

#ifdef USB_DEDICATED_EP1_ENABLED

#ifdef USE_USBHS0
    nvic_irq_enable((uint8_t)USBHS0_EP1_OUT_IRQn, 1U, 0U);
    nvic_irq_enable((uint8_t)USBHS0_EP1_IN_IRQn, 1U, 0U);
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
    nvic_irq_enable((uint8_t)USBHS1_EP1_OUT_IRQn, 1U, 0U);
    nvic_irq_enable((uint8_t)USBHS1_EP1_IN_IRQn, 1U, 0U);
#endif /* USE_USBHS1 */

#endif /* USB_DEDICATED_EP1_ENABLED */
}


#if defined(RT_USING_USB_HOST)
void usb_vbus_drive(uint8_t state)
{
    if (0U == state)
    {
        /* disable is needed on output of the power switch */
        gpio_bit_reset(HOST_POWERSW_PORT, HOST_POWERSW_VBUS);
    }
    else
    {
        /* enable the power switch by driving the enable high */
        gpio_bit_set(HOST_POWERSW_PORT, HOST_POWERSW_VBUS);
    }
}

void usb_vbus_config(void)
{
    rcu_periph_clock_enable(HOST_POWERSW_PORT_RCC);

    /* USBFS_VBUS_CTRL(PD13) GPIO pin configuration */
    gpio_mode_set(HOST_POWERSW_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, HOST_POWERSW_VBUS);
    gpio_output_options_set(HOST_POWERSW_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, HOST_POWERSW_VBUS);

    /* by default, disable is needed on output of the power switch */
    usb_vbus_drive(0U);

    /* delay is need for stabilizing the VBUS low in reset condition,
     * when VBUS = 1 and reset-button is pressed by user
     */
    usb_mdelay(200);
}

#endif

#if defined(BSP_USING_TIMER2) && defined(BSP_USING_USB)
#error "Timer2 has been occupied by USB. Please select another timer resource."
#endif
void usb_timer_init(void)
{
    /* configure the priority group to 2 bits */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

    /* enable the TIM2 global interrupt */
    nvic_irq_enable((uint8_t)TIMER2_IRQn, 1U, 0U);

    rcu_periph_clock_enable(RCU_TIMER2);
}

void usb_udelay(const uint32_t usec)
{
    hw_delay(usec, TIM_USEC_DELAY);
}

void usb_mdelay(const uint32_t msec)
{
    hw_delay(msec, TIM_MSEC_DELAY);
}

void usb_timer_irq(void)
{
    if (RESET != timer_interrupt_flag_get(TIMER2, TIMER_INT_UP))
    {
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_UP);

        if (delay_time > 0x00U)
        {
            delay_time--;
        }
        else
        {
            timer_disable(TIMER2);
        }
    }
}

void TIMER2_IRQHandler(void)
{
    rt_interrupt_enter();

    usb_timer_irq();

    rt_interrupt_leave();
}

static void hw_delay(uint32_t ntime, uint8_t unit)
{
    delay_time = ntime;

    hw_time_set(unit);

    while (0U != delay_time)
    {
    }

    timer_disable(TIMER2);
}

static void hw_time_set(uint8_t unit)
{
    timer_parameter_struct timer_basestructure;

    timer_disable(TIMER2);
    timer_interrupt_disable(TIMER2, TIMER_INT_UP);

    if (TIM_USEC_DELAY == unit)
    {
        timer_basestructure.period = 9U;
    }
    else if (TIM_MSEC_DELAY == unit)
    {
        timer_basestructure.period = 9999U;
    }
    else
    {
        /* no operation */
    }

    timer_basestructure.prescaler = timer_prescaler;
    timer_basestructure.alignedmode = TIMER_COUNTER_EDGE;
    timer_basestructure.counterdirection = TIMER_COUNTER_UP;
    timer_basestructure.clockdivision = TIMER_CKDIV_DIV1;
    timer_basestructure.repetitioncounter = 0U;

    timer_init(TIMER2, &timer_basestructure);

    timer_interrupt_flag_clear(TIMER2, TIMER_INT_UP);

    timer_auto_reload_shadow_enable(TIMER2);

    /* TIMER IT enable */
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);

    /* TIMER2 enable counter */
    timer_enable(TIMER2);
}

int pllusb_rcu_config(uint32_t usb_periph)
{
    uint32_t to;
    if (USBHS0 == usb_periph)
    {
        rcu_pllusb0_config(RCU_PLLUSBHSPRE_HXTAL, RCU_PLLUSBHSPRE_DIV5, RCU_PLLUSBHS_MUL96, RCU_USBHS_DIV8);
        RCU_ADDCTL1 |= RCU_ADDCTL1_PLLUSBHS0EN;
        to = 0U;
        while ((0U == (RCU_ADDCTL1 & RCU_ADDCTL1_PLLUSBHS0STB)) && (++to < 2000000U))
        {
        }
        if (to >= 2000000U)
        {
            rt_kprintf("[usb] USBHS0 PLLUSBHS failed to lock (ADDCTL1=0x%08x)\r\n", RCU_ADDCTL1);
            return -1;
        }

        rcu_usbhs_clock_selection_enable(IDX_USBHS0);
        rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_PLLUSBHS);
        rcu_usbhs_clock_config(IDX_USBHS0, RCU_USBHSSEL_60M);
    }
    else
    {
        rcu_pllusb1_config(RCU_PLLUSBHSPRE_HXTAL, RCU_PLLUSBHSPRE_DIV5, RCU_PLLUSBHS_MUL96, RCU_USBHS_DIV8);
        RCU_ADDCTL1 |= RCU_ADDCTL1_PLLUSBHS1EN;
        to = 0U;
        while ((0U == (RCU_ADDCTL1 & RCU_ADDCTL1_PLLUSBHS1STB)) && (++to < 2000000U))
        {
        }
        if (to >= 2000000U)
        {
            rt_kprintf("[usb] USBHS1 PLLUSBHS failed to lock (ADDCTL1=0x%08x)\r\n", RCU_ADDCTL1);
            return -1;
        }

        rcu_usbhs_clock_selection_enable(IDX_USBHS1);
        rcu_usb48m_clock_config(IDX_USBHS1, RCU_USB48MSRC_PLLUSBHS);
        rcu_usbhs_clock_config(IDX_USBHS1, RCU_USBHSSEL_60M);
    }
    rt_kprintf("[usb] HS PLL ready, USB clock configured\r\n");
    return 0;
}

#endif /* BSP_USING_USB */
