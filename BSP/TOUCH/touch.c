#include "touch.h"
#include <string.h>

static const rt_uint8_t gt911_cfg_params[]={
0x5A,0xE0,0x01,0x10,0x01,0x01,0x8D,0x00,0x01,0x08,0x28,0x05,
0x55,0x32,0x03,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x89,0x2A,0x0B,0x17,0x15,0x31,0x0D,0x00,0x00,
0x02,0xB9,0x04,0x2C,0x00,0x00,0x00,0x00,0x00,0x03,0x64,0x32,
0x00,0x00,0x00,0x0F,0x94,0x94,0xC5,0x02,0x07,0x00,0x00,0x04,
0x8D,0x13,0x00,0x5C,0x1E,0x00,0x3C,0x30,0x00,0x29,0x4C,0x00,
0x20,0x78,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,
0x12,0x14,0x16,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x24,0x22,
0x21,0x20,0x1F,0x1E,0x1D,0x1C,0x18,0x16,0x12,0x10,0x0F,0x0A,
0x08,0x06,0x04,0x02,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0x21,0x01
};

gt911_struct gt911={0};
static rt_uint8_t clear_flag=0;
static rt_uint8_t cfg_buf[sizeof(gt911_cfg_params)]={0};
static rt_uint8_t touch_buf[48];

static void i2c_delay(void)
{
    volatile rt_uint32_t i;
    for(i=0; i < 5 * 100; i++);
}

static void i2c_start(void)
{
    I2C_SDA_1();
    I2C_SCL_1();
    i2c_delay();
    I2C_SDA_0();
    i2c_delay();
    I2C_SCL_0();
    i2c_delay();
}

static void i2c_stop(void)
{
    I2C_SDA_0();
    I2C_SCL_1();
    i2c_delay();
    I2C_SDA_1();
}

static void i2c_sendbyte(rt_uint8_t byte)
{
    rt_uint32_t i;
    for(i=0; i < 8; i++) {
        if(byte & 0x80) {
            I2C_SDA_1();
        } else {
            I2C_SDA_0();
        }
        i2c_delay();
        I2C_SCL_1();
        i2c_delay();
        I2C_SCL_0();
        if(i == 7) {
            I2C_SDA_1();
        }
        byte <<= 1;
        i2c_delay();
    }
}

static rt_uint8_t i2c_readbyte(void)
{
    rt_uint32_t i;
    rt_uint8_t value=0;
    for(i=0; i < 8; i++) {
        value <<= 1;
        I2C_SCL_1();
        i2c_delay();
        if(I2C_SDA_READ()) {
            value++;
        }
        I2C_SCL_0();
        i2c_delay();
    }
    return value;
}

static rt_uint8_t i2c_waitack(void)
{
    rt_uint8_t re;
    I2C_SDA_1();
    i2c_delay();
    I2C_SCL_1();
    i2c_delay();
    if(I2C_SDA_READ()) {
        re=1;
    } else {
        re=0;
    }
    I2C_SCL_0();
    i2c_delay();
    return re;
}

static void i2c_ack(void)
{
    I2C_SDA_0();
    i2c_delay();
    I2C_SCL_1();
    i2c_delay();
    I2C_SCL_0();
    i2c_delay();
    I2C_SDA_1();
}

static void i2c_nack(void)
{
    I2C_SDA_1();
    i2c_delay();
    I2C_SCL_1();
    i2c_delay();
    I2C_SCL_0();
    i2c_delay();
}

static void i2c_gpio_config(void)
{

    rcu_periph_clock_enable(GTP_I2C_SCL_GPIO_CLK);
    gpio_mode_set(GTP_I2C_SCL_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GTP_I2C_SCL_PIN);
    gpio_output_options_set(GTP_I2C_SCL_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GTP_I2C_SCL_PIN);

    rcu_periph_clock_enable(GTP_I2C_SDA_GPIO_CLK);
    gpio_mode_set(GTP_I2C_SDA_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GTP_I2C_SDA_PIN);
    gpio_output_options_set(GTP_I2C_SDA_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GTP_I2C_SDA_PIN);

    rcu_periph_clock_enable(GTP_RST_GPIO_CLK);
    rcu_periph_clock_enable(GTP_INT_GPIO_CLK);

    gpio_mode_set(GTP_RST_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GTP_RST_GPIO_PIN);
    gpio_output_options_set(GTP_RST_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, GTP_RST_GPIO_PIN);

    gpio_mode_set(GTP_INT_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GTP_INT_GPIO_PIN);
    gpio_output_options_set(GTP_INT_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, GTP_INT_GPIO_PIN);
}

static void i2c_resetchip(void)
{
    GTP_RST_0();
    GTP_INT_0();
    rt_thread_mdelay(1);

    GTP_RST_1();
    rt_thread_mdelay(6);

    gpio_mode_set(GTP_INT_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GTP_INT_GPIO_PIN);
}

static void i2c_touch_init(void)
{
    i2c_gpio_config();
    i2c_resetchip();
}

static ErrStatus gt911_readreg(rt_uint16_t addr, rt_uint8_t *buffer, rt_uint8_t length)
{
    rt_uint8_t i;

    i2c_start();
    i2c_sendbyte(GTP_ADDRESS | I2C_DIR_WR);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    i2c_sendbyte(addr >> 8);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    i2c_sendbyte(addr);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    i2c_stop();

    i2c_start();
    i2c_sendbyte(GTP_ADDRESS | I2C_DIR_RD);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    for(i=0; i < length - 1; i++) {
        buffer[i]=i2c_readbyte();
        i2c_ack();
    }
    buffer[i]=i2c_readbyte();
    i2c_nack();
    i2c_stop();

    return SUCCESS;
}

static ErrStatus gt911_writereg(rt_uint16_t addr, rt_uint8_t *buffer, rt_uint8_t length)
{
    rt_uint8_t i;

    i2c_start();
    i2c_sendbyte(GTP_ADDRESS | I2C_DIR_WR);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    i2c_sendbyte(addr >> 8);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    i2c_sendbyte(addr);
    if(i2c_waitack() != 0) {
        i2c_stop();
        return ERROR;
    }
    for(i=0; i < length; i++) {
        i2c_sendbyte(buffer[i]);
        if(i2c_waitack() != 0) {
            i2c_stop();
            return ERROR;
        }
    }
    i2c_stop();
    return SUCCESS;
}

ErrStatus gt911_init(void)
{
    ErrStatus err;
    rt_uint8_t i;
    rt_uint8_t checksum;
    rt_uint8_t cfg_num=GT911_CFG_NUM;

    memset(&gt911, 0, sizeof(gt911));

    i2c_touch_init();

    memcpy(cfg_buf, gt911_cfg_params, cfg_num);
    checksum=0;
    for(i=0; i < cfg_num; i++) {
        checksum += cfg_buf[i];
    }
    cfg_buf[sizeof(gt911_cfg_params) - 2]=(~checksum) + 1;
    cfg_buf[sizeof(gt911_cfg_params) - 1]=1;

    rt_thread_mdelay(100);

    if(ERROR != gt911_writereg(GT911_CFG_START_ADDR, (rt_uint8_t *)cfg_buf, sizeof(gt911_cfg_params))) {
        rt_thread_mdelay(10);
        gt911.enable=1;
        err=SUCCESS;
    } else {
        gt911.enable=0;
        err=ERROR;
    }
    return err;
}

rt_uint32_t gt911_read_id(void)
{
    rt_uint8_t id_buf[4];

    if(SUCCESS != gt911_readreg(0x8140, id_buf, 4)) {
        return 0;
    } else {
        return ((rt_uint32_t)id_buf[3] << 24) + ((rt_uint32_t)id_buf[2] << 16) +
               ((rt_uint32_t)id_buf[1] << 8) + id_buf[0];
    }
}

ErrStatus gt911_handler(void)
{
    ErrStatus err=SUCCESS;

    if(gt911.enable == 0) {
        return err;
    }

    err &= gt911_readreg(GT911_PRESSED_INFO_REG, touch_buf, 1);
    gt911.pressed_info=touch_buf[0];

    if(gt911.pressed_info & 0x0f) {
        err &= gt911_readreg(GT911_COORDINATE_REG, touch_buf, 8 * (gt911.pressed_info & 0x0f));
        for(rt_uint8_t i=0; i < (gt911.pressed_info & 0x0f); i++) {
            gt911.x[i]=((rt_uint16_t)touch_buf[2 + i * 8] << 8) + touch_buf[1 + i * 8];
            gt911.y[i]=((rt_uint16_t)touch_buf[4 + i * 8] << 8) + touch_buf[3 + i * 8];
            gt911.s[i]=((rt_uint16_t)touch_buf[6 + i * 8] << 8) + touch_buf[5 + i * 8];
        }
        gt911.pressed=2;
    } else if(gt911.pressed) {
        gt911.pressed--;
    }

    if(gt911.pressed_info) {
        err &= gt911_writereg(GT911_PRESSED_INFO_REG, &clear_flag, 1);
    }

    return err;
}
