#include <stdio.h>
#include <stdlib.h>

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

// Initializing the cache with the given size, block size, and number of ways.
void cache(Cache* cache, int size, int block_size, int ways) {
    cache->size = size;
    cache->block_size = block_size;
    cache->ways = ways;
    for (int i = 0; i < MAX_LINES; i++) {
        cache->lines[i].valid = 0;
    }
}

// Reads from the cache at the given address.
/*int read(Cache* cache, int address) {
    // Calculates the tag and index for the cache line.
    int tag = address / cache->block_size;
    int index = (address / cache->block_size) % cache->ways;

    // Checks if the cache line is valid and the tag matches.
    CacheLine* line = &cache->lines[index];
    if (line->valid && line->tag == tag) {
        // The block is in the cache, return the data.
        return line->data;
    } else {
        // The block is not in the cache, read it from memory.
        line->valid = 1;
        line->tag = tag;
        line->data = memory[address];
        return line->data;
    }
}*/

// Function to simulate a cache hit
int cache_hit(int address) {
  Cache cache;
  int block_offset = address % cache.block_size;
  int block_number = address / cache.block_size;
  int set_number = block_number % cache.ways;

  // Search the set for the requested cache line
  for (int i = 0; i < cache.ways; i++) {
    CacheLine line = cache.lines[set_number * cache.ways + i];
    if (line.valid && line.data == block_number) {
      // Update the LRU field of the cache line
      line.lru = 0;
      for (int j = 0; j < cache.ways; j++) {
        if (j != i) {
          cache.lines[set_number * cache.ways + j].lru++;
        }
      }
      // Return the data from the cache line
      return line.data[block_offset];
    }
  }

  // Cache miss
  return -1;
}

// Function to simulate a cache miss
void cache_miss(int address, int data) {
  Cache cache;
  int block_offset = address % cache.block_size;
  int block_number = address / cache.block_size;
  int set_number = block_number % cache.ways;

  // Evict the least recently used cache line if the set is full
  int lru_line = 0;
  for (int i = 0; i < cache.ways; i++) {
    CacheLine line = cache.lines[set_number * cache.ways + i];
    if (!line.valid || line.lru > cache.lines[set_number * cache.ways + lru_line].lru) {
      lru_line = i;
    }
  }

  // Bring the requested data into the cache
  CacheLine line = cache.lines[set_number * cache.ways + lru_line];
  line.valid = 1;
  line.data = block_number;
  line.dirty = 0;
  line.lru = 0;
  for (int i = 0; i < cache.block_size; i++) {
    line.data[i] = data;
  }
  for (int i = 0; i < cache.ways; i++) {
    if (i != lru_line) {
      cache.lines[set_number * cache.ways + i].lru++;
    }
  }
}

int read(int address) {
  int data = cache_hit(address);
  if (data == -1) {
    // Cache miss, retrieve data from memory/stock
    data = memory_read(address);
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
        // The block is in the cache, update the data.
        line->dirty = 1;
        line->data = data;
    } else {
        // The block is not in the cache, write it to memory.
        line->valid = 1;
        line->tag = tag;
        line->dirty = 1;
        line->data = data;
        memory[address] = data;
    }
}

int main() {
    // Initialize the cache with a size of 16KB, block size of 4B, and 2 ways
    Cache cache0;
    cache(&cache0, 16384, 4, 2);

    // Read and write to the cache
    int data = read(0);
    printf("Read data: %d\n", data);
    write(&cache0, 0, 42);
}
