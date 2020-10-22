#ifndef __DRAM_H__
#define __DRAM_H__

#include "common.h"

void init_ddr3();
void ddr3_read(hwaddr_t addr, void *data);
void ddr3_write(hwaddr_t addr, void *data, uint8_t *mask);
uint32_t dram_read(hwaddr_t addr, size_t len);
void dram_write(hwaddr_t addr, size_t len, uint32_t data);

#endif
