#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include "common.h"

uint8_t superblock_calc_checksum(superblock *s) 
{
    uint8_t total = s->version + s->total_blocks + s->reserved_blocks + s->block_size;
    return 256 - (total & 0x0ff);
}

/**
 * SFS uses a 64bit timestamp
 */
long long get_milliseconds()
{
    struct timespec spec;
    
    clock_gettime(CLOCK_REALTIME, &spec);

    return (spec.tv_sec * 1000) + (spec.tv_nsec / 1.0e6);
}

long long get_media_size(superblock *s)
{
    long bytes_per_block = 1 << (s->block_size + 7);
    return s->total_blocks * bytes_per_block;
}

