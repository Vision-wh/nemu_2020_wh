#include "common.h"
#include "memory/cache.h"
#include <stdlib.h>
#include "burst.h"
//#include "memory/dram.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);
void ddr3_read_public(hwaddr_t addr, void *data);
void ddr3_write_public(hwaddr_t addr, void *data, uint8_t *mask);

/*cache function*/

void init_cache(){
	int i;
	for (i = 0;i < CACHE_SIZE/CACHE_BLOCK_SIZE;i++){
		cache[i].valid = false;
		//cache[i].tag = 0;
		//memset(cache[i].data,0,CACHE_SIZE);
	}
	for (i = 0;i < CACHE2_SIZE/CACHE2_BLOCK_SIZE;i++){
		cache2[i].valid = false;
		cache2[i].dirty = false;
		//cache2[i].tag = 0;
		//memset(cache2[i].data,0,CACHE2_SIZE);
	}
}

int cache_read(hwaddr_t addr) 
{
	uint32_t g_num = (addr >> 6) & 0x7f; //group number
	//printf("group number:%d\n",(g_num+1)*WAY_8);
	//uint32_t block = (addr >> 6)<<6;
	int i,j;
	bool flag = false;
	for (i = g_num * WAY_8 ; i < (g_num + 1) * WAY_8 ;i++){
		if (cache[i].tag == (addr >> 13) && cache[i].valid){
				flag = true;
				break;
		}
	}
	if (flag == false) {
		j = cache2_read (addr);
		for (i = g_num * WAY_8 ; i < (g_num+1) * WAY_8 ;i++){
			if (!cache[i].valid)break;
		}
		//use random way for replace
		if (i == (g_num + 1) * WAY_8){
			srand (0);
			i = g_num * WAY_8 + rand() % WAY_8;
		}
		memcpy (cache[i].data,cache2[j].data,CACHE_BLOCK_SIZE);
		//1-level cache
		/*for(j = 0; j < CACHE_BLOCK_SIZE / BURST_LEN; j++) {
			ddr3_read_public(block + j * BURST_LEN , cache[i].data + j * BURST_LEN);
		}*/
		cache[i].valid = true;
		cache[i].tag = addr >> 13;
		
	}
	return i;
}

void cache_write(hwaddr_t addr, size_t len,uint32_t data) {
	uint32_t g_num = (addr>>6) & 0x7f; //group number
	uint32_t in_addr = addr & (CACHE_BLOCK_SIZE - 1); // inside addr
	//bool v = false;
	int i;
	for (i = g_num * WAY_8 ; i < (g_num + 1) * WAY_8 ;i++){
		if (cache[i].tag == (addr >> 13) && cache[i].valid){
			if(in_addr + len > CACHE_BLOCK_SIZE) {
				//write
				dram_write(addr, CACHE_BLOCK_SIZE - in_addr, data);
				memcpy(cache[i].data + in_addr, &data, len);
				cache2_write(addr, CACHE_BLOCK_SIZE - in_addr, data);
				cache_write(addr + CACHE_BLOCK_SIZE - in_addr, len - CACHE_BLOCK_SIZE + in_addr, data >> (CACHE_BLOCK_SIZE - in_addr));
			} 
			else {
				dram_write(addr, len, data);
				memcpy(cache[i].data + in_addr, &data, len);
				cache2_write(addr, len, data);
			}
			return;
		}
		//else dram_write(addr, len, data);
	}
	cache2_write(addr,len,data);
	//i = cache_read(addr);
	//cache[i].valid = true;
	//memcpy(cache[i].data + in_addr, &data, len);
}

int cache2_read(hwaddr_t addr) {
	uint32_t g_num = (addr >> 6) & ((1 << 12) - 1);
	uint32_t block = (addr >> 6) << 6;
	int i = 0;
	bool flag = false;
	for (i = g_num * WAY_16 ; i < (g_num + 1) * WAY_16 ;i ++) {
		if (cache2[i].tag == (addr >> 18)&& cache2[i].valid){
				flag = true;
				break;
		}
	}
	if (!flag) {
		int j;
		for (i = g_num * WAY_16 ; i < (g_num + 1) * WAY_16 ;i ++){
			if (!cache2[i].valid)break;
		}
		//use random way for replace 
		if (i == (g_num + 1) * WAY_16){
			srand (0);
			i = g_num * WAY_16 + rand() % WAY_16;
		}
		if (cache2[i].dirty && cache2[i].valid) {
				uint8_t mask[BURST_LEN * 2];
				memset(mask, 1, BURST_LEN * 2);
				uint32_t block2 = (cache2[i].tag << 18) | (g_num << 6);
				for (j = 0;j < CACHE2_BLOCK_SIZE/BURST_LEN;j++){
					ddr3_write_public(block2 + j * BURST_LEN, cache2[i].data + j * BURST_LEN, mask);
				}
		}
		for (j = 0;j < BURST_LEN;j ++){
			ddr3_read_public(block + j * BURST_LEN , cache2[i].data + j * BURST_LEN);
		}
		cache2[i].valid = true;
		cache2[i].tag = addr >> 18;
		cache2[i].dirty = false;
	}
	return i;
}

void cache2_write(hwaddr_t addr, size_t len, uint32_t data) {
	uint32_t g_num = (addr >> 6) & ((1<<12) - 1);  //group number
	uint32_t in_addr = addr & (CACHE2_BLOCK_SIZE - 1); // inside addr
	int i;
	bool flag = false;
	for (i = g_num * WAY_16 ; i < (g_num + 1) * WAY_16 ;i++) {
		if (cache2[i].tag == (addr >> 18) && cache2[i].valid) {
				flag = true;
				break;
		}
	}
	if (flag){
		cache2[i].dirty = true;
		if(in_addr + len > CACHE2_BLOCK_SIZE) {//across
			memcpy(cache2[i].data + in_addr, &data, CACHE2_BLOCK_SIZE - in_addr);
			cache2_write(addr + CACHE2_BLOCK_SIZE - in_addr, len - CACHE2_BLOCK_SIZE + in_addr, data >> (CACHE2_BLOCK_SIZE - in_addr));
		} else {
			memcpy(cache2[i].data + in_addr, &data, len);
		}
		return;
	}
	i = cache2_read(addr);
	cache2[i].dirty = true;
	memcpy(cache2[i].data + in_addr, &data, len);
}

