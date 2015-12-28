/* 
 * File:   sfs.h
 * Author: rday
 *
 * Created on December 21, 2015, 6:49 PM
 */

#ifndef SFS_H
#define	SFS_H

#define SFS_MAGIC_NUMBER	0x10534653

#define SUPERBLOCK_OFFSET   0x0194
#define INDEX_ENTRY_SIZE    0x40

#define BOOT_CODE_1	11
#define BIOS		21
#define BOOT_CODE_2	372

#define VOLUME_ID_ENTRY         0x01
#define STARTING_MARKER_ENTRY   0x02
#define UNUSED_ENTRY            0x10
#define DIRECTORY_ENTRY         0x11
#define FILE_ENTRY              0x12
#define UNUSABLE_ENTRY          0x18
#define DEL_DIRECTORY_ENTRY     0x19
#define DEL_FILE_ENTRY          0x1A

typedef struct superblock {
    long long alteration_time;
    long long data_blocks;
    long long index_bytes;

    union {
        char magic[3]; // First 3 bytes
        int version; // Last byte
    };
    long long total_blocks;
    unsigned int reserved_blocks;
    uint8_t block_size;
    uint8_t checksum;
} superblock;

typedef struct filesystem {
    int fd;
    superblock *s_block;
    char *map;
    char *data_region;
    char *free_region;
    char *index_region;
} filesystem;

typedef struct __attribute__((__packed__)) volume_id_entry {
    char unused[3];
    long long timestamp;
    char volume_name[52];
} volume_id_entry;

typedef struct __attribute__((__packed__)) dir_entry {
    uint8_t continuation_entries;
    long long timestamp;
    char dir_name[54];
} dir_entry;

typedef struct __attribute__((__packed__)) file_entry {
    uint8_t continuation_entries;
    long long timestamp;
    long long starting_block;
    long long ending_block;
    long long length;
    char file_name[30];
} file_entry;

typedef struct __attribute__((__packed__)) unusable_entry {
    char first_unused[9];
    long long starting_block;
    long long ending_block;
    char second_unused[38];
} unusable_entry;

typedef struct __attribute__((__packed__)) first_entry_marker {
	long long next_starting_block;
} first_entry_marker;

typedef struct __attribute__((__packed__)) index_entry {
    uint8_t type;
    union {
	struct first_entry_marker first_entry;
        struct volume_id_entry volume_id;
        struct dir_entry dir;
        struct file_entry file;
	struct unusable_entry unusable;
        unsigned char data[63];
    };
    
} index_entry;

#endif	/* SFS_H */

