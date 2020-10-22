#include "common.h"
#include "stdlib.h"
#include "burst.h"
#include "memory/cache.h"
#include "memory/tlb.h"
#include "cpu/reg.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);
lnaddr_t seg_translate(swaddr_t, size_t, uint8_t);
void sreg_load(uint8_t);
hwaddr_t page_translate(lnaddr_t, size_t); 


/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	int fir_id = cache_read(addr);	//get cache id
	//printf("id:%d\n",fir_id);
	uint32_t in_addr = addr & (CACHE_BLOCK_SIZE - 1); //inside addr
	uint8_t tmp[2 * BURST_LEN];
	if(in_addr + len >= CACHE_BLOCK_SIZE) {
		// it's time to use unalign_rw 
		int sec_id = cache_read(addr + len);
		memcpy(tmp, cache[fir_id].data + in_addr, CACHE_BLOCK_SIZE - in_addr);
		memcpy(tmp + CACHE_BLOCK_SIZE - in_addr, cache[sec_id].data, len - (CACHE_BLOCK_SIZE - in_addr));
	} else {
		memcpy(tmp, cache[fir_id].data + in_addr, len);
	}
	int zero = 0;
	uint32_t ans = unalign_rw(tmp + zero, 4) & (~0u >> ((4 - len) << 3));
	return ans;
	//return dram_read(addr, len) & (~0u >> ((4 - len) << 3));
}

/*uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return dram_read(addr, len) & (~0u >> ((4 - len) << 3));
}*/

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	//dram_write(addr, len, data);	
	cache_write(addr,len,data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	/*page*/
	hwaddr_t hwaddr = page_translate(addr, len); 
	return hwaddr_read(hwaddr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	/*page*/
	hwaddr_t hwaddr = page_translate(addr, len);
	hwaddr_write(hwaddr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_t lnaddr = seg_translate(addr,len,sreg);
	//printf("addr: %d\n",lnaddr);
	return lnaddr_read(lnaddr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_t lnaddr = seg_translate(addr, len, sreg);
	lnaddr_write(lnaddr, len, data);
}

/* seg function*/
lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg) {
	if (cpu.cr0.protect_enable == 1) {
		//printf("Changed!\n");
		Assert(addr+len < cpu.sr[sreg].cache_limit, "Segment Fault!");
		return cpu.sr[sreg].cache_base + addr;
	}
	else return addr;	
}

void sreg_load(uint8_t sreg) {
	//printf("Used!\n");
	uint32_t gd = cpu.gdtr.base_addr;
	gd += cpu.sr[sreg].index << 3;
	SegDescriptor sd;
	sd.first = lnaddr_read(gd, 4);
	sd.second = lnaddr_read(gd + 4, 4);
	uint32_t base = (((uint32_t)sd.base2) << 16) | sd.base1 | (((uint32_t)sd.base3) << 24);
	uint32_t limit = (((uint32_t)sd.limit2) << 16) | sd.limit1;
	if(sd.g) limit <<= 12;
	cpu.sr[sreg].cache_limit = limit;
	cpu.sr[sreg].cache_base = base;
}

/*page function*/
hwaddr_t page_translate(lnaddr_t addr, size_t len) {
	if(cpu.cr0.protect_enable && cpu.cr0.paging) {
		hwaddr_t tmp_addr;
		if((tmp_addr = tlb_read(addr & 0xfffff000)) != -1) return (tmp_addr << 12) + (addr & 0xfff);
		PageDescriptor dir, page;
		uint32_t dir_in_addr = addr >> 22;
		uint32_t page_in_addr = ((addr >> 12) & 0x3ff);
		uint32_t in_addr = addr & 0xfff;
		dir.page_val = hwaddr_read((cpu.cr3.page_directory_base << 12) + (dir_in_addr << 2), 4);
		Assert(dir.p, "Invalid page!"); //avoid error
		page.page_val = hwaddr_read((dir.addr << 12) + (page_in_addr << 2), 4);
		Assert(page.p, "Invalid page!"); //avoid error
		tlb_write(addr & 0xfffff000, page.addr);
		return (page.addr << 12) + in_addr;
	} else {
		return addr;
	}
}

