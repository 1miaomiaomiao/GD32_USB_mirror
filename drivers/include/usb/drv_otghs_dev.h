/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-07     RealThread   the first version
 */

#ifndef __DRV_OTGFS_DEV_H__
#define __DRV_OTGFS_DEV_H__

#include <rtthread.h>
#include "drv_usb_dev.h"

void usbd_reset_callback(void);

void usbd_setup_stage_callback(struct urequest *setup_packet);

void usbd_data_in_stage_callback(uint8_t epnum, uint16_t data_count);

void usbd_data_out_stage_callback(uint8_t epnum, uint16_t data_count);

void usbd_set_connection_state(uint8_t state);

int gd_usbd_register(void);

#endif
