/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#include <stddef.h>
#include <rthw.h>
#include <rtthread.h>

#ifdef RT_USING_SDIO
#include "drv_sdio.h"

#define DBG_TAG "drv.sdio"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SDIO_DMA_USE_IPC 0

/* card status of R1 definitions */
#define SD_R1_OUT_OF_RANGE          BIT(31)                 /* command's argument was out of the allowed range */
#define SD_R1_ADDRESS_ERROR         BIT(30)                 /* misaligned address which did not match the block length */
#define SD_R1_BLOCK_LEN_ERROR       BIT(29)                 /* transferred block length is not allowed */
#define SD_R1_ERASE_SEQ_ERROR       BIT(28)                 /* an error in the sequence of erase commands occurred */
#define SD_R1_ERASE_PARAM           BIT(27)                 /* an invalid selection of write-blocks for erase occurred */
#define SD_R1_WP_VIOLATION          BIT(26)                 /* the host attempts to write to a protected block or to the temporary or permanent write protected card */
#define SD_R1_CARD_IS_LOCKED        BIT(25)                 /* the card is locked by the host */
#define SD_R1_LOCK_UNLOCK_FAILED    BIT(24)                 /* a sequence or password error has been detected in lock/unlock card command */
#define SD_R1_COM_CRC_ERROR         BIT(23)                 /* CRC check of the previous command failed */
#define SD_R1_ILLEGAL_COMMAND       BIT(22)                 /* command not legal for the card state */
#define SD_R1_CARD_ECC_FAILED       BIT(21)                 /* card internal ECC was applied but failed to correct the data */
#define SD_R1_CC_ERROR              BIT(20)                 /* internal card controller error */
#define SD_R1_GENERAL_UNKNOWN_ERROR BIT(19)                 /* a general or an unknown error occurred during the operation */
#define SD_R1_CSD_OVERWRITE         BIT(16)                 /* read only section of the CSD does not match or attempt to reverse the copy or permanent WP bits */
#define SD_R1_WP_ERASE_SKIP         BIT(15)                 /* partial address space was erased */
#define SD_R1_CARD_ECC_DISABLED     BIT(14)                 /* command has been executed without using the internal ECC */
#define SD_R1_ERASE_RESET           BIT(13)                 /* an erase sequence was cleared before executing */
#define SD_R1_READY_FOR_DATA        BIT(8)                  /* correspond to buffer empty signaling on the bus */
#define SD_R1_APP_CMD               BIT(5)                  /* card will expect ACMD */
#define SD_R1_AKE_SEQ_ERROR         BIT(3)                  /* error in the sequence of the authentication process */
#define SD_R1_ERROR_BITS            ((uint32_t)0xFDF9E008U) /* all the R1 error bits */

/* card status of R6 definitions */
#define SD_R6_COM_CRC_ERROR         BIT(15) /* CRC check of the previous command failed */
#define SD_R6_ILLEGAL_COMMAND       BIT(14) /* command not legal for the card state */
#define SD_R6_GENERAL_UNKNOWN_ERROR BIT(13) /* a general or an unknown error occurred during the operation */

/* card state */
#define SD_CARDSTATE_IDLE        ((uint8_t)0x00)         /* card is in idle state */
#define SD_CARDSTATE_READY       ((uint8_t)0x01)         /* card is in ready state */
#define SD_CARDSTATE_IDENTIFICAT ((uint8_t)0x02)         /* card is in identificat state */
#define SD_CARDSTATE_STANDBY     ((uint8_t)0x03)         /* card is in standby state */
#define SD_CARDSTATE_TRANSFER    ((uint8_t)0x04)         /* card is in transfer state */
#define SD_CARDSTATE_DATA        ((uint8_t)0x05)         /* card is in data sending state */
#define SD_CARDSTATE_RECEIVING   ((uint8_t)0x06)         /* card is in receiving state */
#define SD_CARDSTATE_PROGRAMMING ((uint8_t)0x07)         /* card is in programming state */
#define SD_CARDSTATE_DISCONNECT  ((uint8_t)0x08)         /* card is in disconnect state */
#define SD_CARDSTATE_LOCKED      ((uint32_t)0x02000000U) /* card is in locked state */

#define SD_CHECK_PATTERN  ((uint32_t)0x000001AAU) /* check pattern for CMD8 */
#define SD_VOLTAGE_WINDOW ((uint32_t)0x00100000U) /* host 3.3V request in ACMD41 */
#define SD_VOLTAGE_18V    ((uint32_t)0x01000000U) /* host 1.8V request in ACMD41 */

/* parameters for ACMD41(voltage validation) */
#define SD_HIGH_CAPACITY ((uint32_t)0x40000000) /* high capacity SD memory card */
#define SD_STD_CAPACITY  ((uint32_t)0x00000000) /* standard capacity SD memory card */

/* SD bus width, check SCR register */
#define SD_BUS_WIDTH_4BIT ((uint32_t)0x00040000) /* 4-bit width bus mode */
#define SD_BUS_WIDTH_1BIT ((uint32_t)0x00010000) /* 1-bit width bus mode */

/* masks for SCR register */
#define SD_MASK_0_7BITS   ((uint32_t)0x000000FF) /* mask [7:0] bits */
#define SD_MASK_8_15BITS  ((uint32_t)0x0000FF00) /* mask [15:8] bits */
#define SD_MASK_16_23BITS ((uint32_t)0x00FF0000) /* mask [23:16] bits */
#define SD_MASK_24_31BITS ((uint32_t)0xFF000000) /* mask [31:24] bits */

#define SD_FIFOHALF_WORDS ((uint32_t)0x00000008) /* words of FIFO half full/empty */
#define SD_FIFOHALF_BYTES ((uint32_t)0x00000020) /* bytes of FIFO half full/empty */

#define SD_DATATIMEOUT         ((uint32_t)0xFFFFFFFF) /* DSM data timeout */
#define SD_MAX_VOLT_VALIDATION ((uint32_t)0x0000FFFF) /* the maximum times of voltage validation */
#define SD_MAX_DATA_LENGTH     ((uint32_t)0x01FFFFFF) /* the maximum length of data */
#define SD_ALLZERO             ((uint32_t)0x00000000) /* all zero */
#define SD_RCA_SHIFT           ((uint8_t)0x10)        /* RCA shift bits */

/* user can according to need to change the macro values */
#define SD_CLK_DIV_INIT              ((uint32_t)0x01F4) /* SD clock division in initilization phase */
#define SD_CLK_DIV_TRANS_DSPEED      ((uint32_t)0x0008) /* SD clock division in default speed transmission phase */
#define SD_CLK_DIV_TRANS_HSPEED      ((uint32_t)0x0004) /* SD clock division in high speed transmission phase */
#define SD_CLK_DIV_TRANS_SDR25SPEED  ((uint32_t)0x0004) /* SD clock division in SDR25 high speed transmission phase */
#define SD_CLK_DIV_TRANS_SDR50SPEED  ((uint32_t)0x0002) /* SD clock division in SDR50 high speed transmission phase */
#define SD_CLK_DIV_TRANS_SDR104SPEED ((uint32_t)0x0001) /* SD clock division in SDR104 high speed transmission phase */
#define SD_CLK_DIV_TRANS_DDR50SPEED  ((uint32_t)0x0004) /* SD clock division in DDR50 high speed transmission phase */

#define SDIO_MASK_INTC_FLAGS ((uint32_t)0x1FE00FFF) /* mask flags of SDIO_INTC */
#define SDIO_MASK_CMD_FLAGS  ((uint32_t)0x002000C5) /* mask flags of CMD FLAGS */
#define SDIO_MASK_DATA_FLAGS ((uint32_t)0x18000F3A) /* mask flags of DATA FLAGS */

/* SDIO bus switch */
/* config SDIO bus mode, select: BUSMODE_1BIT/BUSMODE_4BIT */
#define SDIO_BUSMODE BUSMODE_4BIT
/* config SDIO speed mode, select: SD_SPEED_DEFAULT/SD_SPEED_HIGH */
#define SDIO_SPEEDMODE SD_SPEED_HIGH
/* config data transfer mode, select: SD_POLLING_MODE/SD_DMA_MODE */
#define SDIO_DTMODE SD_POLLING_MODE

typedef struct
{
    uint32_t sd_scr[2];    /* content of SCR register */
    uint32_t sdcardtype;   /* SD card capacity level, for ACMD41 parameter */
    uint32_t cardcapacity; /* SD card capacity type */
    sdio_card_type_enum cardtype;     /* SD card type */
    uint32_t sd_csd[4];    /* content of CSD register */
    uint32_t sd_cid[4];    /* content of CID register */
    uint16_t sd_rca;       /* RCA of SD card */
    uint32_t transmode;
    uint32_t totalnumber_bytes;
    uint32_t stopcondition;
    __IO sd_error_enum transerror;
    __IO uint32_t transend;
    __IO uint32_t number_bytes;
} sdcard_opration_t;

// 修改 opration 为 SDHC
static sdcard_opration_t card_opration = {
    .sd_scr = { 0, 0 },
    .sdcardtype = SD_HIGH_CAPACITY,
    .cardcapacity = SD_SDHC_SDXC,
    .cardtype = SDIO_HIGH_CAPACITY_SD_CARD,
    .sd_csd = { 0, 0, 0, 0 },
    .sd_cid = { 0, 0, 0, 0 },
    .sd_rca = 0,
    .transmode = SD_POLLING_MODE,
    .totalnumber_bytes = 0,
    .stopcondition = 0,
    .transerror = SD_OK,
    .transend = 0,
    .number_bytes = 0,
};

/* set sector size to 512 */
#define SECTOR_SIZE 512

typedef struct
{
    struct rt_device sdcard_device;
    uint32_t sdio_periph;
    sd_card_info_struct sd_cardinfo;
    IRQn_Type irqn;
    struct rt_mutex sd_lock;
    struct rt_semaphore sem;
    char *device_name;
} gd32_sdio_t;

static gd32_sdio_t sd0 = {
    .sdio_periph = SDIO0,
    .irqn = SDIO0_IRQn,
    .device_name = "sd0",
};

/* check if the command sent error occurs */
static sd_error_enum cmdsent_error_check(uint32_t SDIO);
/* check if error occurs for R1 response */
static sd_error_enum r1_error_check(uint32_t SDIO, uint8_t cmdindex);
/* check if error type for R1 response */
static sd_error_enum r1_error_type_check(uint32_t resp);
/* check if error occurs for R2 response */
static sd_error_enum r2_error_check(uint32_t SDIO);
/* check if error occurs for R3 response */
static sd_error_enum r3_error_check(uint32_t SDIO);
/* check if error occurs for R6 response */
static sd_error_enum r6_error_check(uint32_t SDIO, uint8_t cmdindex, uint16_t *prca);
/* check if error occurs for R7 response */
static sd_error_enum r7_error_check(uint32_t SDIO);

/* get the state which the card is in */
static sd_error_enum sd_card_state_get(uint32_t SDIO, uint8_t *pcardstate);
/* configure the bus width mode */
static sd_error_enum sd_bus_width_config(uint32_t SDIO, uint32_t buswidth);
/* get the SCR of corresponding card */
static sd_error_enum sd_scr_get(uint32_t SDIO, uint16_t rca, uint32_t *pscr);
/* get the data block size */
static uint32_t sd_datablocksize_get(uint16_t bytesnumber);

/* configure the GPIO of SDIO interface */
static void gpio_config(void);
/* configure the RCU of SDIO and DMA */
static void rcu_config(void);
/* configure the DMA for SDIO request */
static void dma_config(uint32_t SDIO, uint32_t *srcbuf, uint32_t bufsize);

#ifdef USE_18V_SWITCH
/* enable SD Transceiver 1.8V mode */
static void sd_transceiver_enable(void);
#endif /* USE_18V_SWITCH */

sd_error_enum sd_config(uint32_t SDIO);
static void card_info_get(uint32_t SDIO);

/* RT-Thread Device Driver Interface */
static rt_err_t rt_sdcard_init(rt_device_t dev)
{
    rt_err_t ret = RT_EOK;
    sd_error_enum sd_error = SD_OK;
    uint16_t retry = 5;

    // 初始化互斥锁
    ret = rt_mutex_init(&sd0.sd_lock, "sd_lock", RT_IPC_FLAG_FIFO);

    if (RT_EOK != ret)
    {
        LOG_E("init mutex failed\n");
        return ret;
    }

    // 初始化信号量(未发现使用)
    ret = rt_sem_init(&sd0.sem, "sd_sem", 0, RT_IPC_FLAG_FIFO);

    if (RT_EOK != ret)
    {
        LOG_E("init semaphore failed\n");
        return ret;
    }

    // 开启中断
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    nvic_irq_enable(sd0.irqn, 2, 0);

    do
    {
        /* initialize the card, get the card information and configurate the bus mode and transfer mode */
        sd_error = sd_config(sd0.sdio_periph);
    } while ((SD_OK != sd_error) && (--retry));

    if (retry)
    {
        LOG_I("Card init success!");
    }
    else
    {
        LOG_E(" Card init failed!");
        ret = -RT_EIO;
        return ret;
    }

    card_info_get(sd0.sdio_periph);

    return ret;
}

static rt_err_t rt_sdcard_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_sdcard_close(rt_device_t dev)
{
    return RT_EOK;
}

static uint32_t dma_buffer[SECTOR_SIZE / sizeof(uint32_t)];
static rt_ssize_t rt_sdcard_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL);

    sd_error_enum sd_error = SD_OK;
    gd32_sdio_t *sd = (gd32_sdio_t *)dev->user_data;

    if (!buffer)
    {
        return 0;
    }

    rt_mutex_take(&sd->sd_lock, RT_WAITING_FOREVER);

    if (((uint32_t)buffer & 0x03) != 0)
    {
        /* non-aligned. */
        uint32_t i;
        uint32_t sector_adr;
        uint32_t *copy_buffer;

        sector_adr = pos * SECTOR_SIZE;
        copy_buffer = (uint32_t *)buffer;

        for (i = 0; i < size; i++)
        {
            sd_error = sd_block_read(sd0.sdio_periph, (uint32_t *)dma_buffer, sector_adr, SECTOR_SIZE);

            rt_memcpy(copy_buffer, dma_buffer, SECTOR_SIZE);
            sector_adr += SECTOR_SIZE;
            copy_buffer += SECTOR_SIZE;
        }
    }
    else
    {
        if (size == 1)
        {
            sd_error = sd_block_read(sd0.sdio_periph, (uint32_t *)buffer, pos * SECTOR_SIZE, SECTOR_SIZE);
        }
        else
        {
            sd_error = sd_multiblocks_read(sd0.sdio_periph, (uint32_t *)buffer, pos * SECTOR_SIZE, SECTOR_SIZE, size);
        }
    }

    rt_mutex_release(&sd->sd_lock);

    if (sd_error == SD_OK)
    {
        return size;
    }
    else
    {
        return 0;
    }
}

static rt_ssize_t rt_sdcard_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
#define WR_RETRY_TIMES 2

    RT_ASSERT(dev != RT_NULL);

    uint8_t retry = 0;
    sd_error_enum sd_error = SD_OK;
    gd32_sdio_t *sd = (gd32_sdio_t *)dev->user_data;

    rt_mutex_take(&sd->sd_lock, RT_WAITING_FOREVER);

    if (((uint32_t)buffer & 0x03) != 0)
    {
        /* non-aligned. */
        uint32_t i;
        rt_size_t sector_adr;
        uint32_t *copy_buffer;

        sector_adr = pos * SECTOR_SIZE;
        copy_buffer = (uint32_t *)buffer;

        for (i = 0; i < size; i++)
        {
            retry = WR_RETRY_TIMES;
            rt_memcpy(dma_buffer, copy_buffer, SECTOR_SIZE);

            while (retry > 0)
            {
                sd_error = sd_block_write(sd0.sdio_periph, (uint32_t *)dma_buffer, sector_adr, SECTOR_SIZE);
                retry--;
                if (sd_error == SD_OK)
                {
                    break;
                }
            }

            sector_adr += SECTOR_SIZE;
            copy_buffer += SECTOR_SIZE;
        }
    }
    else
    {
        retry = WR_RETRY_TIMES;

        if (size == 1)
        {
            while (retry > 0)
            {
                sd_error = sd_block_write(sd0.sdio_periph, (uint32_t *)buffer, pos * SECTOR_SIZE, SECTOR_SIZE);
                retry--;
                if (sd_error == SD_OK)
                {
                    break;
                }
            }
        }
        else
        {
            while (retry > 0)
            {
                sd_error = sd_multiblocks_write(sd0.sdio_periph, (uint32_t *)buffer, pos * SECTOR_SIZE, SECTOR_SIZE, size);
                retry--;
                if (sd_error == SD_OK)
                {
                    break;
                }
            }
        }
    }

    rt_mutex_release(&sd->sd_lock);

    if (!retry)
    {
        LOG_D("sdio e:%d r:%d\n", sd_error, retry);
    }
    if (sd_error == SD_OK)
    {
        return size;
    }
    else
    {
        return 0;
    }
}

static rt_err_t rt_sdcard_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);

    gd32_sdio_t *sd = (gd32_sdio_t *)dev->user_data;

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL)
            return -RT_ERROR;

        geometry->bytes_per_sector = 512;
        geometry->block_size = sd->sd_cardinfo.card_blocksize;
        if (sd->sd_cardinfo.card_type == SDIO_HIGH_CAPACITY_SD_CARD)
            geometry->sector_count = (sd->sd_cardinfo.card_csd.c_size + 1) * 1024;
        else
            geometry->sector_count = sd->sd_cardinfo.card_capacity / sd->sd_cardinfo.card_blocksize;
    }

    return RT_EOK;
}

int rt_hw_sdcard_init(void)
{
    /* register sdcard device */
    sd0.sdcard_device.type = RT_Device_Class_Block;
    sd0.sdcard_device.init = rt_sdcard_init;
    sd0.sdcard_device.open = rt_sdcard_open;
    sd0.sdcard_device.close = rt_sdcard_close;
    sd0.sdcard_device.read = rt_sdcard_read;
    sd0.sdcard_device.write = rt_sdcard_write;
    sd0.sdcard_device.control = rt_sdcard_control;
    sd0.sdcard_device.user_data = &sd0;

    rt_device_register(&sd0.sdcard_device, sd0.device_name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_sdcard_init);

/*!
    \brief      initialize the card, get the card information, set the bus mode and transfer mode
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_config(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    uint32_t cardstate = 0;

    /* initialize the card */
    status = sd_init(SDIO);
    if (SD_OK == status)
    {
        status = sd_card_information_get(SDIO, &sd0.sd_cardinfo);
    }
    if (SD_OK == status)
    {
        status = sd_card_select_deselect(SDIO, sd0.sd_cardinfo.card_rca);
    }
    status = sd_cardstatus_get(SDIO, &cardstate);
    if (cardstate & 0x02000000)
    {
        LOG_D(" The card is locked!");
        status = sd_lock_unlock(SDIO, SD_UNLOCK);
        if (status != SD_OK)
        {
            return SD_LOCK_UNLOCK_FAILED;
        }
        else
        {
            LOG_D("\r\n the card is unlocked successfully!");
        }
    }

    if ((SD_OK == status) && (!(cardstate & 0x02000000)))
    {
        /* set bus mode */
#if (SDIO_BUSMODE == BUSMODE_4BIT)
        status = sd_bus_mode_config(SDIO, SDIO_BUSMODE_4BIT, SDIO_SPEEDMODE);
#else
        status = sd_bus_mode_config(SDIO, SDIO_BUSMODE_1BIT, SDIO_SPEEDMODE);
#endif
    }

    if (SD_OK == status)
    {
        /* set data transfer mode */
        status = sd_transfer_mode_config(SDIO_DTMODE);
    }
    return status;
}

/*!
    \brief      get the card information and print it out by USRAT
    \param[in]  none
    \param[out] none
    \retval     none
*/
void card_info_get(uint32_t SDIO)
{
    uint8_t sd_spec, sd_spec3, sd_spec4, sd_security;
    uint32_t block_count, block_size, sd_capacity;
    uint16_t temp_ccc;

    LOG_D("Card information:");

    sd_spec = (card_opration.sd_scr[1] & 0x0F000000) >> 24;
    sd_spec3 = (card_opration.sd_scr[1] & 0x00008000) >> 15;
    sd_spec4 = (card_opration.sd_scr[1] & 0x00000400) >> 10;
    if (2 == sd_spec)
    {
        if (1 == sd_spec3)
        {
            if (1 == sd_spec4)
            {
                LOG_D("## Card version 4.xx ##");
            }
            else
            {
                LOG_D("## Card version 3.0x ##");
            }
        }
        else
        {
            LOG_D("## Card version 2.00 ##");
        }
    }
    else if (1 == sd_spec)
    {
        LOG_D("## Card version 1.10 ##");
    }
    else if (0 == sd_spec)
    {
        LOG_D("## Card version 1.0x ##");
    }

    sd_security = (card_opration.sd_scr[1] & 0x00700000) >> 20;
    if (2 == sd_security)
    {
        LOG_I("## SDSC card ##");
    }
    else if (3 == sd_security)
    {
        LOG_I("## SDHC card ##");
    }
    else if (4 == sd_security)
    {
        LOG_I("## SDXC card ##");
    }

    block_size = 512;
    sd_capacity = sd_card_capacity_get(SDIO);
    block_count = sd_capacity * 2;
    LOG_I("## Device size is %dKB ##", sd_capacity);
    LOG_D("## Block size is %dB ##", block_size = 512);
    LOG_D("## Block count is %d ##", block_count);

    if (sd0.sd_cardinfo.card_csd.read_bl_partial)
    {
        LOG_D("## Partial blocks for read allowed ##");
    }
    if (sd0.sd_cardinfo.card_csd.write_bl_partial)
    {
        LOG_D("## Partial blocks for write allowed ##");
    }
    temp_ccc = sd0.sd_cardinfo.card_csd.ccc;
    LOG_D("## CardCommandClasses is: %x ##", temp_ccc);
    if ((SD_CCC_BLOCK_READ & temp_ccc) && (SD_CCC_BLOCK_WRITE & temp_ccc))
    {
        LOG_D("## Block operation supported ##");
    }
    if (SD_CCC_ERASE & temp_ccc)
    {
        LOG_D("## Erase supported ##");
    }
    if (SD_CCC_WRITE_PROTECTION & temp_ccc)
    {
        LOG_D("## Write protection supported ##");
    }
    if (SD_CCC_LOCK_CARD & temp_ccc)
    {
        LOG_D("## Lock unlock supported ##");
    }
    if (SD_CCC_APPLICATION_SPECIFIC & temp_ccc)
    {
        LOG_D("## Application specific supported ##");
    }
    if (SD_CCC_IO_MODE & temp_ccc)
    {
        LOG_D("## I/O mode supported ##");
    }
    if (SD_CCC_SWITCH & temp_ccc)
    {
        LOG_D("## Switch function supported ##");
    }
}

void SDIO0_IRQHandler(void)
{
    sd_error_enum status;

    rt_interrupt_enter();

    status = sd_interrupts_process(sd0.sdio_periph);
    if (SD_OK != status)
    {
        LOG_D("irq:%d", status);
    }

    rt_interrupt_leave();
}

/*!
    \brief      initialize the SD card and make it in standby state
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_init(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;

    /* configure the RCU and GPIO, deinitialize the SDIO */
    rcu_config();
    gpio_config();
    sdio_deinit(SDIO);

    /* configure the clock and work voltage */
    status = sd_power_on(SDIO);
    if (SD_OK != status)
    {
        LOG_E("sd_power_on fail");
        return status;
    }

    /* initialize the card and get CID and CSD of the card */
    status = sd_card_init(SDIO);
    if (SD_OK != status)
    {
        LOG_E("sd_card_init fail");
        return status;
    }

    /* configure the SDIO peripheral */
    sdio_clock_config(SDIO, SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_INIT);
    sdio_bus_mode_set(SDIO, SDIO_BUSMODE_1BIT);
    sdio_hardware_clock_enable(SDIO);

    return status;
}

/*!
    \brief      initialize the card and get CID and CSD of the card
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_card_init(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    uint16_t temp_rca = 0x01;

    if (SDIO_POWER_OFF == sdio_power_state_get(SDIO))
    {
        status = SD_OPERATION_IMPROPER;
        return status;
    }

    /* the card is not I/O only card */
    if (SDIO_SECURE_DIGITAL_IO_CARD != card_opration.cardtype)
    {
        /* send CMD2(SD_CMD_ALL_SEND_CID) to get the CID numbers */
        sdio_command_response_config(SDIO, SD_CMD_ALL_SEND_CID, (uint32_t)0x0, SDIO_RESPONSETYPE_LONG);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r2_error_check(SDIO);
        if (SD_OK != status)
        {
            return status;
        }

        /* store the CID numbers */
        card_opration.sd_cid[0] = sdio_response_get(SDIO, SDIO_RESPONSE0);
        card_opration.sd_cid[1] = sdio_response_get(SDIO, SDIO_RESPONSE1);
        card_opration.sd_cid[2] = sdio_response_get(SDIO, SDIO_RESPONSE2);
        card_opration.sd_cid[3] = sdio_response_get(SDIO, SDIO_RESPONSE3);
    }

    /* the card is SD memory card or the I/O card has the memory portion */
    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V3_0 == card_opration.cardtype) ||
        (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype) || (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == card_opration.cardtype))
    {
        /* send CMD3(SEND_RELATIVE_ADDR) to ask the card to publish a new relative address (RCA) */
        sdio_command_response_config(SDIO, SD_CMD_SEND_RELATIVE_ADDR, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r6_error_check(SDIO, SD_CMD_SEND_RELATIVE_ADDR, &temp_rca);
        if (SD_OK != status)
        {
            return status;
        }
    }

    if (SDIO_SECURE_DIGITAL_IO_CARD != card_opration.cardtype)
    {
        /* the card is not I/O only card */
        card_opration.sd_rca = temp_rca;

        /* send CMD9(SEND_CSD) to get the addressed card's card-specific data (CSD) */
        sdio_command_response_config(SDIO, SD_CMD_SEND_CSD, (uint32_t)(temp_rca << SD_RCA_SHIFT), SDIO_RESPONSETYPE_LONG);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r2_error_check(SDIO);
        if (SD_OK != status)
        {
            return status;
        }

        /* store the card-specific data (CSD) */
        card_opration.sd_csd[0] = sdio_response_get(SDIO, SDIO_RESPONSE0);
        card_opration.sd_csd[1] = sdio_response_get(SDIO, SDIO_RESPONSE1);
        card_opration.sd_csd[2] = sdio_response_get(SDIO, SDIO_RESPONSE2);
        card_opration.sd_csd[3] = sdio_response_get(SDIO, SDIO_RESPONSE3);
    }
    return status;
}

/*!
    \brief      configure the clock and the work voltage, and get the card type
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_power_on(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    uint32_t response = 0, count = 0;
    uint8_t busyflag = 0;
    uint32_t timedelay = 0U;

    /* configure the SDIO peripheral */
    sdio_clock_config(SDIO, SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_INIT);
    sdio_bus_mode_set(SDIO, SDIO_BUSMODE_1BIT);
    sdio_hardware_clock_disable(SDIO);
    sdio_power_state_set(SDIO, SDIO_POWER_ON);

    /* time delay for power up */
    timedelay = 500U;
    while (timedelay > 0U)
    {
        timedelay--;
    }

    /* send CMD0(GO_IDLE_STATE) to reset the card */
    sdio_command_response_config(SDIO, SD_CMD_GO_IDLE_STATE, (uint32_t)0x0, SDIO_RESPONSETYPE_NO);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    /* enable the CSM */
    sdio_csm_enable(SDIO);

    /* check if command sent error occurs */
    status = cmdsent_error_check(SDIO);
    if (SD_OK != status)
    {
        return status;
    }

    /* send CMD8(SEND_IF_COND) to get SD memory card interface condition */
    sdio_command_response_config(SDIO, SD_CMD_SEND_IF_COND, SD_CHECK_PATTERN, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);

    if (SD_OK == r7_error_check(SDIO))
    {
        /* SD Card 2.0 */
        card_opration.cardtype = SDIO_STD_CAPACITY_SD_CARD_V2_0;
        card_opration.sdcardtype = SD_HIGH_CAPACITY;
    }

    /* send CMD55(APP_CMD) to indicate next command is application specific command */
    sdio_command_response_config(SDIO, SD_CMD_APP_CMD, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);

    if (SD_OK == r1_error_check(SDIO, SD_CMD_APP_CMD))
    {
        /* SD memory card */
        while ((!busyflag) && (count < SD_MAX_VOLT_VALIDATION))
        {
            /* send CMD55(APP_CMD) to indicate next command is application specific command */
            sdio_command_response_config(SDIO, SD_CMD_APP_CMD, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_APP_CMD);
            if (SD_OK != status)
            {
                return status;
            }

            /* send ACMD41(SD_SEND_OP_COND) to get host capacity support information (HCS) and OCR content */
            sdio_command_response_config(SDIO, SD_APPCMD_SD_SEND_OP_COND, (SD_VOLTAGE_WINDOW | SD_VOLTAGE_18V | card_opration.sdcardtype), SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r3_error_check(SDIO);
            if (SD_OK != status)
            {
                return status;
            }
            /* get the response and check card power up status bit(busy) */
            response = sdio_response_get(SDIO, SDIO_RESPONSE0);
            busyflag = (uint8_t)((response >> 31) & (uint32_t)0x01);
            ++count;
        }
        if (count >= SD_MAX_VOLT_VALIDATION)
        {
            status = SD_VOLTRANGE_INVALID;
            return status;
        }
        if (response & SD_HIGH_CAPACITY)
        {
            LOG_I("SD is SD_HIGH_CAPACITY");
            /* SDHC card */
            card_opration.cardcapacity = SD_SDHC_SDXC;
            card_opration.cardtype = SDIO_HIGH_CAPACITY_SD_CARD;
        }
    }
    return status;
}

/*!
    \brief      close the power of SDIO
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_power_off(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    sdio_power_state_set(SDIO, SDIO_POWER_OFF);
    return status;
}

/*!
    \brief      configure the bus mode
    \param[in]  busmode: the bus mode
      \arg        SDIO_BUSMODE_1BIT: 1-bit SDIO card bus mode
      \arg        SDIO_BUSMODE_4BIT: 4-bit SDIO card bus mode
      \arg        SDIO_BUSMODE_8BIT: 8-bit SDIO card bus mode (MMC only)
    \param[in]  speed: the bus speed mode
      \arg        SD_SPEED_DEFAULT: default bus speed
      \arg        SD_SPEED_HIGH: high bus speed
      \arg        SD_SPEED_SDR25: SDR25 bus speed
      \arg        SD_SPEED_SDR50: SDR50 bus speed
      \arg        SD_SPEED_SDR104: SDR104 bus speed
      \arg        SD_SPEED_DDR50: DDR50 bus speed
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_bus_mode_config(uint32_t SDIO, uint32_t busmode, uint32_t speed)
{
    sd_error_enum status = SD_OK;
    uint32_t count, clk_div;

    if (SDIO_MULTIMEDIA_CARD == card_opration.cardtype)
    {
        /* MMC card doesn't support this function */
        status = SD_FUNCTION_UNSUPPORTED;
        return status;
    }
    else if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype) ||
             (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype))
    {
        if (SDIO_BUSMODE_8BIT == busmode)
        {
            /* 8 bit bus mode doesn't support */
            status = SD_FUNCTION_UNSUPPORTED;
            return status;
        }
        else if (SDIO_BUSMODE_4BIT == busmode)
        {
            /* configure SD bus width and the SDIO */
            status = sd_bus_width_config(SDIO, SD_BUS_WIDTH_4BIT);
            if (SD_OK == status)
            {
                sdio_bus_mode_set(SDIO, busmode);
            }
        }
        else if (SDIO_BUSMODE_1BIT == busmode)
        {
            /* configure SD bus width and the SDIO */
            status = sd_bus_width_config(SDIO, SD_BUS_WIDTH_1BIT);
            if (SD_OK == status)
            {
                sdio_bus_mode_set(SDIO, busmode);
            }
        }
        else
        {
            status = SD_PARAMETER_INVALID;
        }
    }

    if ((speed != SD_SPEED_DEFAULT) && (speed != SD_SPEED_HIGH))
    {
        /* switch UHS-I speed mode */
        switch (speed)
        {
        case SD_SPEED_SDR25:
            clk_div = SD_CLK_DIV_TRANS_SDR25SPEED;
            break;
        case SD_SPEED_SDR50:
            clk_div = SD_CLK_DIV_TRANS_SDR50SPEED;
            break;
        case SD_SPEED_SDR104:
            clk_div = SD_CLK_DIV_TRANS_SDR104SPEED;
            break;
        case SD_SPEED_DDR50:
            clk_div = SD_CLK_DIV_TRANS_DDR50SPEED;
            break;
        default:
            clk_div = SD_CLK_DIV_TRANS_DSPEED;
            break;
        }

        /* send CMD16(SET_BLOCKLEN) to set the block length */
        sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)64, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);

        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
        if (SD_OK != status)
        {
            return status;
        }

        sdio_data_config(SDIO, SD_DATATIMEOUT, 64, SDIO_DATABLOCKSIZE_64BYTES);
        sdio_data_transfer_config(SDIO, SDIO_TRANSDIRECTION_TOSDIO, SDIO_TRANSMODE_BLOCKCOUNT);
        sdio_dsm_enable(SDIO);

        /* SDR25 0x80FFFF01U  SDR50 0x80FF1F02U  SDR104 0x80FF1F03U  DDR50 0x80FF1F04U */
        sdio_command_response_config(SDIO, SD_APPCMD_SET_BUS_WIDTH, speed, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_APPCMD_SET_BUS_WIDTH);
        if (SD_OK != status)
        {
            return status;
        }

        while (!sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))
        {
            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_RFH))
            {
                /* at least 8 words can be read in the FIFO */
                for (count = 0; count < SD_FIFOHALF_WORDS; count++)
                {
                    sdio_data_read(SDIO);
                }
            }
        }
        /* clear the SDIO_INTC flags */
        sdio_flag_clear(SDIO, SDIO_MASK_INTC_FLAGS);
        /* change the clock to UHS-I speed , user according to the speed configuration */
        sdio_clock_config(SDIO, SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKPWRSAVE_DISABLE, clk_div);
        if (speed == SD_SPEED_DDR50)
        {
            /* set SDIO data rate */
            sdio_data_rate_set(SDIO, SDIO_DATA_RATE_DDR);
        }
        sdio_hardware_clock_enable(SDIO);
    }
    else if (speed == SD_SPEED_HIGH)
    {
        /* change the clock to high speed , user according to the speed configuration */
        sdio_clock_config(SDIO, SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_TRANS_HSPEED);
        sdio_hardware_clock_enable(SDIO);
    }
    else if (speed == SD_SPEED_DEFAULT)
    {
        /* change the clock to default speed , user according to the speed configuration */
        sdio_clock_config(SDIO, SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_TRANS_DSPEED);
        sdio_hardware_clock_enable(SDIO);
    }

    return status;
}

/*!
    \brief      switch 1.8V power level of SD card
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_card_voltage_switch(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    /* send CMD11(SD_CMD_VOLATAGE_SWITCH) switch to 1.8V bus signaling level */
    sdio_command_response_config(SDIO, SD_CMD_VOLATAGE_SWITCH, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);

    status = r1_error_check(SDIO, SD_CMD_VOLATAGE_SWITCH);
    return status;
}

/*!
    \brief      configure the mode of transmission
    \param[in]  txmode: transfer mode
      \arg        SD_DMA_MODE: DMA mode
      \arg        SD_POLLING_MODE: polling mode
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_transfer_mode_config(uint32_t txmode)
{
    sd_error_enum status = SD_OK;
    /* set the transfer mode */
    if ((SD_DMA_MODE == txmode) || (SD_POLLING_MODE == txmode))
    {
        card_opration.transmode = txmode;
    }
    else
    {
        status = SD_PARAMETER_INVALID;
    }
    return status;
}

/*!
    \brief      read a block data into a buffer from the specified address of a card
    \param[out] preadbuffer: a pointer that store a block read data
    \param[in]  readaddr: the read data address
    \param[in]  blocksize: the data block size
    \retval     sd_error_enum
*/
sd_error_enum sd_block_read(uint32_t SDIO, uint32_t *preadbuffer, uint32_t readaddr, uint16_t blocksize)
{
    /* initialize the variables */
    sd_error_enum status = SD_OK;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = preadbuffer;

    if (NULL == preadbuffer)
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    card_opration.transerror = SD_OK;
    card_opration.transend = 0;
    card_opration.totalnumber_bytes = 0;
    /* clear all DSM configuration */
    sdio_data_config(SDIO, 0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_dsm_disable(SDIO);
    sdio_idma_disable(SDIO);

    /* check whether the card is locked */
    if (sdio_response_get(SDIO, SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }

    /* blocksize is fixed in 512B for SDHC card */
    if (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype)
    {
        blocksize = 512;
        readaddr /= 512;
    }

    align = blocksize & (blocksize - 1);
    if ((blocksize > 0) && (blocksize <= 2048) && (0 == align))
    {
        datablksize = sd_datablocksize_get(blocksize);
        /* send CMD16(SET_BLOCKLEN) to set the block length */
        sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);

        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
        if (SD_OK != status)
        {
            return status;
        }
    }
    else
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    card_opration.stopcondition = 0;
    card_opration.totalnumber_bytes = blocksize;

    if (SD_POLLING_MODE == card_opration.transmode)
    {
        /* configure SDIO data transmisson */
        sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
        sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
        sdio_trans_start_enable(SDIO);

        /* send CMD17(READ_SINGLE_BLOCK) to read a block */
        sdio_command_response_config(SDIO, SD_CMD_READ_SINGLE_BLOCK, (uint32_t)readaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_READ_SINGLE_BLOCK);
        if (SD_OK != status)
        {
            return status;
        }

        /* polling mode */
        while (!sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))
        {
            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_RFH))
            {
                /* at least 8 words can be read in the FIFO */
                for (count = 0; count < SD_FIFOHALF_WORDS; count++)
                {
                    *(ptempbuff + count) = sdio_data_read(SDIO);
                }
                ptempbuff += SD_FIFOHALF_WORDS;
            }
        }

        sdio_trans_start_disable(SDIO);

        /* whether some error occurs and return it */
        if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR))
        {
            status = SD_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO, SDIO_FLAG_DTCRCERR);
            return status;
        }
        else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTTMOUT))
        {
            status = SD_DATA_TIMEOUT;
            sdio_flag_clear(SDIO, SDIO_FLAG_DTTMOUT);
            return status;
        }
        else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_RXORE))
        {
            status = SD_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO, SDIO_FLAG_RXORE);
            return status;
        }
        else
        {
        }
        while ((SET != sdio_flag_get(SDIO, SDIO_FLAG_RFE)) && (SET == sdio_flag_get(SDIO, SDIO_FLAG_DATSTA)))
        {
            *ptempbuff = sdio_data_read(SDIO);
            ++ptempbuff;
        }
        /* clear the DATA_FLAGS flags */
        sdio_flag_clear(SDIO, SDIO_MASK_DATA_FLAGS);
    }
    else if (SD_DMA_MODE == card_opration.transmode)
    {
        /* DMA mode */
        /* enable the SDIO corresponding interrupts and DMA function */
        sdio_interrupt_enable(SDIO, SDIO_INT_CCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_RXORE | SDIO_INT_DTEND);

        dma_config(SDIO, preadbuffer, (uint32_t)(blocksize >> 5));
        sdio_idma_enable(SDIO);

        /* configure SDIO data transmisson */
        sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
        sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
        sdio_trans_start_enable(SDIO);

        /* send CMD17(READ_SINGLE_BLOCK) to read a block */
        sdio_command_response_config(SDIO, SD_CMD_READ_SINGLE_BLOCK, (uint32_t)readaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_READ_SINGLE_BLOCK);
        if (SD_OK != status)
        {
            return status;
        }

        while ((0U == card_opration.transend) && (SD_OK == card_opration.transerror))
        {
        }
        if (SD_OK != card_opration.transerror)
        {
            return card_opration.transerror;
        }
    }
    else
    {
        status = SD_PARAMETER_INVALID;
    }
    return status;
}

/*!
    \brief      read multiple blocks data into a buffer from the specified address of a card
    \param[out] preadbuffer: a pointer that store multiple blocks read data
    \param[in]  readaddr: the read data address
    \param[in]  blocksize: the data block size
    \param[in]  blocksnumber: number of blocks that will be read
    \retval     sd_error_enum
*/
sd_error_enum sd_multiblocks_read(uint32_t SDIO, uint32_t *preadbuffer, uint32_t readaddr, uint16_t blocksize, uint32_t blocksnumber)
{
    /* initialize the variables */
    sd_error_enum status = SD_OK;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = preadbuffer;

    if (NULL == preadbuffer)
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    card_opration.transerror = SD_OK;
    card_opration.transend = 0;
    card_opration.totalnumber_bytes = 0;
    /* clear all DSM configuration */
    sdio_data_config(SDIO, 0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_dsm_disable(SDIO);
    sdio_idma_disable(SDIO);

    /* check whether the card is locked */
    if (sdio_response_get(SDIO, SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }

    /* blocksize is fixed in 512B for SDHC card */
    if (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype)
    {
        blocksize = 512;
        readaddr /= 512;
    }

    align = blocksize & (blocksize - 1);
    if ((blocksize > 0) && (blocksize <= 2048) && (0 == align))
    {
        datablksize = sd_datablocksize_get(blocksize);
        /* send CMD16(SET_BLOCKLEN) to set the block length */
        sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);

        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
        if (SD_OK != status)
        {
            return status;
        }
    }
    else
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    if (blocksnumber > 1)
    {
        if (blocksnumber * blocksize > SD_MAX_DATA_LENGTH)
        {
            /* exceeds the maximum length */
            status = SD_PARAMETER_INVALID;
            return status;
        }

        card_opration.stopcondition = 1;
        card_opration.totalnumber_bytes = blocksnumber * blocksize;

        if (SD_POLLING_MODE == card_opration.transmode)
        {
            /* configure the SDIO data transmisson */
            sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
            sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
            sdio_trans_start_enable(SDIO);

            /* send CMD18(READ_MULTIPLE_BLOCK) to read multiple blocks */
            sdio_command_response_config(SDIO, SD_CMD_READ_MULTIPLE_BLOCK, readaddr, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);

            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_READ_MULTIPLE_BLOCK);
            if (SD_OK != status)
            {
                return status;
            }

            /* polling mode */
            while (!sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND | SDIO_FLAG_DTBLKEND))
            {
                if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_RFH))
                {
                    /* at least 8 words can be read in the FIFO */
                    for (count = 0; count < SD_FIFOHALF_WORDS; count++)
                    {
                        *(ptempbuff + count) = sdio_data_read(SDIO);
                    }
                    ptempbuff += SD_FIFOHALF_WORDS;
                }
            }

            /* whether some error occurs and return it */
            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR))
            {
                status = SD_DATA_CRC_ERROR;
                sdio_flag_clear(SDIO, SDIO_FLAG_DTCRCERR);
                return status;
            }
            else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTTMOUT))
            {
                status = SD_DATA_TIMEOUT;
                sdio_flag_clear(SDIO, SDIO_FLAG_DTTMOUT);
                return status;
            }
            else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_RXORE))
            {
                status = SD_RX_OVERRUN_ERROR;
                sdio_flag_clear(SDIO, SDIO_FLAG_RXORE);
                return status;
            }
            else
            {
            }
            while ((SET != sdio_flag_get(SDIO, SDIO_FLAG_RFE)) && (SET == sdio_flag_get(SDIO, SDIO_FLAG_DATSTA)))
            {
                *ptempbuff = sdio_data_read(SDIO);
                ++ptempbuff;
            }

            sdio_trans_start_disable(SDIO);

            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTEND))
            {
                if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype) ||
                    (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype))
                {
                    /* send CMD12(STOP_TRANSMISSION) to stop transmission */
                    sdio_command_response_config(SDIO, SD_CMD_STOP_TRANSMISSION, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
                    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
                    sdio_csm_enable(SDIO);
                    /* check if some error occurs */
                    status = r1_error_check(SDIO, SD_CMD_STOP_TRANSMISSION);
                    if (SD_OK != status)
                    {
                        return status;
                    }
                }
            }
            sdio_flag_clear(SDIO, SDIO_MASK_DATA_FLAGS);
        }
        else if (SD_DMA_MODE == card_opration.transmode)
        {
            /* DMA mode */
            /* enable the SDIO corresponding interrupts and DMA function */
            sdio_interrupt_enable(SDIO, SDIO_INT_CCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_RXORE | SDIO_INT_DTEND);
            dma_config(SDIO, preadbuffer, (uint32_t)(blocksize >> 5));
            sdio_idma_enable(SDIO);
            /* configure SDIO data transmisson */
            sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
            sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
            sdio_trans_start_enable(SDIO);

            /* send CMD18(READ_MULTIPLE_BLOCK) to read multiple blocks */
            sdio_command_response_config(SDIO, SD_CMD_READ_MULTIPLE_BLOCK, readaddr, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_READ_MULTIPLE_BLOCK);
            if (SD_OK != status)
            {
                return status;
            }

            while ((0 == card_opration.transend) && (SD_OK == card_opration.transerror))
            {
            }
            if (SD_OK != card_opration.transerror)
            {
                return card_opration.transerror;
            }
        }
        else
        {
            status = SD_PARAMETER_INVALID;
        }
    }
    return status;
}

/*!
    \brief      write a block data to the specified address of a card
    \param[in]  pwritebuffer: a pointer that store a block data to be transferred
    \param[in]  writeaddr: the read data address
    \param[in]  blocksize: the data block size
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_block_write(uint32_t SDIO, uint32_t *pwritebuffer, uint32_t writeaddr, uint16_t blocksize)
{
    /* initialize the variables */
    sd_error_enum status = SD_OK;
    uint8_t cardstate = 0;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = pwritebuffer;
    uint32_t transbytes = 0, restwords = 0, response = 0;
    __IO uint32_t timeout = 0;

    if (NULL == pwritebuffer)
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    card_opration.transerror = SD_OK;
    card_opration.transend = 0;
    card_opration.totalnumber_bytes = 0;
    /* clear all DSM configuration */
    sdio_data_config(SDIO, 0U, 0U, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_dsm_disable(SDIO);
    sdio_idma_disable(SDIO);

    /* check whether the card is locked */
    if (sdio_response_get(SDIO, SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }

    /* blocksize is fixed in 512B for SDHC card */
    if (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype)
    {
        blocksize = 512;
        writeaddr /= 512;
    }

    align = blocksize & (blocksize - 1);
    if ((blocksize > 0) && (blocksize <= 2048) && (0 == align))
    {
        datablksize = sd_datablocksize_get(blocksize);
        /* send CMD16(SET_BLOCKLEN) to set the block length */
        sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);

        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
        if (SD_OK != status)
        {
            LOG_E("errcheck fail:%d", status);
            return status;
        }
    }
    else
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    /* send CMD13(SEND_STATUS), addressed card sends its status registers */
    sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
    if (SD_OK != status)
    {
        return status;
    }

    response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    timeout = 100000U;

    while ((0 == (response & SD_R1_READY_FOR_DATA)) && (timeout > 0))
    {
        /* continue to send CMD13 to polling the state of card until buffer empty or timeout */
        --timeout;
        /* send CMD13(SEND_STATUS), addressed card sends its status registers */
        sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
        if (SD_OK != status)
        {
            return status;
        }
        response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    }
    if (0 == timeout)
    {
        return SD_ERROR;
    }

    card_opration.stopcondition = 0U;
    card_opration.totalnumber_bytes = blocksize;

    /* configure the SDIO data transmisson */
    sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_trans_start_enable(SDIO);

    if (SD_POLLING_MODE == card_opration.transmode)
    {
        /* send CMD24(WRITE_BLOCK) to write a block */
        sdio_command_response_config(SDIO, SD_CMD_WRITE_BLOCK, writeaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_WRITE_BLOCK);
        if (SD_OK != status)
        {
            return status;
        }

        /* polling mode */
        while (!sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_TXURE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))
        {
            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_TFH))
            {
                /* at least 8 words can be written into the FIFO */
                if ((card_opration.totalnumber_bytes - transbytes) < SD_FIFOHALF_BYTES)
                {
                    restwords = (card_opration.totalnumber_bytes - transbytes) / 4 + (((card_opration.totalnumber_bytes - transbytes) % 4 == 0) ? 0 : 1);
                    for (count = 0; count < restwords; count++)
                    {
                        sdio_data_write(SDIO, *ptempbuff);
                        ++ptempbuff;
                        transbytes += 4;
                    }
                }
                else
                {
                    for (count = 0; count < SD_FIFOHALF_WORDS; count++)
                    {
                        sdio_data_write(SDIO, *(ptempbuff + count));
                    }
                    /* 8 words(32 bytes) has been transferred */
                    ptempbuff += SD_FIFOHALF_WORDS;
                    transbytes += SD_FIFOHALF_BYTES;
                }
            }
        }
        sdio_trans_start_disable(SDIO);
        /* whether some error occurs and return it */
        if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR))
        {
            status = SD_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO, SDIO_FLAG_DTCRCERR);
            return status;
        }
        else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTTMOUT))
        {
            status = SD_DATA_TIMEOUT;
            sdio_flag_clear(SDIO, SDIO_FLAG_DTTMOUT);
            return status;
        }
        else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_TXURE))
        {
            status = SD_TX_UNDERRUN_ERROR;
            sdio_flag_clear(SDIO, SDIO_FLAG_TXURE);
            return status;
        }
        else
        {
            /* if else end */
        }
    }
    else if (SD_DMA_MODE == card_opration.transmode)
    {
        /* DMA mode */
        /* enable the SDIO corresponding interrupts and DMA */
        sdio_interrupt_enable(SDIO, SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_TXURE | SDIO_INT_DTEND);
        dma_config(SDIO, pwritebuffer, (uint32_t)(blocksize >> 5));
        sdio_idma_enable(SDIO);

        /* send CMD24(WRITE_BLOCK) to write a block */
        sdio_command_response_config(SDIO, SD_CMD_WRITE_BLOCK, writeaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_WRITE_BLOCK);
        if (SD_OK != status)
        {
            return status;
        }

        while ((0 == card_opration.transend) && (SD_OK == card_opration.transerror))
        {
        }

        if (SD_OK != card_opration.transerror)
        {
            return card_opration.transerror;
        }
    }
    else
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    /* clear the DATA_FLAGS flags */
    sdio_flag_clear(SDIO, SDIO_MASK_DATA_FLAGS);
    /* get the card state and wait the card is out of programming and receiving state */
    status = sd_card_state_get(SDIO, &cardstate);
    while ((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate)))
    {
        status = sd_card_state_get(SDIO, &cardstate);
    }

    return status;
}

/*!
    \brief      write multiple blocks data to the specified address of a card
    \param[in]  pwritebuffer: a pointer that store multiple blocks data to be transferred
    \param[in]  writeaddr: the read data address
    \param[in]  blocksize: the data block size
    \param[in]  blocksnumber: number of blocks that will be written
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_multiblocks_write(uint32_t SDIO, uint32_t *pwritebuffer, uint32_t writeaddr, uint16_t blocksize, uint32_t blocksnumber)
{
    /* initialize the variables */
    sd_error_enum status = SD_OK;
    uint8_t cardstate = 0;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = pwritebuffer;
    uint32_t transbytes = 0U, restwords = 0U, response = 0U;
    __IO uint32_t timeout = 0;

    if (NULL == pwritebuffer)
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    card_opration.transerror = SD_OK;
    card_opration.transend = 0;
    card_opration.totalnumber_bytes = 0;
    /* clear all DSM configuration */
    sdio_data_config(SDIO, 0U, 0U, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_dsm_disable(SDIO);
    sdio_idma_disable(SDIO);

    /* check whether the card is locked */
    if (sdio_response_get(SDIO, SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }

    /* blocksize is fixed in 512B for SDHC card */
    if (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype)
    {
        blocksize = 512;
        writeaddr /= 512;
    }

    align = blocksize & (blocksize - 1);
    if ((blocksize > 0) && (blocksize <= 2048) && (0 == align))
    {
        datablksize = sd_datablocksize_get(blocksize);
        /* send CMD16(SET_BLOCKLEN) to set the block length */
        sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);

        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
        if (SD_OK != status)
        {
            LOG_D("#st:%d\n", status);
            return status;
        }
    }
    else
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    /* send CMD13(SEND_STATUS), addressed card sends its status registers */
    sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
    if (SD_OK != status)
    {
        return status;
    }

    response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    timeout = 100000U;

    while ((0U == (response & SD_R1_READY_FOR_DATA)) && (timeout > 0U))
    {
        /* continue to send CMD13 to polling the state of card until buffer empty or timeout */
        --timeout;
        /* send CMD13(SEND_STATUS), addressed card sends its status registers */
        sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
        if (SD_OK != status)
        {
            return status;
        }
        response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    }
    if (0U == timeout)
    {
        return SD_ERROR;
    }

    if (blocksnumber > 1)
    {
        if (blocksnumber * blocksize > SD_MAX_DATA_LENGTH)
        {
            status = SD_PARAMETER_INVALID;
            return status;
        }

        if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype) ||
            (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype))
        {
            /* send CMD55(APP_CMD) to indicate next command is application specific command */
            sdio_command_response_config(SDIO, SD_CMD_APP_CMD, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_APP_CMD);
            if (SD_OK != status)
            {
                return status;
            }

            /* send ACMD23(SET_WR_BLK_ERASE_COUNT) to set the number of write blocks to be preerased before writing */
            sdio_command_response_config(SDIO, SD_APPCMD_SET_WR_BLK_ERASE_COUNT, blocksnumber, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_APPCMD_SET_WR_BLK_ERASE_COUNT);
            if (SD_OK != status)
            {
                return status;
            }
        }

        card_opration.stopcondition = 1U;
        card_opration.totalnumber_bytes = blocksnumber * blocksize;

        if (SD_POLLING_MODE == card_opration.transmode)
        {
            /* configure the SDIO data transmisson */
            sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
            sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
            sdio_trans_start_enable(SDIO);

            /* send CMD25(WRITE_MULTIPLE_BLOCK) to continuously write blocks of data */
            sdio_command_response_config(SDIO, SD_CMD_WRITE_MULTIPLE_BLOCK, writeaddr, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_WRITE_MULTIPLE_BLOCK);
            if (SD_OK != status)
            {
                return status;
            }

            /* polling mode */
            while (!sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_TXURE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))
            {
                if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_TFH))
                {
                    /* at least 8 words can be written into the FIFO */
                    if ((card_opration.totalnumber_bytes - transbytes) < SD_FIFOHALF_BYTES)
                    {
                        restwords = (card_opration.totalnumber_bytes - transbytes) / 4U + (((card_opration.totalnumber_bytes - transbytes) % 4U == 0U) ? 0U : 1U);
                        for (count = 0U; count < restwords; count++)
                        {
                            sdio_data_write(SDIO, *ptempbuff);
                            ++ptempbuff;
                            transbytes += 4U;
                        }
                    }
                    else
                    {
                        for (count = 0U; count < SD_FIFOHALF_WORDS; count++)
                        {
                            sdio_data_write(SDIO, *(ptempbuff + count));
                        }
                        /* 8 words(32 bytes) has been transferred */
                        ptempbuff += SD_FIFOHALF_WORDS;
                        transbytes += SD_FIFOHALF_BYTES;
                    }
                }
            }
            sdio_trans_start_disable(SDIO);

            /* whether some error occurs and return it */
            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR))
            {
                status = SD_DATA_CRC_ERROR;
                sdio_flag_clear(SDIO, SDIO_FLAG_DTCRCERR);
                return status;
            }
            else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTTMOUT))
            {
                status = SD_DATA_TIMEOUT;
                sdio_flag_clear(SDIO, SDIO_FLAG_DTTMOUT);
                return status;
            }
            else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_TXURE))
            {
                status = SD_TX_UNDERRUN_ERROR;
                sdio_flag_clear(SDIO, SDIO_FLAG_TXURE);
                return status;
            }
            else
            {
                /* if else end */
            }

            if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTEND))
            {
                if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype) ||
                    (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype))
                {
                    /* send CMD12(STOP_TRANSMISSION) to stop transmission */
                    sdio_command_response_config(SDIO, SD_CMD_STOP_TRANSMISSION, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
                    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
                    sdio_csm_enable(SDIO);
                    /* check if some error occurs */
                    status = r1_error_check(SDIO, SD_CMD_STOP_TRANSMISSION);
                    if (SD_OK != status)
                    {
                        return status;
                    }
                }
            }
        }
        else if (SD_DMA_MODE == card_opration.transmode)
        {
            /* DMA mode */
            /* enable the SDIO corresponding interrupts and DMA */
            sdio_interrupt_enable(SDIO, SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_TXURE | SDIO_INT_DTEND);
            dma_config(SDIO, pwritebuffer, (uint32_t)(blocksize >> 5));
            sdio_idma_enable(SDIO);

            /* configure the SDIO data transmisson */
            sdio_data_config(SDIO, SD_DATATIMEOUT, card_opration.totalnumber_bytes, datablksize);
            sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
            sdio_trans_start_enable(SDIO);

            /* send CMD25(WRITE_MULTIPLE_BLOCK) to continuously write blocks of data */
            sdio_command_response_config(SDIO, SD_CMD_WRITE_MULTIPLE_BLOCK, writeaddr, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_WRITE_MULTIPLE_BLOCK);
            if (SD_OK != status)
            {
                return status;
            }
            while ((0U == card_opration.transend) && (SD_OK == card_opration.transerror))
            {
            }

            if (SD_OK != card_opration.transerror)
            {
                return card_opration.transerror;
            }
        }
        else
        {
            status = SD_PARAMETER_INVALID;
            return status;
        }
    }
    /* clear the DATA_FLAGS flags */
    sdio_flag_clear(SDIO, SDIO_MASK_DATA_FLAGS);
    /* get the card state and wait the card is out of programming and receiving state */
    status = sd_card_state_get(SDIO, &cardstate);
    while ((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate)))
    {
        status = sd_card_state_get(SDIO, &cardstate);
    }
    return status;
}

/*!
    \brief      erase a continuous area of a card
    \param[in]  startaddr: the start address
    \param[in]  endaddr: the end address
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_erase(uint32_t SDIO, uint32_t startaddr, uint32_t endaddr)
{
    /* initialize the variables */
    sd_error_enum status = SD_OK;
    uint32_t count = 0, clkdiv = 0;
    __IO uint32_t delay = 0;
    uint8_t cardstate = 0, tempbyte = 0;
    uint16_t tempccc = 0;

    /* get the card command classes from CSD */
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_24_31BITS) >> 24);
    tempccc = (uint16_t)((uint16_t)tempbyte << 4);
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_16_23BITS) >> 16);
    tempccc |= (uint16_t)((uint16_t)(tempbyte & 0xF0) >> 4);
    if (0 == (tempccc & SD_CCC_ERASE))
    {
        /* don't support the erase command */
        status = SD_FUNCTION_UNSUPPORTED;
        return status;
    }
    clkdiv = (SDIO_CLKCTL(SDIO) & SDIO_CLKCTL_DIV);
    clkdiv *= 2U;
    delay = 168000 / clkdiv;

    /* check whether the card is locked */
    if (sdio_response_get(SDIO, SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return (status);
    }

    /* blocksize is fixed in 512B for SDHC card */
    if (SDIO_HIGH_CAPACITY_SD_CARD != card_opration.cardtype)
    {
        startaddr *= 512;
        endaddr *= 512;
    }

    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype) ||
        (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype))
    {
        /* send CMD32(ERASE_WR_BLK_START) to set the address of the first write block to be erased */
        sdio_command_response_config(SDIO, SD_CMD_ERASE_WR_BLK_START, startaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_ERASE_WR_BLK_START);
        if (SD_OK != status)
        {
            return status;
        }

        /* send CMD33(ERASE_WR_BLK_END) to set the address of the last write block of the continuous range to be erased */
        sdio_command_response_config(SDIO, SD_CMD_ERASE_WR_BLK_END, endaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_ERASE_WR_BLK_END);
        if (SD_OK != status)
        {
            return status;
        }
    }

    /* send CMD38(ERASE) to set the address of the first write block to be erased */
    sdio_command_response_config(SDIO, SD_CMD_ERASE, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_ERASE);
    if (SD_OK != status)
    {
        return status;
    }
    /* loop until the counter is reach to the calculated time */
    for (count = 0; count < delay; count++)
    {
    }
    /* get the card state and wait the card is out of programming and receiving state */
    status = sd_card_state_get(SDIO, &cardstate);
    while ((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate)))
    {
        status = sd_card_state_get(SDIO, &cardstate);
    }
    return status;
}

/*!
    \brief      process all the interrupts which the corresponding flags are set
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_interrupts_process(uint32_t SDIO)
{
    card_opration.transerror = SD_OK;
    if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_DTEND))
    {
        /* clear DTEND flag */
        sdio_interrupt_flag_clear(SDIO, SDIO_INT_FLAG_DTEND);
        /* disable idma for idma transfer */
        sdio_idma_disable(SDIO);

        /* disable all the interrupts */
        sdio_interrupt_disable(SDIO, SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND |
                                         SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        sdio_trans_start_disable(SDIO);
        /* send CMD12 to stop data transfer in multipule blocks operation */
        if (1 == card_opration.stopcondition)
        {
            card_opration.transerror = sd_transfer_stop(SDIO);
        }
        else
        {
            card_opration.transerror = SD_OK;
        }
        card_opration.transend = 1;
        card_opration.number_bytes = 0;
        /* clear data flags */
        sdio_flag_clear(SDIO, SDIO_MASK_DATA_FLAGS);
        return card_opration.transerror;
    }

    if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_DTCRCERR | SDIO_INT_FLAG_DTTMOUT | SDIO_INT_FLAG_TXURE | SDIO_INT_FLAG_RXORE))
    {
        /* set different errors */
        if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_DTCRCERR))
        {
            card_opration.transerror = SD_DATA_CRC_ERROR;
        }
        if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_DTTMOUT))
        {
            card_opration.transerror = SD_DATA_TIMEOUT;
        }
        if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_TXURE))
        {
            card_opration.transerror = SD_TX_UNDERRUN_ERROR;
        }
        if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_RXORE))
        {
            card_opration.transerror = SD_RX_OVERRUN_ERROR;
        }
        /* clear data flags */
        sdio_flag_clear(SDIO, SDIO_MASK_DATA_FLAGS);
        /* disable all the interrupts */
        sdio_interrupt_disable(SDIO, SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND |
                                         SDIO_INT_TXURE | SDIO_INT_RXORE);

        sdio_trans_start_disable(SDIO);
        sdio_fifo_reset_enable(SDIO);
        sdio_fifo_reset_disable(SDIO);
        /* send CMD12 to stop data transfer in multipule blocks operation */
        card_opration.transerror = sd_transfer_stop(SDIO);
        sdio_flag_clear(SDIO, SDIO_FLAG_DTABORT);
        card_opration.number_bytes = 0;

        if (card_opration.transmode == SD_DMA_MODE)
        {
            sdio_idma_disable(SDIO);
        }
        return card_opration.transerror;
    }

    if (RESET != sdio_interrupt_flag_get(SDIO, SDIO_INT_FLAG_IDMAERR))
    {
        sdio_interrupt_flag_clear(SDIO, SDIO_INT_FLAG_IDMAERR);
        card_opration.transerror = SD_DMA_ERROR;
        /* disable all the interrupts */
        sdio_interrupt_disable(SDIO, SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND |
                                         SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        sdio_trans_start_disable(SDIO);
        sdio_fifo_reset_enable(SDIO);
        sdio_fifo_reset_disable(SDIO);
        /* send CMD12 to stop data transfer in multipule blocks operation */
        card_opration.transerror = sd_transfer_stop(SDIO);
        sdio_flag_clear(SDIO, SDIO_FLAG_DTABORT);
        card_opration.number_bytes = 0;

        sdio_idma_disable(SDIO);
        return card_opration.transerror;
    }

    return card_opration.transerror;
}

/*!
    \brief      select or deselect a card
    \param[in]  cardrca: the RCA of a card
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_card_select_deselect(uint32_t SDIO, uint16_t cardrca)
{
    sd_error_enum status = SD_OK;

    /* send CMD7(SELECT/DESELECT_CARD) to select or deselect the card */
    sdio_command_response_config(SDIO, SD_CMD_SELECT_DESELECT_CARD, (uint32_t)(cardrca << SD_RCA_SHIFT), SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);

    status = r1_error_check(SDIO, SD_CMD_SELECT_DESELECT_CARD);

    return status;
}

/*!
    \brief      get the card status whose response format R1 contains a 32-bit field
    \param[in]  none
    \param[out] pcardstatus: a pointer that store card status
    \retval     sd_error_enum
*/
sd_error_enum sd_cardstatus_get(uint32_t SDIO, uint32_t *pcardstatus)
{
    sd_error_enum status = SD_OK;
    if (NULL == pcardstatus)
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    /* send CMD13(SEND_STATUS), addressed card sends its status register */
    sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
    if (SD_OK != status)
    {
        return status;
    }

    *pcardstatus = sdio_response_get(SDIO, SDIO_RESPONSE0);
    return status;
}

/*!
    \brief      get SD card capacity
    \param[in]  none
    \param[out] none
    \retval     capacity of the card(KB)
*/
uint32_t sd_card_capacity_get(uint32_t SDIO)
{
    uint8_t tempbyte = 0, devicesize_mult = 0, readblklen = 0;
    uint32_t capacity = 0, devicesize = 0;
    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype))
    {
        /* calculate the c_size(device size) */
        tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_8_15BITS) >> 8);
        devicesize |= (uint32_t)((uint32_t)(tempbyte & 0x03) << 10);
        tempbyte = (uint8_t)(card_opration.sd_csd[1] & SD_MASK_0_7BITS);
        devicesize |= (uint32_t)((uint32_t)tempbyte << 2);
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        devicesize |= (uint32_t)((uint32_t)(tempbyte & 0xC0) >> 6);

        /* calculate the c_size_mult(device size multiplier) */
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        devicesize_mult = (tempbyte & 0x03) << 1;
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_8_15BITS) >> 8);
        devicesize_mult |= (tempbyte & 0x80) >> 7;

        /* calculate the read_bl_len */
        tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_16_23BITS) >> 16);
        readblklen = tempbyte & 0x0F;

        /* capacity = BLOCKNR*BLOCK_LEN, BLOCKNR = (C_SIZE+1)*MULT, MULT = 2^(C_SIZE_MULT+2), BLOCK_LEN = 2^READ_BL_LEN */
        capacity = (devicesize + 1) * (1 << (devicesize_mult + 2));
        capacity *= (1 << readblklen);

        /* change the unit of capacity to KByte */
        capacity /= 1024;
    }
    else if (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype)
    {
        /* calculate the c_size */
        tempbyte = (uint8_t)(card_opration.sd_csd[1] & SD_MASK_0_7BITS);
        devicesize = (uint32_t)((uint32_t)(tempbyte & 0x3F) << 16);
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        devicesize |= (uint32_t)((uint32_t)tempbyte << 8);
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        devicesize |= (uint32_t)tempbyte;

        /* capacity = (c_size+1)*512KByte */
        capacity = (devicesize + 1) * 512;
    }
    return capacity;
}

/*!
    \brief      get the detailed information of the SD card based on received CID and CSD
    \param[in]  none
    \param[out] pcardinfo: a pointer that store the detailed card information
    \retval     sd_error_enum
*/
sd_error_enum sd_card_information_get(uint32_t SDIO, sd_card_info_struct *pcardinfo)
{
    sd_error_enum status = SD_OK;
    uint8_t tempbyte = 0;

    if (NULL == pcardinfo)
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }

    /* store the card type and RCA */
    pcardinfo->card_type = card_opration.cardtype;
    pcardinfo->card_rca = card_opration.sd_rca;

    /* CID byte 0 */
    tempbyte = (uint8_t)((card_opration.sd_cid[0] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.mid = tempbyte;

    /* CID byte 1 */
    tempbyte = (uint8_t)((card_opration.sd_cid[0] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.oid = (uint16_t)((uint16_t)tempbyte << 8);

    /* CID byte 2 */
    tempbyte = (uint8_t)((card_opration.sd_cid[0] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.oid |= (uint16_t)tempbyte;

    /* CID byte 3 */
    tempbyte = (uint8_t)(card_opration.sd_cid[0] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.pnm0 = (uint32_t)((uint32_t)tempbyte << 24);

    /* CID byte 4 */
    tempbyte = (uint8_t)((card_opration.sd_cid[1] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.pnm0 |= (uint32_t)((uint32_t)tempbyte << 16);

    /* CID byte 5 */
    tempbyte = (uint8_t)((card_opration.sd_cid[1] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.pnm0 |= (uint32_t)((uint32_t)tempbyte << 8);

    /* CID byte 6 */
    tempbyte = (uint8_t)((card_opration.sd_cid[1] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.pnm0 |= (uint32_t)(tempbyte);

    /* CID byte 7 */
    tempbyte = (uint8_t)(card_opration.sd_cid[1] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.pnm1 = tempbyte;

    /* CID byte 8 */
    tempbyte = (uint8_t)((card_opration.sd_cid[2] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.prv = tempbyte;

    /* CID byte 9 */
    tempbyte = (uint8_t)((card_opration.sd_cid[2] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.psn = (uint32_t)((uint32_t)tempbyte << 24);

    /* CID byte 10 */
    tempbyte = (uint8_t)((card_opration.sd_cid[2] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.psn |= (uint32_t)((uint32_t)tempbyte << 16);

    /* CID byte 11 */
    tempbyte = (uint8_t)(card_opration.sd_cid[2] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.psn |= (uint32_t)tempbyte;

    /* CID byte 12 */
    tempbyte = (uint8_t)((card_opration.sd_cid[3] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.psn |= (uint32_t)tempbyte;

    /* CID byte 13 */
    tempbyte = (uint8_t)((card_opration.sd_cid[3] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.mdt = (uint16_t)((uint16_t)(tempbyte & 0x0F) << 8);

    /* CID byte 14 */
    tempbyte = (uint8_t)((card_opration.sd_cid[3] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.mdt |= (uint16_t)tempbyte;

    /* CID byte 15 */
    tempbyte = (uint8_t)(card_opration.sd_cid[3] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.cid_crc = (tempbyte & 0xFE) >> 1;

    /* CSD byte 0 */
    tempbyte = (uint8_t)((card_opration.sd_csd[0] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_csd.csd_struct = (tempbyte & 0xC0) >> 6;

    /* CSD byte 1 */
    tempbyte = (uint8_t)((card_opration.sd_csd[0] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_csd.taac = tempbyte;

    /* CSD byte 2 */
    tempbyte = (uint8_t)((card_opration.sd_csd[0] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_csd.nsac = tempbyte;

    /* CSD byte 3 */
    tempbyte = (uint8_t)(card_opration.sd_csd[0] & SD_MASK_0_7BITS);
    pcardinfo->card_csd.tran_speed = tempbyte;

    /* CSD byte 4 */
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_csd.ccc = (uint16_t)((uint16_t)tempbyte << 4);

    /* CSD byte 5 */
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_csd.ccc |= (uint16_t)((uint16_t)(tempbyte & 0xF0) >> 4);
    pcardinfo->card_csd.read_bl_len = tempbyte & 0x0F;

    /* CSD byte 6 */
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_csd.read_bl_partial = (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.write_blk_misalign = (tempbyte & 0x40) >> 6;
    pcardinfo->card_csd.read_blk_misalign = (tempbyte & 0x20) >> 5;
    pcardinfo->card_csd.dsp_imp = (tempbyte & 0x10) >> 4;

    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == card_opration.cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == card_opration.cardtype))
    {
        /* card is SDSC card, CSD version 1.0 */
        pcardinfo->card_csd.c_size = (uint32_t)((uint32_t)(tempbyte & 0x03) << 10);

        /* CSD byte 7 */
        tempbyte = (uint8_t)(card_opration.sd_csd[1] & SD_MASK_0_7BITS);
        pcardinfo->card_csd.c_size |= (uint32_t)((uint32_t)tempbyte << 2);

        /* CSD byte 8 */
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        pcardinfo->card_csd.c_size |= (uint32_t)((uint32_t)(tempbyte & 0xC0) >> 6);
        pcardinfo->card_csd.vdd_r_curr_min = (tempbyte & 0x38) >> 3;
        pcardinfo->card_csd.vdd_r_curr_max = tempbyte & 0x07;

        /* CSD byte 9 */
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        pcardinfo->card_csd.vdd_w_curr_min = (tempbyte & 0xE0) >> 5;
        pcardinfo->card_csd.vdd_w_curr_max = (tempbyte & 0x1C) >> 2;
        pcardinfo->card_csd.c_size_mult = (tempbyte & 0x03) << 1;

        /* CSD byte 10 */
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_8_15BITS) >> 8);
        pcardinfo->card_csd.c_size_mult |= (tempbyte & 0x80) >> 7;

        /* calculate the card block size and capacity */
        pcardinfo->card_blocksize = 1 << (pcardinfo->card_csd.read_bl_len);
        pcardinfo->card_capacity = pcardinfo->card_csd.c_size + 1;
        pcardinfo->card_capacity *= (1 << (pcardinfo->card_csd.c_size_mult + 2));
        pcardinfo->card_capacity *= pcardinfo->card_blocksize;
    }
    else if (SDIO_HIGH_CAPACITY_SD_CARD == card_opration.cardtype)
    {
        /* card is SDHC card, CSD version 2.0 */
        /* CSD byte 7 */
        tempbyte = (uint8_t)(card_opration.sd_csd[1] & SD_MASK_0_7BITS);
        pcardinfo->card_csd.c_size = (uint32_t)((uint32_t)(tempbyte & 0x3F) << 16);

        /* CSD byte 8 */
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        pcardinfo->card_csd.c_size |= (uint32_t)((uint32_t)tempbyte << 8);

        /* CSD byte 9 */
        tempbyte = (uint8_t)((card_opration.sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        pcardinfo->card_csd.c_size |= (uint32_t)tempbyte;

        /* calculate the card block size and capacity */
        pcardinfo->card_blocksize = 512;
        pcardinfo->card_capacity = (pcardinfo->card_csd.c_size + 1) * 512 * 1024;
    }

    pcardinfo->card_csd.erase_blk_en = (tempbyte & 0x40) >> 6;
    pcardinfo->card_csd.sector_size = (tempbyte & 0x3F) << 1;

    /* CSD byte 11 */
    tempbyte = (uint8_t)(card_opration.sd_csd[2] & SD_MASK_0_7BITS);
    pcardinfo->card_csd.sector_size |= (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.wp_grp_size = (tempbyte & 0x7F);

    /* CSD byte 12 */
    tempbyte = (uint8_t)((card_opration.sd_csd[3] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_csd.wp_grp_enable = (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.r2w_factor = (tempbyte & 0x1C) >> 2;
    pcardinfo->card_csd.write_bl_len = (tempbyte & 0x03) << 2;

    /* CSD byte 13 */
    tempbyte = (uint8_t)((card_opration.sd_csd[3] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_csd.write_bl_len |= (tempbyte & 0xC0) >> 6;
    pcardinfo->card_csd.write_bl_partial = (tempbyte & 0x20) >> 5;

    /* CSD byte 14 */
    tempbyte = (uint8_t)((card_opration.sd_csd[3] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_csd.file_format_grp = (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.copy_flag = (tempbyte & 0x40) >> 6;
    pcardinfo->card_csd.perm_write_protect = (tempbyte & 0x20) >> 5;
    pcardinfo->card_csd.tmp_write_protect = (tempbyte & 0x10) >> 4;
    pcardinfo->card_csd.file_format = (tempbyte & 0x0C) >> 2;

    /* CSD byte 15 */
    tempbyte = (uint8_t)(card_opration.sd_csd[3] & SD_MASK_0_7BITS);
    pcardinfo->card_csd.csd_crc = (tempbyte & 0xFE) >> 1;

    return status;
}

/*!
    \brief      check if the command sent error occurs
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum cmdsent_error_check(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    uint32_t timeout = 100000U;
    /* check command sent flag */
    while ((RESET == sdio_flag_get(SDIO, SDIO_FLAG_CMDSEND)) && (timeout > 0U))
    {
        --timeout;
    }
    /* command response is timeout */
    if (0U == timeout)
    {
        status = SD_CMD_RESP_TIMEOUT;
        return status;
    }
    /* if the command is sent, clear the CMD_FLAGS flags */
    sdio_flag_clear(SDIO, SDIO_MASK_CMD_FLAGS);
    return status;
}

/*!
    \brief      check if error type for R1 response
    \param[in]  resp: content of response
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum r1_error_type_check(uint32_t resp)
{
    sd_error_enum status = SD_ERROR;
    /* check which error occurs */
    if (resp & SD_R1_OUT_OF_RANGE)
    {
        status = SD_OUT_OF_RANGE;
    }
    else if (resp & SD_R1_ADDRESS_ERROR)
    {
        status = SD_ADDRESS_ERROR;
    }
    else if (resp & SD_R1_BLOCK_LEN_ERROR)
    {
        status = SD_BLOCK_LEN_ERROR;
    }
    else if (resp & SD_R1_ERASE_SEQ_ERROR)
    {
        status = SD_ERASE_SEQ_ERROR;
    }
    else if (resp & SD_R1_ERASE_PARAM)
    {
        status = SD_ERASE_PARAM;
    }
    else if (resp & SD_R1_WP_VIOLATION)
    {
        status = SD_WP_VIOLATION;
    }
    else if (resp & SD_R1_LOCK_UNLOCK_FAILED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
    }
    else if (resp & SD_R1_COM_CRC_ERROR)
    {
        status = SD_COM_CRC_ERROR;
    }
    else if (resp & SD_R1_ILLEGAL_COMMAND)
    {
        status = SD_ILLEGAL_COMMAND;
    }
    else if (resp & SD_R1_CARD_ECC_FAILED)
    {
        status = SD_CARD_ECC_FAILED;
    }
    else if (resp & SD_R1_CC_ERROR)
    {
        status = SD_CC_ERROR;
    }
    else if (resp & SD_R1_GENERAL_UNKNOWN_ERROR)
    {
        status = SD_GENERAL_UNKNOWN_ERROR;
    }
    else if (resp & SD_R1_CSD_OVERWRITE)
    {
        status = SD_CSD_OVERWRITE;
    }
    else if (resp & SD_R1_WP_ERASE_SKIP)
    {
        status = SD_WP_ERASE_SKIP;
    }
    else if (resp & SD_R1_CARD_ECC_DISABLED)
    {
        status = SD_CARD_ECC_DISABLED;
    }
    else if (resp & SD_R1_ERASE_RESET)
    {
        status = SD_ERASE_RESET;
    }
    else if (resp & SD_R1_AKE_SEQ_ERROR)
    {
        status = SD_AKE_SEQ_ERROR;
    }
    return status;
}

/*!
    \brief      check if error occurs for R1 response
    \param[in]  cmdindex: the index of command
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum r1_error_check(uint32_t SDIO, uint8_t cmdindex)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0, resp_r1 = 0;

    /* store the content of SDIO_STAT */
    reg_status = SDIO_STAT(SDIO);
    while (!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO);
    }
    /* check whether an error or timeout occurs or command response received */
    if (reg_status & SDIO_FLAG_CCRCERR)
    {
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_CCRCERR);
        return status;
    }
    else if (reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDTMOUT);
        return status;
    }

    /* check whether the last response command index is the desired one */
    if (sdio_command_index_get(SDIO) != cmdindex)
    {
        status = SD_ILLEGAL_COMMAND;
        return status;
    }
    /* clear all the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_CMD_FLAGS);
    /* get the SDIO response register 0 for checking */
    resp_r1 = sdio_response_get(SDIO, SDIO_RESPONSE0);
    if (SD_ALLZERO == (resp_r1 & SD_R1_ERROR_BITS))
    {
        /* no error occurs, return SD_OK */
        status = SD_OK;
        return status;
    }

    /* if some error occurs, return the error type */
    status = r1_error_type_check(resp_r1);
    return status;
}

/*!
    \brief      check if error occurs for R2 response
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum r2_error_check(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0;

    /* store the content of SDIO_STAT */
    reg_status = SDIO_STAT(SDIO);
    while (!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO);
    }
    /* check whether an error or timeout occurs or command response received */
    if (reg_status & SDIO_FLAG_CCRCERR)
    {
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_CCRCERR);
        return status;
    }
    else if (reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDTMOUT);
        return status;
    }
    /* clear all the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_CMD_FLAGS);
    return status;
}

/*!
    \brief      check if error occurs for R3 response
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum r3_error_check(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0;

    /* store the content of SDIO_STAT */
    reg_status = SDIO_STAT(SDIO);
    while (!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO);
    }
    if (reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDTMOUT);
        return status;
    }
    /* clear all the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_CMD_FLAGS);
    return status;
}

/*!
    \brief      check if error occurs for R6 response
    \param[in]  cmdindex: the index of command
    \param[out] prca: a pointer that store the RCA of card
    \retval     sd_error_enum
*/
static sd_error_enum r6_error_check(uint32_t SDIO, uint8_t cmdindex, uint16_t *prca)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0, response = 0;

    /* store the content of SDIO_STAT */
    reg_status = SDIO_STAT(SDIO);
    while (!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO);
    }
    /* check whether an error or timeout occurs or command response received */
    if (reg_status & SDIO_FLAG_CCRCERR)
    {
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_CCRCERR);
        return status;
    }
    else if (reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDTMOUT);
        return status;
    }

    /* check whether the last response command index is the desired one */
    if (sdio_command_index_get(SDIO) != cmdindex)
    {
        status = SD_ILLEGAL_COMMAND;
        return status;
    }
    /* clear all the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_CMD_FLAGS);
    /* get the SDIO response register 0 for checking */
    response = sdio_response_get(SDIO, SDIO_RESPONSE0);

    if (SD_ALLZERO == (response & (SD_R6_COM_CRC_ERROR | SD_R6_ILLEGAL_COMMAND | SD_R6_GENERAL_UNKNOWN_ERROR)))
    {
        *prca = (uint16_t)(response >> 16);
        return status;
    }
    /* if some error occurs, return the error type */
    if (response & SD_R6_COM_CRC_ERROR)
    {
        status = SD_COM_CRC_ERROR;
    }
    else if (response & SD_R6_ILLEGAL_COMMAND)
    {
        status = SD_ILLEGAL_COMMAND;
    }
    else if (response & SD_R6_GENERAL_UNKNOWN_ERROR)
    {
        status = SD_GENERAL_UNKNOWN_ERROR;
    }
    return status;
}

/*!
    \brief      check if error occurs for R7 response
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum r7_error_check(uint32_t SDIO)
{
    sd_error_enum status = SD_ERROR;
    uint32_t reg_status = 0U, timeout = 100000U;

    /* store the content of SDIO_STAT */
    reg_status = SDIO_STAT(SDIO);
    while (!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)) && (timeout > 0))
    {
        reg_status = SDIO_STAT(SDIO);
        --timeout;
    }

    /* check the flags */
    if ((reg_status & SDIO_FLAG_CMDTMOUT) || (0 == timeout))
    {
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDTMOUT);
        return status;
    }
    if (reg_status & SDIO_FLAG_CMDRECV)
    {
        status = SD_OK;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDRECV);
        return status;
    }
    return status;
}

/*!
    \brief      get the state which the card is in
    \param[in]  none
    \param[out] pcardstate: a pointer that store the card state
      \arg        SD_CARDSTATE_IDLE: card is in idle state
      \arg        SD_CARDSTATE_READY: card is in ready state
      \arg        SD_CARDSTATE_IDENTIFICAT: card is in identificat state
      \arg        SD_CARDSTATE_STANDBY: card is in standby state
      \arg        SD_CARDSTATE_TRANSFER: card is in transfer state
      \arg        SD_CARDSTATE_DATA: card is in data state
      \arg        SD_CARDSTATE_RECEIVING: card is in receiving state
      \arg        SD_CARDSTATE_PROGRAMMING: card is in programming state
      \arg        SD_CARDSTATE_DISCONNECT: card is in disconnect state
      \arg        SD_CARDSTATE_LOCKED: card is in locked state
    \retval     sd_error_enum
*/
static sd_error_enum sd_card_state_get(uint32_t SDIO, uint8_t *pcardstate)
{
    sd_error_enum status = SD_OK;
    __IO uint32_t reg_status = 0, response = 0;

    /* send CMD13(SEND_STATUS), addressed card sends its status register */
    sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);

    /* store the content of SDIO_STAT */
    reg_status = SDIO_STAT(SDIO);
    while (!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO);
    }
    /* check whether an error or timeout occurs or command response received */
    if (reg_status & SDIO_FLAG_CCRCERR)
    {
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_CCRCERR);
        return status;
    }
    else if (reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_CMDTMOUT);
        return status;
    }

    /* command response received, store the response command index */
    reg_status = (uint32_t)sdio_command_index_get(SDIO);
    if (reg_status != (uint32_t)SD_CMD_SEND_STATUS)
    {
        status = SD_ILLEGAL_COMMAND;
        return status;
    }
    /* clear all the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_INTC_FLAGS);
    /* get the SDIO response register 0 for checking */
    response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    *pcardstate = (uint8_t)((response >> 9) & 0x0000000F);

    if (SD_ALLZERO == (response & SD_R1_ERROR_BITS))
    {
        /* no error occurs, return SD_OK */
        status = SD_OK;
        return status;
    }

    /* if some error occurs, return the error type */
    status = r1_error_type_check(response);
    return status;
}

/*!
    \brief      stop an ongoing data transfer
    \param[in]  none
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_transfer_stop(uint32_t SDIO)
{
    sd_error_enum status = SD_OK;
    /* send CMD12(STOP_TRANSMISSION) to stop transmission */
    sdio_command_response_config(SDIO, SD_CMD_STOP_TRANSMISSION, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_trans_stop_enable(SDIO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_STOP_TRANSMISSION);
    sdio_trans_stop_disable(SDIO);
    return status;
}

/*!
    \brief      lock or unlock a card
    \param[in]  lockstate: the lock state
      \arg        SD_LOCK: lock the SD card
      \arg        SD_UNLOCK: unlock the SD card
    \param[out] none
    \retval     sd_error_enum
*/
sd_error_enum sd_lock_unlock(uint32_t SDIO, uint8_t lockstate)
{
    sd_error_enum status = SD_OK;
    uint8_t cardstate = 0, tempbyte = 0;
    uint32_t pwd1 = 0, pwd2 = 0, response = 0, timeout = 0;
    uint16_t tempccc = 0;

    /* get the card command classes from CSD */
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_24_31BITS) >> 24);
    tempccc = (uint16_t)((uint16_t)tempbyte << 4);
    tempbyte = (uint8_t)((card_opration.sd_csd[1] & SD_MASK_16_23BITS) >> 16);
    tempccc |= (uint16_t)((uint16_t)(tempbyte & 0xF0) >> 4);

    if (0 == (tempccc & SD_CCC_LOCK_CARD))
    {
        /* don't support the lock command */
        status = SD_FUNCTION_UNSUPPORTED;
        return status;
    }
    /* password pattern */
    pwd1 = (0x01020600 | lockstate);
    pwd2 = 0x03040506;

    /* clear all DSM configuration */
    sdio_data_config(SDIO, 0U, 0U, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_dsm_disable(SDIO);
    sdio_idma_disable(SDIO);

    /* send CMD16(SET_BLOCKLEN) to set the block length */
    sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)8, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
    if (SD_OK != status)
    {
        return status;
    }

    /* send CMD13(SEND_STATUS), addressed card sends its status register */
    sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
    if (SD_OK != status)
    {
        return status;
    }

    response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    timeout = 400000;
    while ((0 == (response & SD_R1_READY_FOR_DATA)) && (timeout > 0))
    {
        /* continue to send CMD13 to polling the state of card until buffer empty or timeout */
        --timeout;
        /* send CMD13(SEND_STATUS), addressed card sends its status registers */
        sdio_command_response_config(SDIO, SD_CMD_SEND_STATUS, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO);
        /* check if some error occurs */
        status = r1_error_check(SDIO, SD_CMD_SEND_STATUS);
        if (SD_OK != status)
        {
            return status;
        }
        response = sdio_response_get(SDIO, SDIO_RESPONSE0);
    }
    if (0 == timeout)
    {
        return SD_ERROR;
    }

    /* send CMD42(LOCK_UNLOCK) to set/reset the password or lock/unlock the card */
    sdio_command_response_config(SDIO, SD_CMD_LOCK_UNLOCK, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_LOCK_UNLOCK);
    if (SD_OK != status)
    {
        return status;
    }

    response = sdio_response_get(SDIO, SDIO_RESPONSE0);

    /* configure the SDIO data transmisson */
    sdio_data_config(SDIO, SD_DATATIMEOUT, (uint32_t)8, SDIO_DATABLOCKSIZE_8BYTES);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_dsm_enable(SDIO);

    /* write password pattern */
    sdio_data_write(SDIO, pwd1);
    sdio_data_write(SDIO, pwd2);

    /* whether some error occurs and return it */
    if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR))
    {
        status = SD_DATA_CRC_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_DTCRCERR);
        return status;
    }
    else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTTMOUT))
    {
        status = SD_DATA_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_DTTMOUT);
        return status;
    }
    else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_TXURE))
    {
        status = SD_TX_UNDERRUN_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_TXURE);
        return status;
    }
    else
    {
        /* if else end */
    }

    /* clear the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_INTC_FLAGS);
    /* get the card state and wait the card is out of programming and receiving state */
    status = sd_card_state_get(SDIO, &cardstate);
    while ((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate)))
    {
        status = sd_card_state_get(SDIO, &cardstate);
    }
    return status;
}

/*!
    \brief      configure the bus width mode
    \param[in]  buswidth: the bus width
      \arg        SD_BUS_WIDTH_1BIT: 1-bit bus width
      \arg        SD_BUS_WIDTH_4BIT: 4-bit bus width
    \param[out] none
    \retval     sd_error_enum
*/
static sd_error_enum sd_bus_width_config(uint32_t SDIO, uint32_t buswidth)
{
    sd_error_enum status = SD_OK;
    /* check whether the card is locked */
    if (sdio_response_get(SDIO, SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }
    /* get the SCR register */
    status = sd_scr_get(SDIO, card_opration.sd_rca, card_opration.sd_scr);
    if (SD_OK != status)
    {
        return status;
    }

    if (SD_BUS_WIDTH_1BIT == buswidth)
    {
        if (SD_ALLZERO != (card_opration.sd_scr[1] & buswidth))
        {
            /* send CMD55(APP_CMD) to indicate next command is application specific command */
            sdio_command_response_config(SDIO, SD_CMD_APP_CMD, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_APP_CMD);
            if (SD_OK != status)
            {
                return status;
            }

            /* send ACMD6(SET_BUS_WIDTH) to define the data bus width */
            sdio_command_response_config(SDIO, SD_APPCMD_SET_BUS_WIDTH, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_APPCMD_SET_BUS_WIDTH);
            if (SD_OK != status)
            {
                return status;
            }
        }
        else
        {
            status = SD_OPERATION_IMPROPER;
        }
        return status;
    }
    else if (SD_BUS_WIDTH_4BIT == buswidth)
    {
        if (SD_ALLZERO != (card_opration.sd_scr[1] & buswidth))
        {
            /* send CMD55(APP_CMD) to indicate next command is application specific command */
            sdio_command_response_config(SDIO, SD_CMD_APP_CMD, (uint32_t)card_opration.sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_CMD_APP_CMD);
            if (SD_OK != status)
            {
                return status;
            }

            /* send ACMD6(SET_BUS_WIDTH) to define the data bus width */
            sdio_command_response_config(SDIO, SD_APPCMD_SET_BUS_WIDTH, (uint32_t)0x2, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
            sdio_csm_enable(SDIO);
            /* check if some error occurs */
            status = r1_error_check(SDIO, SD_APPCMD_SET_BUS_WIDTH);
            if (SD_OK != status)
            {
                return status;
            }
        }
        else
        {
            status = SD_OPERATION_IMPROPER;
        }
        return status;
    }
    else
    {
        status = SD_PARAMETER_INVALID;
        return status;
    }
}

/*!
    \brief      get the SCR of corresponding card
    \param[in]  rca: RCA of a card
    \param[out] pscr: a pointer that store the SCR content
    \retval     sd_error_enum
*/
static sd_error_enum sd_scr_get(uint32_t SDIO, uint16_t rca, uint32_t *pscr)
{
    sd_error_enum status = SD_OK;
    uint32_t temp_scr[2] = { 0, 0 }, idx_scr = 0;
    /* send CMD16(SET_BLOCKLEN) to set block length */
    sdio_command_response_config(SDIO, SD_CMD_SET_BLOCKLEN, (uint32_t)8, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_SET_BLOCKLEN);
    if (SD_OK != status)
    {
        return status;
    }

    /* send CMD55(APP_CMD) to indicate next command is application specific command */
    sdio_command_response_config(SDIO, SD_CMD_APP_CMD, (uint32_t)rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_CMD_APP_CMD);
    if (SD_OK != status)
    {
        return status;
    }

    /* configure SDIO data */
    sdio_data_config(SDIO, SD_DATATIMEOUT, (uint32_t)8, SDIO_DATABLOCKSIZE_8BYTES);
    sdio_data_transfer_config(SDIO, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
    sdio_dsm_enable(SDIO);

    /* send ACMD51(SEND_SCR) to read the SD configuration register */
    sdio_command_response_config(SDIO, SD_APPCMD_SEND_SCR, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO);
    /* check if some error occurs */
    status = r1_error_check(SDIO, SD_APPCMD_SEND_SCR);
    if (SD_OK != status)
    {
        return status;
    }

    /* store the received SCR */
    while (!sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))
    {
        if ((SET != sdio_flag_get(SDIO, SDIO_FLAG_RFE)) && (SET == sdio_flag_get(SDIO, SDIO_FLAG_DATSTA)))
        {
            *(temp_scr + idx_scr) = sdio_data_read(SDIO);
            ++idx_scr;
        }
    }

    /* check whether some error occurs */
    if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTCRCERR))
    {
        status = SD_DATA_CRC_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_DTCRCERR);
        return status;
    }
    else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_DTTMOUT))
    {
        status = SD_DATA_TIMEOUT;
        sdio_flag_clear(SDIO, SDIO_FLAG_DTTMOUT);
        return status;
    }
    else if (RESET != sdio_flag_get(SDIO, SDIO_FLAG_RXORE))
    {
        status = SD_RX_OVERRUN_ERROR;
        sdio_flag_clear(SDIO, SDIO_FLAG_RXORE);
        return status;
    }
    else
    {
        /* if else end */
    }

    /* clear all the SDIO_INTC flags */
    sdio_flag_clear(SDIO, SDIO_MASK_INTC_FLAGS);
    /* readjust the temp SCR value */
    *(pscr) = ((temp_scr[1] & SD_MASK_0_7BITS) << 24) | ((temp_scr[1] & SD_MASK_8_15BITS) << 8) |
              ((temp_scr[1] & SD_MASK_16_23BITS) >> 8) | ((temp_scr[1] & SD_MASK_24_31BITS) >> 24);
    *(pscr + 1) = ((temp_scr[0] & SD_MASK_0_7BITS) << 24) | ((temp_scr[0] & SD_MASK_8_15BITS) << 8) |
                  ((temp_scr[0] & SD_MASK_16_23BITS) >> 8) | ((temp_scr[0] & SD_MASK_24_31BITS) >> 24);
    return status;
}

/*!
    \brief      get the data block size
    \param[in]  bytesnumber: the number of bytes
    \param[out] none
    \retval     data block size
      \arg        SDIO_DATABLOCKSIZE_1BYTE: block size = 1 byte
      \arg        SDIO_DATABLOCKSIZE_2BYTES: block size = 2 bytes
      \arg        SDIO_DATABLOCKSIZE_4BYTES: block size = 4 bytes
      \arg        SDIO_DATABLOCKSIZE_8BYTES: block size = 8 bytes
      \arg        SDIO_DATABLOCKSIZE_16BYTES: block size = 16 bytes
      \arg        SDIO_DATABLOCKSIZE_32BYTES: block size = 32 bytes
      \arg        SDIO_DATABLOCKSIZE_64BYTES: block size = 64 bytes
      \arg        SDIO_DATABLOCKSIZE_128BYTES: block size = 128 bytes
      \arg        SDIO_DATABLOCKSIZE_256BYTES: block size = 256 bytes
      \arg        SDIO_DATABLOCKSIZE_512BYTES: block size = 512 bytes
      \arg        SDIO_DATABLOCKSIZE_1024BYTES: block size = 1024 bytes
      \arg        SDIO_DATABLOCKSIZE_2048BYTES: block size = 2048 bytes
      \arg        SDIO_DATABLOCKSIZE_4096BYTES: block size = 4096 bytes
      \arg        SDIO_DATABLOCKSIZE_8192BYTES: block size = 8192 bytes
      \arg        SDIO_DATABLOCKSIZE_16384BYTES: block size = 16384 bytes
*/
static uint32_t sd_datablocksize_get(uint16_t bytesnumber)
{
    uint8_t exp_val = 0;
    /* calculate the exponent of 2 */
    while (1 != bytesnumber)
    {
        bytesnumber >>= 1;
        ++exp_val;
    }
    return DATACTL_BLKSZ(exp_val);
}

/*!
    \brief      configure the GPIO of SDIO interface
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void gpio_config(void)
{
    /* configure the SDIO_DAT0(PC8), SDIO_DAT1(PC9), SDIO_DAT2(PC10), SDIO_DAT3(PC11), SDIO_CLK(PC12) and SDIO_CMD(PD2) */
    gpio_af_set(SDIO_CLK_PORT, GPIO_AF_12, SDIO_CLK_PIN);
    gpio_af_set(SDIO_CMD_PORT, GPIO_AF_12, SDIO_CMD_PIN);
    gpio_af_set(SDIO_D0_PORT, GPIO_AF_12, SDIO_D0_PIN);
    gpio_af_set(SDIO_D1_PORT, GPIO_AF_12, SDIO_D1_PIN);
    gpio_af_set(SDIO_D2_PORT, GPIO_AF_12, SDIO_D2_PIN);
    gpio_af_set(SDIO_D3_PORT, GPIO_AF_12, SDIO_D3_PIN);

    gpio_mode_set(SDIO_CLK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SDIO_CLK_PIN);
    gpio_output_options_set(SDIO_CLK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SDIO_CLK_PIN);

    gpio_mode_set(SDIO_CMD_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SDIO_CMD_PIN);
    gpio_output_options_set(SDIO_CMD_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SDIO_CMD_PIN);

    gpio_mode_set(SDIO_D0_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SDIO_D0_PIN);
    gpio_output_options_set(SDIO_D0_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SDIO_D0_PIN);

    gpio_mode_set(SDIO_D1_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SDIO_D1_PIN);
    gpio_output_options_set(SDIO_D1_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SDIO_D1_PIN);

    gpio_mode_set(SDIO_D2_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SDIO_D2_PIN);
    gpio_output_options_set(SDIO_D2_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SDIO_D2_PIN);

    gpio_mode_set(SDIO_D3_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SDIO_D3_PIN);
    gpio_output_options_set(SDIO_D3_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SDIO_D3_PIN);
}

/*!
    \brief      configure the RCU of SDIO and DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rcu_config(void)
{
    /* SDIO clock */
    /* configure the pll1 input and output clock range */
    rcu_pll_input_output_clock_range_config(IDX_PLL1, RCU_PLL1RNG_4M_8M, RCU_PLL1VCO_192M_836M);
    /* configure the PLL1 clock: CK_PLL1P/CK_PLL1Q/CK_PLL1R = HXTAL_VALUE / 5 * 40 / 1 */
    rcu_pll1_config(5, 40, 1, 1, 1);
    /* SDIO clock 400M */
    // rcu_pll1_config(5, 160, 1, 2, 2);
    /* enable PLL1R clock output */
    rcu_pll_clock_output_enable(RCU_PLL1R);
    /* enable PLL1 clock */
    rcu_osci_on(RCU_PLL1_CK);

    if (ERROR == rcu_osci_stab_wait(RCU_PLL1_CK))
    {
        while (1)
        {
        }
    }

    rcu_periph_clock_enable(SDIO_GPIO_CLK);
    rcu_periph_clock_enable(SDIO_GPIO_CMD);
    rcu_periph_clock_enable(SDIO_GPIO_D0);
    rcu_periph_clock_enable(SDIO_GPIO_D1);
    rcu_periph_clock_enable(SDIO_GPIO_D2);
    rcu_periph_clock_enable(SDIO_GPIO_D3);

    rcu_sdio_clock_config(IDX_SDIO0, RCU_SDIO0SRC_PLL1R);
    rcu_periph_clock_enable(SDIO_PERI_CLOCK);
}

/*!
    \brief      configure the IDMA
    \param[in]  srcbuf: a pointer point to a buffer which will be transferred
    \param[in]  bufsize: the size of buffer(not used in flow controller is peripheral)
    \param[out] none
    \retval     none
*/
static void dma_config(uint32_t SDIO, uint32_t *srcbuf, uint32_t bufsize)
{
    sdio_idma_set(SDIO, SDIO_IDMA_SINGLE_BUFFER, bufsize);
    sdio_idma_buffer0_address_set(SDIO, (uint32_t)srcbuf);
}

#endif /* RT_USING_SDIO */
