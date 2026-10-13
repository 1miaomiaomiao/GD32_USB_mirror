#ifndef __TOUCH_H__
#define __TOUCH_H__

#include <rtthread.h>
#include "gd32h7xx.h"

#define GTP_ADDRESS                      0xBA

#define GTP_I2C_SCL_PIN                  GPIO_PIN_7
#define GTP_I2C_SCL_GPIO_PORT            GPIOH
#define GTP_I2C_SCL_GPIO_CLK             RCU_GPIOH

#define GTP_I2C_SDA_PIN                  GPIO_PIN_9
#define GTP_I2C_SDA_GPIO_PORT            GPIOF
#define GTP_I2C_SDA_GPIO_CLK             RCU_GPIOF

#define GTP_RST_GPIO_CLK                 RCU_GPIOF
#define GTP_RST_GPIO_PORT                GPIOF
#define GTP_RST_GPIO_PIN                 GPIO_PIN_6
#define GTP_RST_1()                      gpio_bit_set(GTP_RST_GPIO_PORT,GTP_RST_GPIO_PIN)
#define GTP_RST_0()                      gpio_bit_reset(GTP_RST_GPIO_PORT,GTP_RST_GPIO_PIN)

#define GTP_INT_GPIO_CLK                 RCU_GPIOH
#define GTP_INT_GPIO_PORT                GPIOH
#define GTP_INT_GPIO_PIN                 GPIO_PIN_6
#define GTP_INT_1()                      gpio_bit_set(GTP_INT_GPIO_PORT,GTP_INT_GPIO_PIN)
#define GTP_INT_0()                      gpio_bit_reset(GTP_INT_GPIO_PORT,GTP_INT_GPIO_PIN)

#define I2C_SCL_1()                      gpio_bit_set(GTP_I2C_SCL_GPIO_PORT, GTP_I2C_SCL_PIN)
#define I2C_SCL_0()                      gpio_bit_reset(GTP_I2C_SCL_GPIO_PORT, GTP_I2C_SCL_PIN)
#define I2C_SDA_1()                      gpio_bit_set(GTP_I2C_SDA_GPIO_PORT, GTP_I2C_SDA_PIN)
#define I2C_SDA_0()                      gpio_bit_reset(GTP_I2C_SDA_GPIO_PORT, GTP_I2C_SDA_PIN)
#define I2C_SDA_READ()                   gpio_input_bit_get(GTP_I2C_SDA_GPIO_PORT, GTP_I2C_SDA_PIN)

#define I2C_DIR_WR    0
#define I2C_DIR_RD    1

#define GT911_PRESSED_INFO_REG 0x814E
#define GT911_COORDINATE_REG   0x814F
#define GT911_CFG_START_ADDR   0x8047
#define GT911_CFG_NUM          (0x80FE - 0x8047 + 1)

typedef struct {
    rt_uint8_t  enable;
    rt_uint8_t  pressed;
    rt_uint8_t  pressed_info;
    rt_uint16_t x[5];
    rt_uint16_t y[5];
    rt_uint16_t s[5];
} gt911_struct;

extern gt911_struct gt911;

ErrStatus gt911_init(void);

rt_uint32_t gt911_read_id(void);

ErrStatus gt911_handler(void);

#endif
