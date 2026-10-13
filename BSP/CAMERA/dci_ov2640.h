#ifndef DCI_OV2640_H
#define DCI_OV2640_H

#include "sccb.h"
#define SRAM_Address       0x20010000
#define DCI_DATA_ADDRESS   ((uint32_t)&DCI_DATA)
#define DCI_TIMEOUT        20000
#define GPIO_AF_CKOUT      GPIO_AF_0

typedef struct {
    uint8_t manufacturer_id1;
    uint8_t manufacturer_id2;
    uint8_t version;
    uint8_t pid;
} ov2640_id_struct;

#define OV2640_PID        0x0A
#define OV2640_VER        0x0B
#define OV2640_MIDH       0x1C
#define OV2640_MIDL       0x1D

void dci_config(void);
void ckout0_init(void);
uint8_t dci_ov2640_init(void);
uint8_t ov2640_outsize_set(uint16_t width, uint16_t height);

#endif
