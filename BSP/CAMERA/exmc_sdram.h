#ifndef EXMC_SDRAM_H
#define EXMC_SDRAM_H

#include "gd32h7xx.h"

void exmc_synchronous_dynamic_ram_init(uint32_t sdram_device);

#define SDRAM_DEVICE0_ADDR                         ((uint32_t)0xC0000000U)
#define SDRAM_DEVICE1_ADDR                         ((uint32_t)0xD0000000U)

#endif
