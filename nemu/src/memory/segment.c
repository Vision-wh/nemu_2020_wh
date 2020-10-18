#include <nemu.h>

lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg) {
	if (cpu.cr0.protect_enable == 0)return addr;
	Assert(addr+len < cpu.sr[sreg].cache.limit, "cs segment out limit");
	return cpu.sr[sreg].cache.base + addr;	
}
