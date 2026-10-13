#include "dci_ov2640.h"
#include "dci_ov2640_init_table.h"
#include "gd32h7xx.h"
#include <rtthread.h>

#define delay_1ms(ms)   rt_thread_mdelay(ms)

void dci_config(void)
{
    dci_parameter_struct dci_struct;
    dma_single_data_parameter_struct  dma_single_struct;;
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOH);
    rcu_periph_clock_enable(RCU_GPIOG);
    rcu_periph_clock_enable(RCU_DCI);

    gpio_af_set(GPIOE, GPIO_AF_13, GPIO_PIN_3);
    gpio_af_set(GPIOB, GPIO_AF_13, GPIO_PIN_7);
    gpio_af_set(GPIOA, GPIO_AF_13, GPIO_PIN_4);

    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_3);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_7);

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_4);

    gpio_af_set(GPIOC, GPIO_AF_13, GPIO_PIN_6);
    gpio_af_set(GPIOC, GPIO_AF_13, GPIO_PIN_8);
    gpio_af_set(GPIOC, GPIO_AF_13, GPIO_PIN_9);
    gpio_af_set(GPIOB, GPIO_AF_13, GPIO_PIN_6);
    gpio_af_set(GPIOE, GPIO_AF_13, GPIO_PIN_4);
    gpio_af_set(GPIOE, GPIO_AF_13, GPIO_PIN_5);
    gpio_af_set(GPIOE, GPIO_AF_13, GPIO_PIN_6);
    gpio_af_set(GPIOH, GPIO_AF_13, GPIO_PIN_10);

    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_10);

    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_6);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_9);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_8);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_6);

    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_4);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_5);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_6);

    dci_struct.capture_mode=DCI_CAPTURE_MODE_CONTINUOUS;
    dci_struct.clock_polarity= DCI_CK_POLARITY_RISING;
    dci_struct.hsync_polarity=DCI_HSYNC_POLARITY_LOW;
    dci_struct.vsync_polarity=DCI_VSYNC_POLARITY_LOW;
    dci_struct.frame_rate=DCI_FRAME_RATE_ALL;
    dci_struct.interface_format=DCI_INTERFACE_FORMAT_8BITS;
    dci_init(&dci_struct);

    rcu_periph_clock_enable(RCU_DMA1);
    rcu_periph_clock_enable(RCU_DMAMUX);

    dma_single_data_para_struct_init(&dma_single_struct);
    dma_deinit(DMA1, DMA_CH7);
    dma_single_struct.request=DMA_REQUEST_DCI;
    dma_single_struct.periph_addr=(uint32_t)DCI_DATA_ADDRESS;
    dma_single_struct.memory0_addr=(uint32_t)0xC0000000;
    dma_single_struct.direction=DMA_PERIPH_TO_MEMORY;
    dma_single_struct.number=38400;
    dma_single_struct.periph_inc=DMA_PERIPH_INCREASE_DISABLE;
    dma_single_struct.memory_inc=DMA_MEMORY_INCREASE_ENABLE;
    dma_single_struct.periph_memory_width=DMA_PERIPH_WIDTH_32BIT;
    dma_single_struct.circular_mode=DMA_CIRCULAR_MODE_ENABLE;
    dma_single_struct.priority=DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH7, &dma_single_struct);
}

uint8_t ov2640_outsize_set(uint16_t width, uint16_t height)
{
    uint16_t outh;
    uint16_t outw;
    uint8_t temp;
    if(width % 4) {
        return 0xFF;
    }
    if(height % 4) {
        return 0xFF;
    }
    outw=width / 4;
    outh=height / 4;
    dci_byte_write(0xFF, 0x00);
    dci_byte_write(0xE0, 0x04);
    dci_byte_write(0x5A, outw & 0xFF);
    dci_byte_write(0x5B, outh & 0xFF);
    temp=(outw >> 8) & 0x03;
    temp |= (outh >> 6) & 0x04;
    dci_byte_write(0x5C, temp);
    dci_byte_write(0xE0, 0x00);
    return 0;
}

uint8_t dci_ov2640_init(void)
{
    uint8_t i;
    sccb_config();
    dci_config();

    ckout0_init();
    delay_1ms(100);

    if(dci_byte_write(0xFF, 0x01) != 0) {
        return 0xFF;
    }
    if(dci_byte_write(0x12, 0x80) != 0) {
        return 0xFF;
    }
    delay_1ms(10);
    for(i=0; i < sizeof(ov2640_svga_init_reg_tbl) / 2; i++) {
        if(0 != dci_byte_write(ov2640_svga_init_reg_tbl[i][0], ov2640_svga_init_reg_tbl[i][1])) {
            return 0xFF;
        }
    }

    delay_1ms(100);
    for(i=0; i < (sizeof(ov2640_rgb565_reg_tbl) / 2); i++) {
        if(0 != dci_byte_write(ov2640_rgb565_reg_tbl[i][0], ov2640_rgb565_reg_tbl[i][1])) {
            return 0xFF;
        }
    }
    delay_1ms(100);
    ov2640_outsize_set(320, 240);
    return 0;
}

void ckout0_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_af_set(GPIOA, GPIO_AF_CKOUT, GPIO_PIN_8);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_8);

    rcu_ckout0_config(RCU_CKOUT0SRC_HXTAL, RCU_CKOUT0_DIV3);
}


