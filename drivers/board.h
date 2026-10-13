/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-08-28     RealThread   first version
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef SOC_SERIES_GD32H75E
#include <gd32h75e.h>
#include "gd32h75e_exti.h"
#else
#include <gd32h7xx.h>
#include "gd32h7xx_exti.h"
#endif
#include <drv_gpio.h>
#include "drv_legacy.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------- CHIP CONFIG BEGIN --------------------------*/

#define CHIP_FAMILY_GD32
#define CHIP_SERIES_GD32H7
#define CHIP_NAME_GD32H759IM

/*-------------------------- CHIP CONFIG END --------------------------*/

/*-------------------------- ROM/RAM CONFIG BEGIN --------------------------*/

#define ROM_START              ((uint32_t)0x8000000)
#define ROM_SIZE               (3840 * 1024)
#define ROM_END                ((uint32_t)(ROM_START + ROM_SIZE))

#define RAM_START              (0x24000000)
#define RAM_SIZE               (512 * 1024)
#define RAM_END                (RAM_START + RAM_SIZE)
/* TLI LAYER0 LVGL framebuffer occupies the top of AXI SRAM (261120 B).
 * It was moved out of SDRAM because the board's SDRAM has two stuck data
 * bits that visibly corrupt displayed pixels. The RT-Thread heap
 * (HEAP_END in drv_common.h) must stay below this region. */
#define LVGL_FB_SIZE           (480u * 272u * 2u)
#define LVGL_FB_BASE           (RAM_START + RAM_SIZE - LVGL_FB_SIZE)

/*-------------------------- ROM/RAM CONFIG END --------------------------*/

/*-------------------------- UART CONFIG BEGIN --------------------------*/

/** After configuring corresponding UART or UART DMA, you can use it.
 *
 * STEP 1, define macro define related to the serial port opening based on the serial port number
 *                 such as     #define BSP_USING_UART1
 *
 * STEP 2, according to the corresponding pin of serial port, define the related serial port information macro
 *                 such as     #define BSP_UART1_TX_PIN       "PA9"
 *                             #define BSP_UART1_RX_PIN       "PA10"
 *
 * STEP 3, if you want using SERIAL DMA, you must open it in the RT-Thread Settings.
 *                 RT-Thread Setting -> Components -> Device Drivers -> Serial Device Drivers -> Enable Serial DMA Mode
 *
 * STEP 4, according to serial port number to define serial port tx/rx DMA function in the board.h file
 *                 such as     #define BSP_UART1_RX_USING_DMA
 *
 */

#define BSP_USING_UART1
#define BSP_UART1_TX_PIN       "PA9"
#define BSP_UART1_RX_PIN       "PA10"

/*-------------------------- UART CONFIG END --------------------------*/

/*-------------------------- I2C CONFIG BEGIN --------------------------*/

/** How to enable and configure I2C (hardware)
 *
 * STEP 1, enable the I2C driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using I2C device drivers
 *
 * STEP 2, turn on the I2C instance by defining the related macro, and prefer the
 *         default board-level settings provided in i2c_config.h.
 *           e.g.  #define BSP_USING_I2C1
 *
 * STEP 3, if you accept the default pins in i2c_config.h, you are done. No further
 *         pin configuration is required here.
 *
 * STEP 4, to override pins, set the port pins below for your MCU package according
 *         to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_I2C1_SCL_PIN    "PH4"
 *                 #define BSP_I2C1_SDA_PIN    "PB11"
 *
 * STEP 5, to override alternate function (AF), set the AF macro below in this file
 *         according to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_I2C1_AFIO       "AF4"
 *
 * Notes:
 * - For details on using the I2C device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/i2c/i2c
 * - If using the default configuration in i2c_config.h, just add the device macro (e.g., BSP_USING_I2C1) in board.h.
 * - To use custom pins, follow the definitions below.
 */

/*----------------------- Driver configuration example ----------------------*/
/* #define BSP_USING_I2C1 */
/* #define BSP_I2C1_SCL_PIN    "PH4"  */
/* #define BSP_I2C1_SDA_PIN    "PB11" */
/* #define BSP_I2C1_AFIO       "AF4"  */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- I2C CONFIG END --------------------------*/

/*-------------------------- SPI CONFIG BEGIN --------------------------*/

/** How to enable and configure SPI (hardware)
 *
 * STEP 1, enable the SPI driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using SPI Bus/Device drivers
 *
 * STEP 2, turn on the SPI instance by defining the related macro, and prefer the
 *         default board-level settings provided in spi_config.h.
 *           e.g.  #define BSP_USING_SPI1
 *
 * STEP 3, if you accept the default pins in spi_config.h, you are done. No further
 *         pin configuration is required here.
 *
 * STEP 4, to override pins, set the port pins below for your MCU package according
 *         to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_SPI1_SCK_PIN    "PB13"
 *                 #define BSP_SPI1_MISO_PIN   "PB14"
 *                 #define BSP_SPI1_MOSI_PIN   "PB15"
 *
 * STEP 5, to override alternate function (AF), set the AF macro below in this file
 *         according to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_SPI1_AFIO       "AF5"
 *
 * Notes:
 * - For details on using the SPI device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/spi/spi
 * - If using the default configuration in spi_config.h, just add the device macro (e.g., BSP_USING_SPI1) in board.h.
 * - To use custom pins, follow the definitions below.
 */

/*----------------------- Driver configuration example ----------------------*/
/* #define BSP_USING_SPI1 */
/* #define BSP_SPI1_SCK_PIN   "PB13" */
/* #define BSP_SPI1_MISO_PIN  "PB14" */
/* #define BSP_SPI1_MOSI_PIN  "PB15" */
/* #define BSP_SPI1_AFIO      "AF5"  */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- SPI CONFIG END --------------------------*/

/*-------------------------- PWM CONFIG BEGIN --------------------------*/

/** How to enable and configure PWM (hardware)
 *
 * STEP 1, enable the PWM driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using PWM device drivers
 *
 * STEP 2, turn on the PWM instance by defining the related macro, and prefer the
 *         default board-level settings provided in pwm_config.h.
 *           e.g.  #define BSP_USING_PWM0
 *
 * STEP 3, if you accept the default pins in pwm_config.h, you are done. No further
 *         pin configuration is required here.
 *
 * STEP 4, to override pins, set the port pins below for your MCU package according
 *         to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_PWM0_PIN        "PC7"
 *                 #define BSP_PWM0_CHANNEL     3
 *
 * STEP 5, to override alternate function (AF), set the AF macro below in this file
 *         according to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_PWM0_AFIO       "AF1"
 *
 * Notes:
 * - For details on using the PWM device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/pwm/pwm
 * - If using the default configuration in pwm_config.h, just add the device macro (e.g., BSP_USING_PWM0) in board.h.
 * - To use custom pins, follow the definitions below.
 */

/*----------------------- PWM Driver configuration example ----------------------*/
/* #define BSP_USING_PWM0 */
/* #define BSP_PWM0_PIN       "PC7" */
/* #define BSP_PWM0_CHANNEL    3    */
/* #define BSP_PWM0_AFIO      "AF1" */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- PWM CONFIG END --------------------------*/

/*-------------------------- ADC CONFIG BEGIN --------------------------*/

/** How to enable and configure ADC
 *
 * STEP 1, enable the ADC driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using ADC device drivers
 *
 * STEP 2, turn on the ADC instance by defining the related macro.
 *           e.g.  #define BSP_USING_ADC0
 *
 * Notes:
 * - For details on using the ADC device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/adc/adc
 * - The ADC pins are defined in adc_config.h.
 * - To view the corresponding pins and channels, refer to the GD32H759xx Datasheet.
 */

/*----------------------- ADC Driver configuration example ----------------------*/
/* #define BSP_USING_ADC0 */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- ADC CONFIG END --------------------------*/

/*-------------------------- DAC CONFIG BEGIN --------------------------*/

/** How to enable and configure DAC
 *
 * STEP 1, enable the DAC driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using DAC device drivers
 *
 * STEP 2, turn on the DAC instance by defining the related macro, and prefer the
 *         default board-level settings provided in dac_config.h.
 *           e.g.  #define BSP_USING_DAC0
 *
 * STEP 3, if you accept the default pins in dac_config.h, you are done. No further
 *         pin configuration is required here.
 *
 * STEP 4, to override pins, set the port pins below for your MCU package according
 *         to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_DAC0_PIN        "PA4"
 *                 #define BSP_DAC0_CHANNEL     DAC_OUT0
 *
 * Notes:
 * - For details on using the DAC device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/dac/dac 
 * - If using the default configuration in dac_config.h, just add the device macro (e.g., BSP_USING_DAC0) in board.h.
 * - To use custom pins, follow the definitions below.
 */

/*-----------------------DAC Driver configuration example ----------------------*/
/* #define BSP_USING_DAC0 */
/* #define BSP_DAC0_PIN                    "PA4"    */
/* #define BSP_DAC0_CHANNEL                DAC_OUT0 */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- DAC CONFIG END --------------------------*/

/*-------------------------- WDT CONFIG BEGIN --------------------------*/

/** How to enable and configure WDT
 *
 * STEP 1, enable the WDT driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using Watchdog device drivers
 *
 * Notes:
 * - For details on using the WDT device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/watchdog/watchdog
 */

/*-------------------------- WDT CONFIG END --------------------------*/

/*-------------------------- HARDWARE TIMER CONFIG BEGIN --------------------------*/

/** if you want to use hardware timer you can use the following instructions.
 *
 * STEP 1, enable the hardware timer driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using hardware timer device drivers
 *
 * STEP 2, define macro related to the hwtimer
 *                 such as     #define BSP_USING_HWTIMER0
 *
 * Notes:
 * - The macro definitions and device names for the hardware timer are defined in drv_hwtimer.c.
 * - For details on using the hardware timer device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/hwtimer/hwtimer
 */

/*-----------------------HARDWARE TIMER configuration example ----------------------*/
/* #define BSP_USING_HWTIMER0 */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- HARDWARE TIMER CONFIG END --------------------------*/

/*-------------------------- RTC CONFIG BEGIN --------------------------*/

/** How to enable and configure RTC
 *
 * STEP 1, enable the RTC driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using RTC device drivers
 *
 * STEP 2, define macro related to the RTC
 *                 such as    #define BSP_USING_ALARM0
 *
 * Notes:
 * - The macro definitions and device names for the RTC are defined in drv_rtc.c.
 * - For details on using the RTC device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/rtc/rtc
 */
 
/*-----------------------RTC configuration example ----------------------*/
/* #define BSP_USING_ALARM0 */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- RTC CONFIG END --------------------------*/

/*-------------------------- SDIO CONFIG BEGIN --------------------------*/

/** How to enable and configure SDIO
 * STEP 1, enable the SDIO driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using SD/EMMC device drivers
 * STEP 2, enable the DFS driver in RT-Thread Settings.
 *           e.g.  Components -> DFS -> Using DFS device drivers
 * STEP 3, enable the elm-Fatfs driver in RT-Thread Settings.
 *           e.g.  Components -> DFS -> Using elm-Fatfs device drivers
 *
 * Notes:
 * - The SDIO pin configuration is implemented in the gpio_config() function
 *   located in drv_sdio.c.
 * - Before using the SDIO interface, please consult the MCU datasheet and ensure
 *   that all related pins are configured according to the hardware requirements.
 * - If custom pins are needed, update the configuration inside gpio_config()
 *   accordingly.
 */
 
/*-------------------------- SDIO CONFIG END --------------------------*/

/*-------------------------- CAN CONFIG BEGIN --------------------------*/

/** How to enable and configure CAN
 *
 * STEP 1, enable the CAN driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using CAN device drivers
 *
 * STEP 2, turn on the CAN instance by defining the related macro, and prefer the
 *         default board-level settings provided in can_config.h.
 *           e.g.  #define BSP_USING_CAN0
 *
 * STEP 3, if you accept the default pins in can_config.h, you are done. No further
 *         pin configuration is required here.
 *
 * STEP 4, to override pins, set the port pins below for your MCU package according
 *         to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_CAN0_RX_PIN     "PD0"
 *                 #define BSP_CAN0_TX_PIN     "PD1"
 *
 * STEP 5, to override alternate function (AF), set the AF macro below in this file
 *         according to the datasheet (GD32H759xx Datasheet 2.6.3. GD32H759xx pin alternate functions):
 *           e.g.  #define BSP_CAN0_AFIO       "AF9"
 *
 * Notes:
 * - For details on using the CAN device, refer to the documentation center at the following link:
 * - https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/device/can/can
 * - If using the default configuration in can_config.h, just add the device macro (e.g., BSP_USING_CAN0) in board.h.
 * - To use custom pins, follow the definitions below.
 */

/*-----------------------CAN Driver configuration example ----------------------*/
/* #define BSP_USING_CAN0 */
/* #define BSP_CAN0_RX_PIN    "PD0" */ 
/* #define BSP_CAN0_TX_PIN    "PD1" */
/* #define BSP_CAN0_AFIO      "AF9" */

/*---------- Configure your desired device here as described above ---------*/
/* USER CODE BEGIN */

/* USER CODE END */

/*-------------------------- CAN CONFIG END --------------------------*/

/*-------------------------- LCD CONFIG BEGIN --------------------------*/

/** How to enable and configure LCD
 * STEP 1, enable the SDIO driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using LCD device drivers
 *
 * Notes:
 * - The LCD pin configuration is implemented in the tli_gpio_config() function
 *   located in drv_lcd.c.
 * - Before using the LCD, please consult the MCU datasheet and ensure that all
 *   related pins are configured according to the hardware requirements.
 * - If custom pins are needed, update the configuration inside tli_gpio_config()
 *   accordingly.
 */

/*-------------------------- LCD CONFIG END --------------------------*/

/*-------------------------- ETH CONFIG BEGIN --------------------------*/

/** if you want to use eth you can use the following instructions.
 *
 * STEP 1, enable the network interface device in RT-Thread Settings.
 *           e.g.  Components -> Network -> Enable network interface device
 *
 * STEP 2, enable the socket abstraction layer in RT-Thread Settings.
 *           e.g.  Components -> Network -> SAL: socket abstraction layer
 *
 * STEP 3, select the lwip stack and features you need:
 *           e.g.  Components -> Network -> LwIP: light weight TCP/IP stack
 *
 * STEP 4, configure MPU settings for Ethernet DMA descriptors and buffers:
 *           Check and modify the enet_mpu_config() function in drivers/drv_enet.c:
 *           - Configure MPU region for DMA descriptors and Rx/Tx buffers (0x30000000)
 *           - Set appropriate memory attributes (bufferable, non-cacheable, non-shareable)
 *           - Ensure MPU region size matches your actual buffer requirements
 *
 * STEP 5, configure Ethernet GPIO pins according to your hardware design:
 *           Check and modify the enet_gpio_config() function in drivers/rtt/include/config/enet_config.h:
 *           - Verify all MII/RMII/RGMII interface pins are correctly configured
 *           - Check clock output configuration (such as: PA8 for PHY reference clock)
 *           - Ensure correct alternate function (such as: AF11 for Ethernet pins)
 *           - Configure PHY reset pin if required by your hardware
 *
 * Notes:
 * - The pin configuration in enet_gpio_config() must match your actual hardware design and PCB layout
 * - MPU configuration in enet_mpu_config() is critical for proper DMA operation
 * - For MII/RMII/RGMII interface, ensure all required pins are properly configured in enet_gpio_config()
 * - Refer to GD32H7xx datasheet for Ethernet pin alternate function mapping
 * - Refer to PHY chip datasheet for specific configuration requirements
 */

/*-------------------------- ETH CONFIG END --------------------------*/

/*-------------------------- USB HOST CONFIG BEGIN --------------------------*/

/** How to enable and configure USB Host (hardware)
 *
 * STEP 1, enable the USB Host driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using USB -> USB Host
 *
 * STEP 2, select the USB Host stack and drivers you need:
 *           e.g.  USB Host -> Using USB Host
 *                 USB Host -> Using USB Host -> Enable UDisk Drivers
 *                 USB Host -> Using USB Host -> Enable HID Drivers
 *
 * STEP 3, define the basic USB configuration macros in board.h:
 *           e.g.  #define BSP_USING_USB           // Enable USB peripheral
 *                 #define USE_USBHS0              // Enable USBHS0 instance
 *
 * STEP 4, select USB speed mode in board.h:
 *           For High Speed (HS) mode:
 *              e.g.  #define USE_USB_HS
 *           For Full Speed (FS) mode:
 *              e.g.  #define USE_USB_FS
 *
 * STEP 5, verify and modify the VBUS power switch configuration in the hardware driver:
 *           Check and modify the following definitions in drivers/usb/gd32h7_usb_hw.c:
 *           e.g.  #define HOST_POWERSW_PORT_RCC RCU_GPIOG
 *                 #define HOST_POWERSW_PORT     GPIOG
 *                 #define HOST_POWERSW_VBUS     GPIO_PIN_7
 *           Ensure these definitions match your actual hardware design and pin connections.
 *
 * Notes:
 * - USB HS (High Speed) requires external PHY and supports 480 Mbps data rate
 * - USB FS (Full Speed) uses internal PHY and supports 12 Mbps data rate  
 * - The pin configuration depends on the specific USB instance (USBHS0/USBHS1) and package type
 * - The HOST_POWERSW_VBUS configuration in gd32h7_usb_hw.c must match your actual hardware design
 * - Refer to GD32H7xx datasheet for the specific pin alternate function mapping
 */

/*----------------------- Driver configuration example ----------------------*/
/* Basic USB configuration in board.h */
/* #define BSP_USING_USB */
/* #define USE_USBHS0 | #define USE_USBHS1 */
/* #define USE_USB_HS | #define USE_USB_FS */

/*-------------------------- USB HOST CONFIG END --------------------------*/

/*-------------------------- USB DEVICE CONFIG BEGIN --------------------------*/

/** How to enable and configure USB Device (hardware)
 *
 * STEP 1, enable the USB Device driver in RT-Thread Settings.
 *           e.g.  Components -> Device Drivers -> Using USB -> USB Device
 *
 * STEP 2, select the USB Device stack and device classes you need:
 *           e.g.  USB Device -> Using USB Device
 *                 USB Device -> Using USB Device -> Enable CDC Virtual Com Port
 *                 USB Device -> Using USB Device -> Enable Mass Storage Gadget
 *                 USB Device -> Using USB Device -> Enable HID Gadget
 *                                                -> ...
 *
 * STEP 3, define the basic USB configuration macros in board.h:
 *           e.g.  #define BSP_USING_USB           // Enable USB peripheral
 *                 #define USE_USBHS0              // Enable USBHS0 instance
 *
 * STEP 4, select USB speed mode in board.h:
 *           For High Speed (HS) mode:
 *              e.g.  #define USE_USB_HS
 *           For Full Speed (FS) mode:
 *              e.g.  #define USE_USB_FS
 *
 * Notes:
 * - USB HS (High Speed) requires external PHY and supports 480 Mbps data rate
 * - USB FS (Full Speed) uses internal PHY and supports 12 Mbps data rate  
 * - The pin configuration depends on the specific USB instance (USBHS0/USBHS1) and package type
 * - The DEVICE_VBUS configuration in gd32h7_usb_hw.c must match your actual hardware design
 * - For CDC Virtual Com Port, additional configurations may be needed in the device class settings
 * - Refer to GD32H7xx datasheet for the specific pin alternate function mapping
 */

/*----------------------- Driver configuration example ----------------------*/
/* Basic USB configuration in board.h */
/* #define BSP_USING_USB */
/* #define USE_USBHS0 | #define USE_USBHS1 */
/* #define USE_USB_HS | #define USE_USB_FS */

/*-------------------------- USB DEVICE CONFIG END --------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
