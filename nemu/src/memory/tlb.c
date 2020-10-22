#include "common.h"
#include "memory/tlb.h"
#include <stdlib.h>
#include "burst.h"

/* tlb function */

uint32_t tlb_read(uint32_t tag) {
	int i;
	for(i = 0; i < TLB_SIZE; i++) {
		if(tlb[i].valid && tlb[i].tag == tag) return tlb[i].page;
	}
	return -1; //invalid
}

void init_tlb() {
	int i;
	for(i = 0; i < TLB_SIZE; i++) {
		tlb[i].valid = false;
	}
}

void tlb_write(uint32_t tag, uint32_t page) {
	int i;
	for(i = 0; i < TLB_SIZE; i++) {
		if(!tlb[i].valid) {
			tlb[i].valid = true;
			tlb[i].tag = tag;
			tlb[i].page = page;
			return;
		}
	}
	i = rand() % TLB_SIZE; //random way
	tlb[i].valid = true;
	tlb[i].tag = tag;
	tlb[i].page = page;
	return;
}
