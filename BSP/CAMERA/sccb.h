#ifndef SCCB_H
#define SCCB_H

#include "gd32h7xx.h"

#define DCI_SCCB                       I2C1
#define DCI_SCCB_CLK                   RCU_I2C1

#define DCI_SCCB_SDA_PIN               GPIO_PIN_11
#define DCI_SCCB_SDA_GPIO_PORT         GPIOB
#define DCI_SCCB_SDA_GPIO_CLK          RCU_GPIOB
#define DCI_SCCB_SDA_AF                GPIO_AF_4

#define DCI_SCCB_SCL_PIN               GPIO_PIN_4
#define DCI_SCCB_SCL_GPIO_PORT         GPIOH
#define DCI_SCCB_SCL_GPIO_CLK          RCU_GPIOH
#define DCI_SCCB_SCL_AF                GPIO_AF_4

#define SCCB_FLAG_TIMEOUT              2000000
#define OV2640_DEVICE_WRITE_ADDRESS    0x60
#define OV2640_DEVICE_READ_ADDRESS     0x61

#define SCCB_SPEED                     100000
#define SCCB_SLAVE_ADDRESS7            0xFE

void sccb_config(void);
uint8_t dci_byte_write(uint8_t reg, uint8_t data);
uint8_t dci_byte_read(uint8_t reg, uint8_t *data);

#endif
