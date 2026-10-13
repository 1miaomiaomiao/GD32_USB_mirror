/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#include <rtthread.h>
#include <rthw.h>

#if defined(RT_USING_SAL) && defined(RT_USING_NETDEV) && defined(RT_USING_LWIP)

#include <netif/ethernetif.h>
#include <lwip/dhcp.h>
#include <netdev.h>

#include "drv_enet.h"
#include "enet_config.h"

#define DBG_TAG "drv.eth"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define ETHERNET_MAC0 ((struct rt_synopsys_eth *)(0x40020000U + 0x00008000U))

//#define EMAC_DEBUG
//#define EMAC_RX_DUMP
//#define EMAC_TX_DUMP

#ifdef EMAC_DEBUG
#define EMAC_TRACE rt_kprintf
#else
#define EMAC_TRACE(...)
#endif

#ifdef __ARMCC_VERSION
static struct gd32_emac gd32_emac_device0 __attribute__((section(".ARM.__at_0x30000000")));
#elif defined(__GNUC__)
static struct gd32_emac gd32_emac_device0 __attribute__((section(".ent_desc_tab_section")));
#endif

/*!
    \brief      configure the MPU
    \param[in]  none
    \param[out] none
    \retval     none
*/
int enet_mpu_config(void)
{
    mpu_region_init_struct mpu_init_struct;
    mpu_region_struct_para_init(&mpu_init_struct);

    /* disable the MPU */
    ARM_MPU_Disable();
    ARM_MPU_SetRegion(0, 0);

    /* configure the MPU attributes for the entire 4GB area, Reserved, no access */
    /* This configuration is highly recommended to prevent Speculative Prefetching of external memory,
       which may cause CPU read locks and even system errors */
    mpu_init_struct.region_base_address = 0x0;
    mpu_init_struct.region_size = MPU_REGION_SIZE_4GB;
    mpu_init_struct.access_permission = MPU_AP_NO_ACCESS;
    mpu_init_struct.access_bufferable = MPU_ACCESS_NON_BUFFERABLE;
    mpu_init_struct.access_cacheable = MPU_ACCESS_NON_CACHEABLE;
    mpu_init_struct.access_shareable = MPU_ACCESS_SHAREABLE;
    mpu_init_struct.region_number = MPU_REGION_NUMBER0;
    mpu_init_struct.subregion_disable = 0x87;
    mpu_init_struct.instruction_exec = MPU_INSTRUCTION_EXEC_NOT_PERMIT;
    mpu_init_struct.tex_type = MPU_TEX_TYPE0;
    mpu_region_config(&mpu_init_struct);
    mpu_region_enable();

    /* Configure the DMA descriptors and Rx/Tx buffer*/
    mpu_init_struct.region_base_address = 0x30000000;
    mpu_init_struct.region_size = MPU_REGION_SIZE_32KB;
    mpu_init_struct.access_permission = MPU_AP_FULL_ACCESS;
    mpu_init_struct.access_bufferable = MPU_ACCESS_BUFFERABLE;
    mpu_init_struct.access_cacheable = MPU_ACCESS_NON_CACHEABLE;
    mpu_init_struct.access_shareable = MPU_ACCESS_NON_SHAREABLE;

    mpu_init_struct.region_number = MPU_REGION_NUMBER1;
    mpu_init_struct.subregion_disable = MPU_SUBREGION_ENABLE;
    mpu_init_struct.instruction_exec = MPU_INSTRUCTION_EXEC_PERMIT;
    mpu_init_struct.tex_type = MPU_TEX_TYPE0;
    mpu_region_config(&mpu_init_struct);
    mpu_region_enable();

    /* enable the MPU */
    ARM_MPU_Enable(MPU_MODE_PRIV_DEFAULT);

    return 0;
}
INIT_BOARD_EXPORT(enet_mpu_config);

/**
 * @brief Configure a single ENET pin
 * @param pin_cfg: pointer to pin configuration
 * @retval RT_EOK on success, error code on failure
 */
static rt_err_t enet_pin_configure(const struct enet_pin_config *pin_cfg)
{
    uint32_t port, pin, af;
    rcu_periph_enum clk;

    /* Get pin configuration */
    if (get_pin_config(pin_cfg->pin_name, &port, &pin, &clk) == -RT_ERROR)
    {
        LOG_E("Invalid pin name: %s", pin_cfg->pin_name);
        return -RT_EINVAL;
    }

    /* Get alternate function */
    if (pin_alternate_config(pin_cfg->alternate, &af) == -RT_ERROR)
    {
        LOG_E("Invalid alternate function: %s", pin_cfg->alternate);
        return -RT_EINVAL;
    }

    /* Enable GPIO clock */
    rcu_periph_clock_enable(clk);

    /* Configure GPIO with common parameters */
    gpio_mode_set(port, GPIO_MODE_AF, enet0_cfg.pupd, pin);
    gpio_output_options_set(port, enet0_cfg.otype, enet0_cfg.speed, pin);
    gpio_af_set(port, af, pin);

    LOG_D("Configured pin %s with %s", pin_cfg->pin_name, pin_cfg->alternate);

    return RT_EOK;
}

/**
 * @brief Configure clock output for PHY
 */
static void enet_clock_config(void)
{
    uint32_t port, pin, af;
    rcu_periph_enum clk;

    /* Enable SYSCFG clock */
    rcu_periph_clock_enable(RCU_SYSCFG);

    /* Configure clock output pin */
    if (get_pin_config(enet_clock_pin.pin_name, &port, &pin, &clk) != -RT_ERROR &&
        pin_alternate_config(enet_clock_pin.alternate, &af) != -RT_ERROR)
    {
        rcu_periph_clock_enable(clk);
        gpio_af_set(port, af, pin);
        gpio_mode_set(port, GPIO_MODE_AF, enet0_cfg.pupd, pin);
        gpio_output_options_set(port, enet0_cfg.otype, enet0_cfg.speed, pin);

        LOG_D("Configured clock output pin %s", enet_clock_pin.pin_name);
    }

    /* Choose DIV12 to get 50MHz from 200MHz on CKOUT0 pin (PA8) to clock the PHY */
    rcu_ckout0_config(RCU_CKOUT0SRC_PLL0P, RCU_CKOUT0_DIV12);

    /* Configure PHY interface */
    syscfg_enet_phy_interface_config(enet0_cfg.enet_periph, enet0_cfg.phy_interface);

    LOG_I("Clock output configured for PHY");
}

/**
 * @brief Initialize ENET GPIO configuration
 * @param emac: pointer to ENET device structure
 * @retval RT_EOK on success, error code on failure
 */
rt_err_t gd32_enet_gpio_init(struct gd32_emac *emac)
{
    rt_uint32_t i;
    rt_err_t result;

    /* Configure all ENET pins */
    for (i = 0; i < enet0_cfg.pin_count; i++)
    {
        result = enet_pin_configure(&enet0_cfg.pins[i]);
        if (result != RT_EOK)
        {
            LOG_E("Failed to configure pin %s", enet0_cfg.pins[i].pin_name);
            return result;
        }
    }

    /* Store pin configuration in device structure */
    emac->pins = enet0_cfg.pins;
    emac->pin_count = enet0_cfg.pin_count;

    /* Enable ENET peripheral clock */
    rcu_periph_clock_enable(enet0_cfg.enet_clk);
    rcu_periph_clock_enable(enet0_cfg.enet_tx_clk);
    rcu_periph_clock_enable(enet0_cfg.enet_rx_clk);

    LOG_I("ENET GPIO initialization completed successfully");

    return RT_EOK;
}

/**
 * @brief Get ENET configuration
 * @return pointer to ENET configuration structure
 */
const struct enet_config *gd32_get_enet_config(void)
{
    return &enet0_cfg;
}

/**
  * Initializes the DMA Tx descriptors in chain mode.
  */
static void EMAC_DMA_tx_desc_init(EMAC_DMADESCTypeDef *DMATxDescTab, uint8_t *TxBuff, uint32_t TxBuffCount)
{
    uint32_t i = 0;
    EMAC_DMADESCTypeDef *DMATxDesc;

    /* Fill each DMATxDesc descriptor with the right values */
    for (i = 0; i < TxBuffCount; i++)
    {
        /* Get the pointer on the ith member of the Tx Desc list */
        DMATxDesc = DMATxDescTab + i;
        /* Set Second Address Chained bit */
        DMATxDesc->Status = EMAC_DMATxDesc_TCH;

        /* Set Buffer1 address pointer */
        DMATxDesc->Buffer1Addr = (uint32_t)(&TxBuff[i * EMAC_MAX_PACKET_SIZE]);

        /* Initialize the next descriptor with the Next Descriptor Polling Enable */
        if (i < (TxBuffCount - 1))
        {
            /* Set next descriptor address register with next descriptor base address */
            DMATxDesc->Buffer2NextDescAddr = (uint32_t)(DMATxDescTab + i + 1);
        }
        else
        {
            /* For last descriptor, set next descriptor address register equal to the first descriptor base address */
            DMATxDesc->Buffer2NextDescAddr = (uint32_t)DMATxDescTab;
        }

#ifdef RT_LWIP_USING_HW_CHECKSUM
        enet_transmit_checksum_config(DMATxDesc, ENET_CHECKSUM_TCPUDPICMP_FULL);
#endif
    }
}

/**
  * Initializes the DMA Rx descriptors in chain mode.
  */
static void EMAC_DMA_rx_desc_init(EMAC_DMADESCTypeDef *DMARxDescTab, uint8_t *RxBuff, uint32_t RxBuffCount)
{
    uint32_t i = 0;
    EMAC_DMADESCTypeDef *DMARxDesc;

    /* Fill each DMARxDesc descriptor with the right values */
    for (i = 0; i < RxBuffCount; i++)
    {
        /* Get the pointer on the ith member of the Rx Desc list */
        DMARxDesc = DMARxDescTab + i;
        /* Set Own bit of the Rx descriptor Status */
        DMARxDesc->Status = EMAC_DMARxDesc_OWN;

        /* Set Buffer1 size and Second Address Chained bit */
        DMARxDesc->ControlBufferSize = EMAC_DMARxDesc_RCH | (uint32_t)EMAC_MAX_PACKET_SIZE;
        /* Set Buffer1 address pointer */
        DMARxDesc->Buffer1Addr = (uint32_t)(&RxBuff[i * EMAC_MAX_PACKET_SIZE]);

        /* Initialize the next descriptor with the Next Descriptor Polling Enable */
        if (i < (RxBuffCount - 1))
        {
            /* Set next descriptor address register with next descriptor base address */
            DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab + i + 1);
        }
        else
        {
            /* For last descriptor, set next descriptor address register equal to the first descriptor base address */
            DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab);
        }
    }
}

static rt_err_t gd32_emac_init(rt_device_t dev)
{
    struct gd32_emac *gd32_emac_device;
    struct rt_synopsys_eth *ETHERNET_MAC;

    gd32_emac_device = (struct gd32_emac *)dev;
    ETHERNET_MAC = gd32_emac_device->ETHERNET_MAC;

    /* Configure ETHERNET */
    EMAC_init(ETHERNET_MAC, SystemCoreClock);

    /* mask all GMAC MMC Interrupt.*/
    ETHERNET_MAC->mmc_cntl = (1 << 3) | (1 << 0); /* MMC Counter Freeze and reset. */
    ETHERNET_MAC->mmc_intr_mask_rx = 0xFFFFFFFF;
    ETHERNET_MAC->mmc_intr_mask_tx = 0xFFFFFFFF;
    ETHERNET_MAC->mmc_ipc_intr_mask_rx = 0xFFFFFFFF;

    ETHERNET_MAC->mmc_cntl &= ~((1 << 3) | (1 << 0)); /* MMC Counter enable. */


    /* Enable DMA Receive interrupt (need to enable in this case Normal interrupt) */
    EMAC_INT_config(ETHERNET_MAC, EMAC_DMA_INT_NIS | EMAC_DMA_INT_R | EMAC_DMA_INT_T, ENABLE);

    /* Initialize Tx Descriptors list: Chain Mode */
    EMAC_DMA_tx_desc_init(gd32_emac_device->DMATxDscrTab, &gd32_emac_device->Tx_Buff[0][0], EMAC_TXBUFNB);
    gd32_emac_device->DMATxDescToSet = gd32_emac_device->DMATxDscrTab;
    /* Set Transmit Descriptor List Address Register */
    ETHERNET_MAC->TDLAR = (uint32_t)gd32_emac_device->DMATxDescToSet;

    /* Initialize Rx Descriptors list: Chain Mode  */
    EMAC_DMA_rx_desc_init(gd32_emac_device->DMARxDscrTab, &gd32_emac_device->Rx_Buff[0][0], EMAC_RXBUFNB);
    gd32_emac_device->DMARxDescToGet = gd32_emac_device->DMARxDscrTab;
    /* Set Receive Descriptor List Address Register */
    ETHERNET_MAC->RDLAR = (uint32_t)gd32_emac_device->DMARxDescToGet;

    /* MAC address configuration */
    EMAC_MAC_Addr_config(ETHERNET_MAC, EMAC_MAC_Address0, (uint8_t *)&gd32_emac_device->dev_addr[0]);

    NVIC_EnableIRQ(gd32_emac_device->ETHER_MAC_IRQ);

    /* Enable MAC and DMA transmission and reception */
    EMAC_start(ETHERNET_MAC);

    return RT_EOK;
}

static rt_err_t gd32_emac_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t gd32_emac_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_ssize_t gd32_emac_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    return -RT_ENOSYS;
}

static rt_ssize_t gd32_emac_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    return -RT_ENOSYS;
}

static rt_err_t gd32_emac_control(rt_device_t dev, int cmd, void *args)
{
    struct gd32_emac *gd32_emac_device = (struct gd32_emac *)dev;

    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* get mac address */
        if (args)
            rt_memcpy(args, &gd32_emac_device->dev_addr[0], MAX_ADDR_LEN);
        else
            return -RT_ERROR;
        break;

    default:
        break;
    }

    return RT_EOK;
}

static void EMAC_IRQHandler(struct gd32_emac *gd32_emac_device)
{
    rt_uint32_t status, ier;
    struct rt_synopsys_eth *ETHERNET_MAC;

    ETHERNET_MAC = gd32_emac_device->ETHERNET_MAC;

    /* get DMA IT status */
    status = ETHERNET_MAC->SR;
    ier = ETHERNET_MAC->IER;

    /* GMAC MMC Interrupt. */
    if (status & EMAC_DMA_INT_GMI)
    {
        volatile rt_uint32_t dummy;
        volatile rt_uint32_t *reg;

        EMAC_TRACE("EMAC_DMA_INT_GMI\r\n");

        /* read clear all MMC interrupt. */
        reg = &ETHERNET_MAC->mmc_cntl;
        while ((uint32_t)reg < (uint32_t)&ETHERNET_MAC->rxicmp_err_octets)
        {
            dummy = *reg++;
        }
    }

    /* Normal interrupt summary. */
    if (status & EMAC_DMA_INT_NIS)
    {
        rt_uint32_t nis_clear = EMAC_DMA_INT_NIS;

        /* [0]:Transmit Interrupt. */
        if ((status & ier) & EMAC_DMA_INT_T) /* packet transmission */
        {
            rt_sem_release(&gd32_emac_device->tx_buf_free);

            nis_clear |= EMAC_DMA_INT_T;
        }

        /* [2]:Transmit Buffer Unavailable. */

        /* [6]:Receive Interrupt. */
        if ((status & ier) & EMAC_DMA_INT_R) /* packet reception */
        {
            /* a frame has been received */
            eth_device_ready(&(gd32_emac_device->parent));

            nis_clear |= EMAC_DMA_INT_R;
        }

        /* [14]:Early Receive Interrupt. */

        EMAC_clear_pending(ETHERNET_MAC, nis_clear);
    }

    /* Abnormal interrupt summary. */
    if (status & EMAC_DMA_INT_AIS)
    {
        rt_uint32_t ais_clear = EMAC_DMA_INT_AIS;

        /* [1]:Transmit Process Stopped. */
        /* [3]:Transmit Jabber Timeout. */
        /* [4]: Receive FIFO Overflow. */
        /* [5]: Transmit Underflow. */
        /* [7]: Receive Buffer Unavailable. */
        /* [8]: Receive Process Stopped. */
        /* [9]: Receive Watchdog Timeout. */
        /* [10]: Early Transmit Interrupt. */
        /* [13]: Fatal Bus Error. */

        EMAC_clear_pending(ETHERNET_MAC, ais_clear);
    }
}

void ENET0_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    EMAC_IRQHandler(&gd32_emac_device0);

    /* leave interrupt */
    rt_interrupt_leave();
}


/* EtherNet Device Interface */
rt_err_t gd32_emac_tx(rt_device_t dev, struct pbuf *p)
{
    struct pbuf *q;
    char *to;
    struct gd32_emac *gd32_emac_device;
    struct rt_synopsys_eth *ETHERNET_MAC;

    gd32_emac_device = (struct gd32_emac *)dev;
    ETHERNET_MAC = gd32_emac_device->ETHERNET_MAC;

    /* get free tx buffer */
    {
        rt_err_t result;
        result = rt_sem_take(&gd32_emac_device->tx_buf_free, RT_TICK_PER_SECOND / 10);
        if (result != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    to = (char *)gd32_emac_device->DMATxDescToSet->Buffer1Addr;

    for (q = p; q != NULL; q = q->next)
    {
        /* Copy the frame to be sent into memory pointed by the current ETHERNET DMA Tx descriptor */
        rt_memcpy(to, q->payload, q->len);
        to += q->len;
    }

#ifdef EMAC_TX_DUMP
    {
        rt_uint32_t i;
        rt_uint8_t *ptr = (rt_uint8_t *)(gd32_emac_device->DMATxDescToSet->Buffer1Addr);

        EMAC_TRACE("\r\n%c%c tx_dump:", gd32_emac_device->parent.netif->name[0], gd32_emac_device->parent.netif->name[1]);
        for (i = 0; i < p->tot_len; i++)
        {
            if ((i % 8) == 0)
            {
                EMAC_TRACE("  ");
            }
            if ((i % 16) == 0)
            {
                EMAC_TRACE("\r\n");
            }
            EMAC_TRACE("%02x ", *ptr);
            ptr++;
        }
        EMAC_TRACE("\r\ndump done!\r\n");
    }
#endif

    /* Setting the Frame Length: bits[12:0] */
    gd32_emac_device->DMATxDescToSet->ControlBufferSize = (p->tot_len & EMAC_DMATxDesc_TBS1);
    /* Setting the last segment and first segment bits (in this case a frame is transmitted in one descriptor) */
    gd32_emac_device->DMATxDescToSet->Status |= EMAC_DMATxDesc_LS | EMAC_DMATxDesc_FS;
    /* Enable TX Completion Interrupt */
    gd32_emac_device->DMATxDescToSet->Status |= EMAC_DMATxDesc_IC;
#ifdef CHECKSUM_BY_HARDWARE
    gd32_emac_device->DMATxDescToSet->Status |= EMAC_DMATxDesc_ChecksumTCPUDPICMPFull;
    /* clean ICMP checksum */
    {
        struct eth_hdr *ethhdr = (struct eth_hdr *)(gd32_emac_device->DMATxDescToSet->Buffer1Addr);
        /* is IP ? */
        if (ethhdr->type == htons(ETHTYPE_IP))
        {
            struct ip_hdr *iphdr = (struct ip_hdr *)(gd32_emac_device->DMATxDescToSet->Buffer1Addr + SIZEOF_ETH_HDR);
            /* is ICMP ? */
            if (IPH_PROTO(iphdr) == IP_PROTO_ICMP)
            {
                struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(gd32_emac_device->DMATxDescToSet->Buffer1Addr + SIZEOF_ETH_HDR + sizeof(struct ip_hdr));
                iecho->chksum = 0;
            }
        }
    }
#endif
    /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
    gd32_emac_device->DMATxDescToSet->Status |= EMAC_DMATxDesc_OWN;
    /* When Tx Buffer unavailable flag is set: clear it and resume transmission */
    if ((ETHERNET_MAC->SR & EMAC_DMASR_TBUS) != (uint32_t)RESET)
    {
        /* Clear TBUS ETHERNET DMA flag */
        ETHERNET_MAC->SR = EMAC_DMASR_TBUS;
        /* Transmit Poll Demand to resume DMA transmission*/
        ETHERNET_MAC->TPDR = 0;
    }

    /* Update the ETHERNET DMA global Tx descriptor with next Tx decriptor */
    /* Chained Mode */
    /* Selects the next DMA Tx descriptor list for next buffer to send */
    gd32_emac_device->DMATxDescToSet = (EMAC_DMADESCTypeDef *)(gd32_emac_device->DMATxDescToSet->Buffer2NextDescAddr);

    /* Return SUCCESS */
    return RT_EOK;
}

/* reception a Ethernet packet. */
struct pbuf *gd32_emac_rx(rt_device_t dev)
{
    struct pbuf *p;
    rt_uint32_t framelength = 0;
    struct gd32_emac *gd32_emac_device;
    struct rt_synopsys_eth *ETHERNET_MAC;

    gd32_emac_device = (struct gd32_emac *)dev;
    ETHERNET_MAC = gd32_emac_device->ETHERNET_MAC;

    /* init p pointer */
    p = RT_NULL;

    /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
    if (((gd32_emac_device->DMARxDescToGet->Status & EMAC_DMARxDesc_OWN) != (uint32_t)RESET))
    {
        return p;
    }

    if (((gd32_emac_device->DMARxDescToGet->Status & EMAC_DMARxDesc_ES) == (uint32_t)RESET) &&
        ((gd32_emac_device->DMARxDescToGet->Status & EMAC_DMARxDesc_LS) != (uint32_t)RESET) &&
        ((gd32_emac_device->DMARxDescToGet->Status & EMAC_DMARxDesc_FS) != (uint32_t)RESET))
    {
        /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
        framelength = ((gd32_emac_device->DMARxDescToGet->Status & EMAC_DMARxDesc_FL) >> EMAC_DMARXDESC_FRAME_LENGTHSHIFT) - 4;

        /* allocate buffer */
        p = pbuf_alloc(PBUF_LINK, framelength, PBUF_RAM);

        if (p != RT_NULL)
        {
            const char *from;
            struct pbuf *q;

            from = (const char *)gd32_emac_device->DMARxDescToGet->Buffer1Addr;

            for (q = p; q != RT_NULL; q = q->next)
            {
                /* Copy the received frame into buffer from memory pointed by the current ETHERNET DMA Rx descriptor */
                rt_memcpy(q->payload, from, q->len);
                from += q->len;
            }
#ifdef EMAC_RX_DUMP
            {
                rt_uint32_t i;
                rt_uint8_t *ptr = (rt_uint8_t *)(gd32_emac_device->DMARxDescToGet->Buffer1Addr);

                EMAC_TRACE("\r\n%c%c rx_dump:", gd32_emac_device->parent.netif->name[0], gd32_emac_device->parent.netif->name[1]);
                for (i = 0; i < p->tot_len; i++)
                {
                    if ((i % 8) == 0)
                    {
                        EMAC_TRACE("  ");
                    }
                    if ((i % 16) == 0)
                    {
                        EMAC_TRACE("\r\n");
                    }
                    EMAC_TRACE("%02x ", *ptr);
                    ptr++;
                }
                EMAC_TRACE("\r\ndump done!\r\n");
            }
#endif
        }
    }

    /* Set Own bit of the Rx descriptor Status: gives the buffer back to ETHERNET DMA */
    gd32_emac_device->DMARxDescToGet->Status = EMAC_DMARxDesc_OWN;

    /* When Rx Buffer unavailable flag is set: clear it and resume reception */
    if ((ETHERNET_MAC->SR & EMAC_DMASR_RBUS) != (uint32_t)RESET)
    {
        /* Clear RBUS ETHERNET DMA flag */
        ETHERNET_MAC->SR = EMAC_DMASR_RBUS;
        /* Resume DMA reception */
        ETHERNET_MAC->RPDR = 0;
    }

    /* Update the ETHERNET DMA global Rx descriptor with next Rx decriptor */
    /* Chained Mode */
    if ((gd32_emac_device->DMARxDescToGet->ControlBufferSize & EMAC_DMARxDesc_RCH) != (uint32_t)RESET)
    {
        /* Selects the next DMA Rx descriptor list for next buffer to read */
        gd32_emac_device->DMARxDescToGet = (EMAC_DMADESCTypeDef *)(gd32_emac_device->DMARxDescToGet->Buffer2NextDescAddr);
    }
    else /* Ring Mode */
    {
        if ((gd32_emac_device->DMARxDescToGet->ControlBufferSize & EMAC_DMARxDesc_RER) != (uint32_t)RESET)
        {
            /* Selects the first DMA Rx descriptor for next buffer to read: last Rx descriptor was used */
            gd32_emac_device->DMARxDescToGet = (EMAC_DMADESCTypeDef *)(ETHERNET_MAC->RDLAR);
        }
        else
        {
            /* Selects the next DMA Rx descriptor list for next buffer to read */
            gd32_emac_device->DMARxDescToGet = (EMAC_DMADESCTypeDef *)((uint32_t)gd32_emac_device->DMARxDescToGet + 0x10 + ((ETHERNET_MAC->BMR & EMAC_DMABMR_DSL) >> 2));
        }
    }

    return p;
}

/*!
    \brief      configures the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void nvic_configuration(void)
{
    nvic_irq_enable(ENET0_IRQn, 1U, 0U);
}

static void netdev_callback(struct netdev *netdev, enum netdev_cb_type type)
{
    const char *netdev_string_info[] = {
        "ADDR_IP",                 /* 0. IP address */
        "ADDR_NETMASK",            /* 1. subnet mask */
        "ADDR_GATEWAY",            /* 2. netmask */
        "ADDR_DNS_SERVER",         /* 3. dns server */
        "STATUS_UP",               /* 4. changed to 'up' */
        "STATUS_DOWN",             /* 5. changed to 'down' */
        "STATUS_LINK_UP",          /* 6. changed to 'link up' */
        "STATUS_LINK_DOWN",        /* 7. changed to 'link down' */
        "STATUS_INTERNET_UP",      /* 8. changed to 'internet up' */
        "STATUS_INTERNET_DOWN",    /* 9. changed to 'internet down' */
        "STATUS_DHCP_ENABLE",      /* 10.enable DHCP capability */
        "STATUS_DHCP_DISABLE",     /* 11.disable DHCP capability */
    };
    if (type <= 11)
    {
        rt_kprintf("net device state changed to <%s>\n", netdev_string_info[type]);
        if (type == 7 || type == 9)              //LINK_DOWN
        {
            ;
        }
    }
    else
    {
        rt_kprintf("Unexpected net device state:%d\n", type);
    }
}

static void phy_linkchange(void)
{
    uint16_t phy_status = 0U;
    static uint8_t eth_status = 0;
    struct netdev *netdev = RT_NULL;

    enet_phy_write_read(ENET0, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_status);
    phy_status &= PHY_LINKED_STATUS;
    LOG_D("phy basic status reg is 0x%X", phy_status);

    netdev = netdev_get_by_name("e0");

    if (netdev)
    {
        if (netdev_is_up(netdev))
        {
            /* 配置成默认网卡 */
            netdev_set_default(netdev);
            rt_kprintf("net dev %s is  up\r\n", netdev->name);
            netdev_set_status_callback(netdev, netdev_callback);
        }
    }

    if (phy_status != RESET)//phy_status == 0x4
    {
        if (0 == eth_status)
        {
            LOG_I("link status up\r\n");
            eth_device_linkchange(&(gd32_emac_device0.parent), RT_TRUE);
            eth_status = 1;
        }
    }
    else//phy_status == 0x0
    {
        if (1 == eth_status)
        {
            LOG_I("link status down\r\n");
            eth_device_linkchange(&(gd32_emac_device0.parent), RT_FALSE);
            eth_status = 0;
        }
    }
}


#define PHY_LINK_MASK   BIT(1)
#define PHY_10M_MASK    BIT(2)
#define PHY_100M_MASK   BIT(3)
#define PHY_DUPLEX_MASK BIT(4)

static uint8_t enet_init_flag = 1;
static uint32_t phy_speed;

static void phy_monitor(void *parameter)
{
    uint8_t phy_addr = 0xFF;
    uint8_t phy_speed_new = 0;
    rt_uint16_t temp;
    rt_uint32_t reg_value = 0;

    phy_addr = PHY_ADDRESS;

    /* RESET PHY */
    rt_kprintf("[PHY] Initializing PHY...\r\n");
#if 0
    haidware_reset_phy();
#else
    temp = PHY_Reset;
    enet_phy_write_read(ENET0, ENET_PHY_WRITE, phy_addr, PHY_BCR, &temp);
#endif

    rt_thread_delay(RT_TICK_PER_SECOND * 2);

    temp = PHY_AUTONEGOTIATION;
    enet_phy_write_read(ENET0, ENET_PHY_WRITE, phy_addr, PHY_BCR, &temp);

    rt_kprintf("[PHY] PHY initialized, starting link monitoring...\r\n");

    while (1)
    {
        uint16_t status;
        enet_phy_write_read(ENET0, ENET_PHY_READ, phy_addr, PHY_BSR, &status);

        phy_speed_new = 0;

        if (status & (PHY_AUTONEGO_COMPLETE | PHY_LINKED_STATUS))
        {
            uint16_t SR;

            enet_phy_write_read(ENET0, ENET_PHY_READ, phy_addr, PHY_SR, &SR);
            phy_speed_new = PHY_LINK_MASK;
#if (PHY_TYPE == YT8512H || PHY_TYPE == YT8522)
            if ((SR & PHY_SPEED_STATUS))
            {
                phy_speed_new |= PHY_100M_MASK;
            }
#else
            if ((SR & PHY_SPEED_STATUS) == 0)
            {
                phy_speed_new |= PHY_100M_MASK;
            }
#endif
            if (SR & PHY_DUPLEX_STATUS)
            {
                phy_speed_new |= PHY_DUPLEX_MASK;
            }
        }

        /* linkchange */
        if (phy_speed_new != phy_speed)
        {
            if (phy_speed_new & PHY_LINK_MASK)
            {
                rt_kprintf("[PHY] Link established - ");

                if (phy_speed_new & PHY_100M_MASK)
                {
                    rt_kprintf("100Mbps");
                    reg_value = ENET_SPEEDMODE_100M;
                }
                else
                {
                    rt_kprintf("10Mbps");
                    reg_value = ENET_SPEEDMODE_10M;
                }

                if (phy_speed_new & PHY_DUPLEX_MASK)
                {
                    rt_kprintf(" full-duplex\r\n");
                    reg_value |= ENET_MODE_FULLDUPLEX;
                }
                else
                {
                    rt_kprintf(" half-duplex\r\n");
                    reg_value |= ENET_MODE_HALFDUPLEX;
                }

                ENET_MAC_CFG(ENET0) |= reg_value;

                if (enet_init_flag == 1)
                {
                    rt_kprintf("[PHY] First link up, reinitializing network interface\r\n");

                    gd32_emac_init((rt_device_t)&gd32_emac_device0);

                    enet_init_flag = 0;
                }

                eth_device_linkchange(&gd32_emac_device0.parent, RT_TRUE);

                if (gd32_emac_device0.parent.netif != RT_NULL)
                {
                    struct netif *netif = gd32_emac_device0.parent.netif;
#if LWIP_DHCP
                    rt_kprintf("[PHY] Restarting DHCP client...\r\n");

                    if (netif_dhcp_data(netif) != NULL)
                    {
                        dhcp_stop(netif);
                        dhcp_cleanup(netif);
                    }

                    netif_set_addr(netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY);

                    rt_thread_delay(RT_TICK_PER_SECOND / 5);
                    dhcp_start(netif);

                    rt_kprintf("[PHY] DHCP client restarted\r\n");
#endif
                }

                rt_kprintf("[PHY] Link up process completed\r\n");
            } /* link up. */
            else
            {
                rt_kprintf("[PHY] Link down detected\r\n");

                /* stop DHCP */
                if (gd32_emac_device0.parent.netif != RT_NULL)
                {
#if LWIP_DHCP
                    struct netif *netif = gd32_emac_device0.parent.netif;
                    if (netif_dhcp_data(netif) != NULL)
                    {
                        dhcp_stop(netif);
                        rt_kprintf("[PHY] DHCP stopped due to link down\r\n");
                    }
#endif
                }

                eth_device_linkchange(&gd32_emac_device0.parent, RT_FALSE);
            } /* link down. */

            phy_speed = phy_speed_new;
        } /* linkchange */

        rt_thread_delay(RT_TICK_PER_SECOND / 2);
    } /* while(1) */
}


int rt_hw_gd32_eth_init(void)
{
    rt_err_t state = RT_EOK;

    rt_kprintf("rt_gd32_eth_init...\n");

    nvic_configuration();

    /* Configure clock output for PHY */
    enet_clock_config();

    /* configure the GPIO ports for ethernet pins using new method */
    state = gd32_enet_gpio_init(&gd32_emac_device0);
    if (state != RT_EOK)
    {
        rt_kprintf("ENET GPIO initialization failed: %d\n", state);
        return state;
    }

    /* set autonegotiation mode */
    gd32_emac_device0.phy_mode = EMAC_PHY_AUTO;
    gd32_emac_device0.ETHERNET_MAC = ETHERNET_MAC0;
    gd32_emac_device0.ETHER_MAC_IRQ = ENET0_IRQn;

    // OUI 00-00-0E FUJITSU LIMITED
    gd32_emac_device0.dev_addr[0] = 0x00;
    gd32_emac_device0.dev_addr[1] = 0x11;
    gd32_emac_device0.dev_addr[2] = 0x22;
    /* set mac address: (only for test) */
    gd32_emac_device0.dev_addr[3] = 0x33;
    gd32_emac_device0.dev_addr[4] = 0x44;
    gd32_emac_device0.dev_addr[5] = 0x55;

    gd32_emac_device0.parent.parent.init = gd32_emac_init;
    gd32_emac_device0.parent.parent.open = gd32_emac_open;
    gd32_emac_device0.parent.parent.close = gd32_emac_close;
    gd32_emac_device0.parent.parent.read = gd32_emac_read;
    gd32_emac_device0.parent.parent.write = gd32_emac_write;
    gd32_emac_device0.parent.parent.control = gd32_emac_control;
    gd32_emac_device0.parent.parent.user_data = RT_NULL;

    gd32_emac_device0.parent.eth_rx = gd32_emac_rx;
    gd32_emac_device0.parent.eth_tx = gd32_emac_tx;

    /* init tx buffer free semaphore */
    rt_sem_init(&gd32_emac_device0.tx_buf_free, "tx_buf0", EMAC_TXBUFNB, RT_IPC_FLAG_FIFO);
    eth_device_init(&(gd32_emac_device0.parent), "e0");

    /* change device link status */
    eth_device_linkchange(&(gd32_emac_device0.parent), RT_TRUE);

    /* Start PHY monitoring thread */
    rt_thread_t tid = RT_NULL;
    tid = rt_thread_create("phy_monitor", phy_monitor, RT_NULL, 1024, 20, 1);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
        rt_kprintf("[ETH] PHY monitor thread started\n");
    }
    else
    {
        rt_kprintf("[ETH] Failed to create PHY monitor thread\n");
    }

    return state;
}
INIT_DEVICE_EXPORT(rt_hw_gd32_eth_init);

#ifdef RT_USING_FINSH
static void eth_show_status(void)
{
    rt_uint16_t phy_status, phy_sr;
    struct netif *netif = gd32_emac_device0.parent.netif;

    rt_kprintf("\n=== Ethernet Status [e0] ===\n");

    /* Read PHY status */
    enet_phy_write_read(ENET0, ENET_PHY_READ, PHY_ADDRESS, PHY_BSR, &phy_status);
    enet_phy_write_read(ENET0, ENET_PHY_READ, PHY_ADDRESS, PHY_SR, &phy_sr);

    rt_kprintf("PHY Address: 0x%02X\n", PHY_ADDRESS);
    rt_kprintf("PHY Status Register: 0x%04X\n", phy_status);
    rt_kprintf("PHY Speed Register:  0x%04X\n", phy_sr);
    rt_kprintf("Link Status: %s\n", (phy_status & PHY_LINKED_STATUS) ? "UP" : "DOWN");
    rt_kprintf("Auto-negotiation: %s\n", (phy_status & PHY_AUTONEGO_COMPLETE) ? "Complete" : "In Progress");

    if (phy_status & PHY_LINKED_STATUS)
    {
        rt_kprintf("Speed: %s\n",
#if (PHY_TYPE == YT8512H || PHY_TYPE == YT8522)
                   (phy_sr & PHY_SPEED_STATUS) ? "100Mbps" : "10Mbps"
#else
                   (phy_sr & PHY_SPEED_STATUS) ? "10Mbps" : "100Mbps"
#endif
        );
        rt_kprintf("Duplex: %s\n", (phy_sr & PHY_DUPLEX_STATUS) ? "Full" : "Half");
    }

    rt_kprintf("\n=== Network Interface ===\n");
    if (netif != RT_NULL)
    {
        rt_kprintf("Interface: %c%c%s\n",
                   netif->name[0], netif->name[1],
                   (netif == netif_default) ? " (Default)" : "");
        rt_kprintf("Link: %s\n", netif_is_link_up(netif) ? "UP" : "DOWN");
        rt_kprintf("Status: %s\n", netif_is_up(netif) ? "UP" : "DOWN");
        rt_kprintf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   gd32_emac_device0.dev_addr[0], gd32_emac_device0.dev_addr[1],
                   gd32_emac_device0.dev_addr[2], gd32_emac_device0.dev_addr[3],
                   gd32_emac_device0.dev_addr[4], gd32_emac_device0.dev_addr[5]);
        rt_kprintf("IP: %s\n", ipaddr_ntoa(&(netif->ip_addr)));
        rt_kprintf("Gateway: %s\n", ipaddr_ntoa(&(netif->gw)));
        rt_kprintf("Netmask: %s\n", ipaddr_ntoa(&(netif->netmask)));

#if LWIP_DHCP
        struct dhcp *dhcp = netif_dhcp_data(netif);
        if (dhcp != RT_NULL)
        {
            rt_kprintf("DHCP: Enabled (State: %d)\n", dhcp->state);
        }
        else
        {
            rt_kprintf("DHCP: Disabled\n");
        }
#endif
    }
    else
    {
        rt_kprintf("Network interface not initialized\n");
    }

    rt_kprintf("\n=== Device Status ===\n");
    rt_kprintf("PHY Mode: %s\n",
               gd32_emac_device0.phy_mode == EMAC_PHY_AUTO ? "Auto" : gd32_emac_device0.phy_mode == EMAC_PHY_10MBIT ? "10M"
                                                                                                                    : "100M");
}

static void eth_list_instance(void)
{
    rt_kprintf("=== Ethernet Instance ===\n");

    struct netif *netif = gd32_emac_device0.parent.netif;
    rt_kprintf("Instance 0: e0 - ");
    rt_kprintf("MAC: %02X:%02X:%02X:%02X:%02X:%02X - ",
               gd32_emac_device0.dev_addr[0], gd32_emac_device0.dev_addr[1],
               gd32_emac_device0.dev_addr[2], gd32_emac_device0.dev_addr[3],
               gd32_emac_device0.dev_addr[4], gd32_emac_device0.dev_addr[5]);

    if (netif != RT_NULL)
    {
        rt_kprintf("Link: %s - ", netif_is_link_up(netif) ? "UP" : "DOWN");
        rt_kprintf("IP: %s", ipaddr_ntoa(&(netif->ip_addr)));
        if (netif == netif_default)
        {
            rt_kprintf(" (Default)");
        }
    }
    else
    {
        rt_kprintf("No Netif");
    }
    rt_kprintf("\n");
}

static void eth_dhcp_restart(void)
{
    struct netif *netif = gd32_emac_device0.parent.netif;

    if (netif == RT_NULL)
    {
        rt_kprintf("Network interface not found\n");
        return;
    }

    if (!netif_is_link_up(netif))
    {
        rt_kprintf("Network link is down, cannot restart DHCP\n");
        return;
    }

#if LWIP_DHCP
    rt_kprintf("Restarting DHCP client...\n");

    /* Stop current DHCP */
    if (netif_dhcp_data(netif) != NULL)
    {
        dhcp_stop(netif);
        dhcp_cleanup(netif);
        rt_kprintf("Stopped existing DHCP client\n");
    }

    /* Clear IP address */
    netif_set_addr(netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY);
    rt_kprintf("Cleared IP address\n");

    /* Restart DHCP */
    rt_thread_delay(RT_TICK_PER_SECOND / 5);
    dhcp_start(netif);

    rt_kprintf("DHCP client restarted\n");
#else
    rt_kprintf("DHCP is not enabled in build configuration\n");
#endif
}

static void eth_phy_reset(void)
{
    rt_uint16_t temp;

    rt_kprintf("Resetting PHY...\n");

    /* Software reset PHY */
    temp = PHY_Reset;
    enet_phy_write_read(ENET0, ENET_PHY_WRITE, PHY_ADDRESS, PHY_BCR, &temp);

    rt_thread_delay(RT_TICK_PER_SECOND);

    /* Restart auto-negotiation */
    temp = PHY_AUTONEGOTIATION;
    enet_phy_write_read(ENET0, ENET_PHY_WRITE, PHY_ADDRESS, PHY_BCR, &temp);

    rt_kprintf("PHY reset completed\n");
}

static void eth_set_default_interface(void)
{
    struct netif *netif = gd32_emac_device0.parent.netif;

    if (netif == RT_NULL)
    {
        rt_kprintf("Network interface not found\n");
        return;
    }

    netif_set_default(netif);
    rt_kprintf("Set e0 as default network interface\n");
}

static void eth_cmd(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: eth <command> [options]\n");
        rt_kprintf("Commands:\n");
        rt_kprintf("  status       - Show ethernet status\n");
        rt_kprintf("  list         - List ethernet instance\n");
        rt_kprintf("  dhcp_restart - Restart DHCP client\n");
        rt_kprintf("  phy_reset    - Reset PHY\n");
        rt_kprintf("  set_default  - Set as default network interface\n");
        rt_kprintf("  help         - Show this help message\n");
        rt_kprintf("\nExamples:\n");
        rt_kprintf("  eth status       - Show interface status\n");
        rt_kprintf("  eth dhcp_restart - Restart DHCP\n");
        rt_kprintf("  eth phy_reset    - Reset PHY\n");
        rt_kprintf("  eth set_default  - Set as default interface\n");
        return;
    }

    const char *command = argv[1];

    if (rt_strcmp(command, "status") == 0)
    {
        eth_show_status();
    }
    else if (rt_strcmp(command, "list") == 0)
    {
        eth_list_instance();
    }
    else if (rt_strcmp(command, "dhcp_restart") == 0)
    {
        eth_dhcp_restart();
    }
    else if (rt_strcmp(command, "phy_reset") == 0)
    {
        eth_phy_reset();
    }
    else if (rt_strcmp(command, "set_default") == 0)
    {
        eth_set_default_interface();
    }
    else if (rt_strcmp(command, "help") == 0)
    {
        eth_cmd(1, RT_NULL);
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", command);
        rt_kprintf("Use 'eth help' for usage information.\n");
    }
}
MSH_CMD_EXPORT_ALIAS(eth_cmd, eth, ethernet management commands);

#endif /* RT_USING_FINSH */
#endif /* RT_USING_SAL && RT_USING_NETDEV && RT_USING_LWIP */
