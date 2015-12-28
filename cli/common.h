/* 
 * File:   common.h
 * Author: rday
 *
 * Created on December 21, 2015, 6:46 PM
 */

#ifndef COMMON_H
#define	COMMON_H

#define _POSIX_C_SOURCE 200809L

#include "../common/sfs.h"

// Definitions for functions related to userspace mapping of a filesystem
filesystem *open_filesystem(char *fname);
filesystem *create_filesystem(int fd, superblock *s);
filesystem *map_filesystem(int fd, superblock *s);
int close_filesystem(filesystem *fs);

// Definitions for functions that can read/write a userspace filesystem
struct index_entry *add_index_entry(filesystem *fs, int type);
struct index_entry *find_directory(filesystem *fs, char *dname);
struct index_entry *find_file(filesystem *fs, char *fname);
int add_directory(filesystem *fs, char *dname);
int add_file(filesystem *fs, char *fname, long long size);
int write_file(filesystem *fs, index_entry *entry, char *data, long long len);
int read_file(filesystem *fs, index_entry *entry, char *buf, long long bytes);

// Helper functions
uint8_t superblock_calc_checksum(superblock *s);
long long get_milliseconds();
long long get_media_size(superblock *s);

#endif	/* COMMON_H */

