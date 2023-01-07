/*#include <stdio.h>
#include "cache.h"

int main(int argc, char *argv[]) {
    // Create a new cache
    cache_t *cache = cache_create(4, 128, 16);

    // Read from cache
    int addr = 0;
    int result = cache_read(cache, addr);
    if (result >= 0) {
        printf("Cache hit at address %d, block %d\n", addr, result);
    } else {
        printf("Cache miss at address %d\n", addr);
    }

    // Write to cache
    cache_write(cache, addr);
    result = cache_read(cache, addr);
    if (result >= 0) {
        printf("Cache hit at address %d, block %d\n", addr, result);
    } else {
        printf("Cache miss at address %d\n", addr);
    }

    // Clean up
    cache_delete(cache);

    return 0;
}*/