#include "sccb.h"
#include <rtthread.h>

#define delay_1ms(ms)   rt_thread_mdelay(ms)

static rt_mutex_t sccb_mtx=RT_NULL;

void sccb_config(void)
{
    rcu_i2c_clock_config(IDX_I2C1,RCU_I2CSRC_IRC64MDIV);

    rcu_periph_clock_enable(DCI_SCCB_SCL_GPIO_CLK);
    rcu_periph_clock_enable(DCI_SCCB_SDA_GPIO_CLK);
    rcu_periph_clock_enable(DCI_SCCB_CLK);

    gpio_af_set(DCI_SCCB_SDA_GPIO_PORT, DCI_SCCB_SDA_AF, DCI_SCCB_SDA_PIN);
    gpio_af_set(DCI_SCCB_SCL_GPIO_PORT, DCI_SCCB_SCL_AF, DCI_SCCB_SCL_PIN);

    gpio_mode_set(DCI_SCCB_SDA_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DCI_SCCB_SDA_PIN);
    gpio_output_options_set(DCI_SCCB_SDA_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_12MHZ, DCI_SCCB_SDA_PIN);

    gpio_mode_set(DCI_SCCB_SCL_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DCI_SCCB_SCL_PIN);
    gpio_output_options_set(DCI_SCCB_SCL_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_12MHZ, DCI_SCCB_SCL_PIN);

    i2c_deinit(DCI_SCCB);
    i2c_timing_config(DCI_SCCB, 0, 6, 0);
    i2c_master_clock_config(DCI_SCCB, 0x26, 0x73);
    i2c_address_config(DCI_SCCB, SCCB_SLAVE_ADDRESS7, I2C_ADDFORMAT_7BITS);
    i2c_enable(DCI_SCCB);

    if(sccb_mtx == RT_NULL) {
        sccb_mtx=rt_mutex_create("sccb_mtx", RT_IPC_FLAG_PRIO);
    }
}

uint8_t dci_byte_write(uint8_t reg, uint8_t data)
{
    volatile uint32_t timeout;
    uint8_t ret=0;

    rt_mutex_take(sccb_mtx, RT_WAITING_FOREVER);

    i2c_master_addressing(DCI_SCCB, OV2640_DEVICE_WRITE_ADDRESS, I2C_MASTER_TRANSMIT);
    i2c_transfer_byte_number_config(DCI_SCCB, 0x2);
    i2c_automatic_end_enable(DCI_SCCB);
    i2c_start_on_bus(DCI_SCCB);

    i2c_data_transmit(DCI_SCCB, reg);
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_TI)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    i2c_data_transmit(DCI_SCCB, data);
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_TI)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_STPDET)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    i2c_flag_clear(DCI_SCCB, I2C_FLAG_STPDET);

done:
    rt_mutex_release(sccb_mtx);
    return ret;
}

uint8_t dci_byte_read(uint8_t reg, uint8_t *data)
{
    volatile uint32_t timeout;
    uint8_t ret=0;

    rt_mutex_take(sccb_mtx, RT_WAITING_FOREVER);

    i2c_master_addressing(DCI_SCCB, OV2640_DEVICE_READ_ADDRESS, I2C_MASTER_TRANSMIT);
    i2c_transfer_byte_number_config(DCI_SCCB, 0x1);
    i2c_automatic_end_enable(DCI_SCCB);
    i2c_start_on_bus(DCI_SCCB);

    i2c_data_transmit(DCI_SCCB, reg);
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_TI)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_STPDET)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    i2c_flag_clear(DCI_SCCB, I2C_FLAG_STPDET);

    i2c_master_addressing(DCI_SCCB,  OV2640_DEVICE_READ_ADDRESS, I2C_MASTER_RECEIVE);
    i2c_transfer_byte_number_config(DCI_SCCB, 0x1);
    i2c_automatic_end_enable(DCI_SCCB);
        delay_1ms(1);
    i2c_start_on_bus(DCI_SCCB);
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_RBNE)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    *data=i2c_data_receive(DCI_SCCB);
    timeout=SCCB_FLAG_TIMEOUT;
    while((SET != i2c_flag_get(DCI_SCCB, I2C_FLAG_STPDET)) && --timeout) {
    }
    if(0 == timeout) {
        ret=0xFF;
        goto done;
    }
    i2c_flag_clear(DCI_SCCB, I2C_FLAG_STPDET);

done:
    rt_mutex_release(sccb_mtx);
    return ret;
}
