#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "../common/sfs.h"
#include "common.h"

filesystem *open_filesystem(char *fname)
{
    // We need to read in the superblock to get
    // enough information to map the file.
    char *buf = malloc(SUPERBLOCK_OFFSET + sizeof(superblock));

    int fd = open(fname, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("File open");
        return NULL;
    }

    if (read(fd, buf, SUPERBLOCK_OFFSET + sizeof(superblock))<0) {
        perror("Reading file");
        return NULL;
    }

    // Point to the superblock, and grab the mapped filesystem
    superblock *s = (superblock*)(buf + SUPERBLOCK_OFFSET);
    filesystem *fs = map_filesystem(fd, s);
    if (fs==NULL) {
        return NULL;
    }

    fs->fd = fd;

    // Cleanup. We will read the superblock from the map
    // from now on.
    free(buf);
    return fs;
}

filesystem *create_filesystem(int fd, superblock *s) 
{
    long long media_size = get_media_size(s);

    // Meet the SFS spec
    s->reserved_blocks = 1;
    strcpy(s->magic, "\x53\x46\x53\x10");
    s->checksum = superblock_calc_checksum(s);
    s->index_bytes = sizeof(struct index_entry);    // Room for one entry

    // Make sure the file is at least the size that
    // we intend to map
    ftruncate(fd, media_size);

    filesystem *fs = map_filesystem(fd, s);
    if (fs == NULL) {
        return NULL;
    }

    // Copy our supplied superblock into the map
    memcpy(fs->s_block, (void*) s, sizeof (superblock));

    // Hack to add the initial STARTING entry, and then reset the
    // index_region pointer. This ensures that the first index entry
    // is always the STARTING entry.
    add_index_entry(fs, STARTING_MARKER_ENTRY);
    fs->index_region += sizeof(struct index_entry);
    fs->s_block->index_bytes -= sizeof(struct index_entry);

    // Create an appropriate Volume ID Index Entry
    // This entry will be closest to the end of the media.
    struct index_entry *volume_id = add_index_entry(fs, VOLUME_ID_ENTRY);
    volume_id->volume_id.timestamp = get_milliseconds();
    strcpy(volume_id->volume_id.volume_name, "The Header");

    // Create a directory entry
    struct index_entry *first_dir = add_index_entry(fs, DIRECTORY_ENTRY);
    strcpy(first_dir->dir.dir_name, "first_directory");
    first_dir->dir.timestamp = get_milliseconds();
    first_dir->dir.continuation_entries = 0;

    // Create another directory entry
    struct index_entry *second_dir = add_index_entry(fs, DIRECTORY_ENTRY);
    strcpy(second_dir->dir.dir_name, "second_directory");
    second_dir->dir.timestamp = get_milliseconds();
    second_dir->dir.continuation_entries = 0;

    // Create a file entry
    struct index_entry *first_file = add_index_entry(fs, FILE_ENTRY);
    strcpy(first_file->file.file_name, "first_file");
    first_file->file.timestamp = get_milliseconds();
    first_file->file.continuation_entries = 0;
    first_file->file.starting_block = 0;
    first_file->file.ending_block = 0;
    first_file->file.length = 12;

    memcpy(fs->data_region, "Hello world!", 12);

    // Create a file entry
    struct index_entry *second_file = add_index_entry(fs, FILE_ENTRY);
    strcpy(second_file->file.file_name, "second_file");
    second_file->file.timestamp = get_milliseconds();
    second_file->file.continuation_entries = 0;
    second_file->file.starting_block = 1;
    second_file->file.ending_block = 1;
    second_file->file.length = 24;

    memcpy(fs->data_region + 512, "This is the second file\n", 24);

    // Push the index region beyond a single block (more than 512 bytes
    // worth of entries).
    struct index_entry *dir = add_index_entry(fs, DIRECTORY_ENTRY);
    strcpy(dir->dir.dir_name, "你好");
    dir->dir.timestamp = get_milliseconds();
    dir->dir.continuation_entries = 0;

    dir = add_index_entry(fs, DIRECTORY_ENTRY);
    strcpy(dir->dir.dir_name, "¡Hola, Mundo!");
    dir->dir.timestamp = get_milliseconds();
    dir->dir.continuation_entries = 0;

    dir = add_index_entry(fs, DIRECTORY_ENTRY);
    strcpy(dir->dir.dir_name, "more entries");
    dir->dir.timestamp = get_milliseconds();
    dir->dir.continuation_entries = 0;

    return fs;
}

struct index_entry *add_index_entry(filesystem *fs, int type)
{
    struct index_entry *new_entry = (struct index_entry*)fs->index_region;

    fs->index_region -= sizeof(struct index_entry);
    fs->s_block->index_bytes += sizeof(struct index_entry);

    memcpy(fs->index_region, new_entry, sizeof(struct index_entry));

    new_entry->type = type;

    return new_entry;
}

filesystem *map_filesystem(int fd, superblock *s)
{
    uint32_t bytes_per_block = 1 << (s->block_size + 7);
    long long media_size = get_media_size(s);

    filesystem *fs = (filesystem*) calloc(1, sizeof (filesystem));
    if (fs == NULL) {
        perror("Allocating FS map");
        return NULL;
    }
 
    char *map = mmap(NULL, media_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        free(fs);
        perror("Error mmapping the file");
        return NULL;
    }
    
    fs->map = map;
    fs->s_block = (superblock*)(map + SUPERBLOCK_OFFSET);
    fs->data_region = map + (s->reserved_blocks * bytes_per_block);
    fs->index_region = map + (media_size - s->index_bytes);
    
    return fs;
}

int close_filesystem(filesystem *fs)
{
    munmap(fs->map, get_media_size(fs->s_block));
    return 1;
}
