/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-12     RTT       the first version
 */
#ifndef DRIVERS_DRV_LEGACY_H_
#define DRIVERS_DRV_LEGACY_H_

#include <sys/types.h>
#include "rtdef.h"

#if RTTHREAD_VERSION <= 40101
#if defined(RT_USING_LIBC) && !defined(RT_USING_NANO)
typedef size_t                          rt_size_t;      /**< Type for size number */
typedef ssize_t                         rt_ssize_t;     /**< Used for a count of bytes or an error indication */
#else
typedef rt_base_t                       rt_ssize_t;     /**< Used for a count of bytes or an error indication */
#endif /* defined(RT_USING_LIBC) && !defined(RT_USING_NANO) */
#else
#define RT_SECTION rt_section
#define RT_WEAK    rt_weak
#define RT_USED    rt_used
#ifndef ALIGN
#define ALIGN      rt_align
#endif
#endif /* RTTHREAD_VERSION <= RT_VERSION_CHECK(4, 1, 1) */

#endif /* DRIVERS_DRV_LEGACY_H_ */
