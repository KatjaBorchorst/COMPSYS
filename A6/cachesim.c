#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

#define MAX_LINES 1000  // Max amount of cache lines.

// Struct for the cache line data.
typedef struct {
  int valid;
  int dirty;
  int tag;
  int *data;
  int lru;
} CacheLine;

// Struct for the cache data.
typedef struct {
  int size;
  int block_size;
  int ways;
  CacheLine lines[MAX_LINES];
} Cache;

// The memory data structure.
int memory[10000];

// Initializes the cache with the cache-size, block-size, and number of ways.
void cache(Cache* cache, int size, int block_size, int ways) {
  cache->size = size;
  cache->block_size = block_size;
  cache->ways = ways;
  for (int i = 0; i < MAX_LINES; i++) {
    cache->lines[i].valid = 0;
  }
}

// Function to simulate a cache-hit.
int cache_hit(int address) {
  Cache Cache;
  int block_offset = address % Cache.block_size;
  int block_number = address / Cache.block_size;
  int set_number = block_number % Cache.ways;

  // Searches the set for the requested cache line.
  for (int i = 0; i < Cache.ways; i++) {
    CacheLine line = Cache.lines[set_number * Cache.ways + i];
    if (line.valid && line.data == block_number) {
      // Update the LRU field of the cache line
      line.lru = 0;
      for (int j = 0; j < Cache.ways; j++) {
        if (j != i) {
          Cache.lines[set_number * Cache.ways + j].lru++;
        }
      }
      // Returns the data from the cache line.
      return line.data[block_offset];
    }
  }

  // Cache miss.
  return -1;
}

// Function to simulate a cache-miss.
void cache_miss(int address, int data) {
  Cache Cache;
  int block_offset = address % Cache.block_size;
  int block_number = address / Cache.block_size;
  int set_number = block_number % Cache.ways;

  // Removes the LRU-cache line if the set is full.
  int lru_line = 0;
  for (int i = 0; i < Cache.ways; i++) {
    CacheLine line = Cache.lines[set_number * Cache.ways + i];
    if (!line.valid || line.lru > Cache.lines[set_number * Cache.ways + lru_line].lru) {
      lru_line = i;
    }
  }

  // Brings the requested data into the cache.
  CacheLine line = Cache.lines[set_number * Cache.ways + lru_line];
  line.valid = 1;
  line.data = block_number;
  line.dirty = 0;
  line.lru = 0;
  for (int i = 0; i < Cache.block_size; i++) {
    line.data[i] = data;
  }
  for (int i = 0; i < Cache.ways; i++) {
    if (i != lru_line) {
      Cache.lines[set_number * Cache.ways + i].lru++;
    }
  }
}

int read(int address) {
  int data = cache_hit(address);
  if (data == -1) {
    // Cache miss, retrieve data from memory/stock.
    data = memory_rd_b(*memory, address); 
    cache_miss(address, data);
  }
  return data;
}

// Writes to the cache at the given address.
void write(Cache* cache, int address, int data) {
  // Calculates the tag and index for the cache line.
  int tag = address / cache->block_size;
  int index = (address / cache->block_size) % cache->ways;

  // Checks if the cache line is valid and the tag matches.
  CacheLine* line = &cache->lines[index];
  if (line->valid && line->tag == tag) {
    // The block is in the cache; updates the data.
    line->dirty = 1;
    line->data = data;
  } else {
    // The block is not in the cache; writes it to memory.
    line->valid = 1;
    line->tag = tag;
    line->dirty = 1;
    line->data = data;
    memory[address] = data;
  }
}

int main() {
  // Initializes the cache with a size of 16KB, block size of 4B, and 2 ways.
  Cache cache0;
  cache(&cache0, 16384, 4, 2);

  // Read and write to the cache
  int data = read(0);
  printf("Read data: %d\n", data);
  write(&cache0, 0, 42);
}
