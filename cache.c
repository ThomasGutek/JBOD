#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;
bool cache_created = false;

int cache_create(int num_entries) {
    if (num_entries < 2 || num_entries > 4096) return -1;
    if (cache_created == true) return -1;
    // allocate space
    cache = (cache_entry_t *)calloc(sizeof(cache_entry_t), num_entries);
    // cache = (cache_entry_t *)calloc(num_entries, cache_size);
    cache_size = num_entries;
    cache_created = true;
    return 1;
}

int cache_destroy(void) {
    if (cache_created == false) return -1;
    free(cache);
    cache = NULL;
    cache_size = 0;
    cache_created = false;
    return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
    // check parameters
    if (!cache) {
        return -1;
    }
    // printf("pass cache check\n");
    // fail if cache is empty
    if ((buf == NULL)) {
        return -1;
    }
    // printf("pass empty check\n");

    // lookup block and dis
    num_queries++;
    for (int i = 0; i < cache_size; i++) {
        // if (!cache[i].valid) {
        //   return -1;
        //}
        // printf("pass valid check\n");
        if (((cache[i].disk_num == disk_num) &&
             (cache[i].block_num == block_num)) &&
            (cache[i].valid != false)) {
            memcpy(buf, cache[i].block, 256);
            num_hits++;
            clock++;
            cache[i].access_time = clock;
            return 1;
        }
        // incrementations on success
    }
    // incrementations always
    return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
    // if the entry exists in cache, update the block with buf{
    for (int i = 0; i < cache_size; i++) {
        if ((cache[i].disk_num == disk_num) &&
            (cache[i].block_num == block_num) && (cache[i].valid != false)) {
            memcpy(cache[i].block, buf, 256);
            clock++;
            cache[i].access_time = clock;
        }
    }
    return;
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
    // check parameters
    // valid disk and block numbers
    if (disk_num < 0 || block_num < 0 || disk_num > 16 || block_num > 256) {
        return -1;
    }
    // printf("pass address check\n");

    // fail if buf or cache empty
    if (buf == NULL || cache == NULL) {
        // printf("failed empty check\n");
        //  assert(cache == NULL);
        return -1;
    }
    // printf("pass empty check\n");

    for (int i = 0; i < cache_size; i++) {
        // fail if inserting into existing cache
        if ((cache[i].valid == true) && (cache[i].disk_num == disk_num) &&
            (cache[i].block_num == block_num)) {
            return -1;
        }
        if (cache[i].valid == false) {
            // insert the disk and block number into cache
            cache[i].disk_num = disk_num;
            cache[i].block_num = block_num;
            // copy buf to cache entry
            memcpy(cache[i].block, buf, 256);
            cache[i].valid = true;
            clock++;
            cache[i].access_time = clock;
            return 1;
        }
    }
    int LRU = cache[0].access_time;
    for (int j = 1; j < cache_size; j++) {
        if (LRU > cache[j].access_time) LRU = cache[j].access_time;
    }
    for (int k = 0; k < cache_size; k++) {
        if (cache[k].access_time == LRU) {
            // insert the disk and block number into cache
            cache[k].disk_num = disk_num;
            cache[k].block_num = block_num;
            // copy buf to cache entry
            memcpy(cache[k].block, buf, 256);
            clock++;
            cache[k].access_time = clock;
        }

        // return 1;
    }
    return 1;
}

bool cache_enabled(void) {
    return false;
    if (cache)
        return true;
    else
        return false;
}

void cache_print_hit_rate(void) {
    fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float)num_hits / num_queries);
    // fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float)num_hits /
    //  num_queries);
}