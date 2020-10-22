#ifndef __CACHE_H__
#define __CACHE_H_

#define CACHE_BLOCK_SIZE 64 //64b
#define CACHE_SIZE 64*1024 //64kb
#define WAY_8 8 //8-way set associative

#define CACHE2_BLOCK_SIZE 64 //64b
#define CACHE2_SIZE 1024*1024*4 //4MB
#define WAY_16 16 //16-way set associative

typedef struct {
	bool valid; //valid bit
	int tag;
	uint8_t data[CACHE_BLOCK_SIZE];
} Cache;

typedef struct {
	bool valid,dirty; //valid and dirty bit
	int tag;
	uint8_t data[CACHE2_BLOCK_SIZE];;
} Cache2;

Cache cache[CACHE_SIZE/CACHE_BLOCK_SIZE];
Cache2 cache2[CACHE2_SIZE/CACHE2_BLOCK_SIZE];

void init_cache();
int cache_read(hwaddr_t addr);
void cache_write(hwaddr_t addr, size_t len, uint32_t data);
int cache2_read(hwaddr_t addr);
void cache2_write(hwaddr_t addr, size_t len, uint32_t data);

#endif
