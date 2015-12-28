#include "sfs.h"

struct index_entry *find_directory(filesystem *fs, char *dname)
{
	struct index_entry *ptr = (struct index_entry *)fs->index_region;

	for(int i=0; i<fs->s_block->index_bytes / 64; i++, ptr++) {
		if (ptr->type == DIRECTORY_ENTRY) {
			if (strcmp(ptr->dir.dir_name, dname)==0) {
				return ptr;
			}
		}
	}

	return NULL;
}

struct index_entry *find_file(filesystem *fs, char *fname)
{
	struct index_entry *ptr = (struct index_entry *)fs->index_region;

	for(int i=0; i<fs->s_block->index_bytes / 64; i++, ptr++) {
		if (ptr->type == FILE_ENTRY) {
			if (strcmp(ptr->file.file_name, fname)==0) {
				return ptr;
			}
		}
	}

	return NULL;
}


int add_directory(filesystem *fs, char *dname)
{
	if (find_directory(fs, dname)!=NULL) {
		return -1;
	}

	add_index_entry(fs, DIRECTORY_ENTRY);
	return 0;
}

int add_file(filesystem *fs, char *fname, long long size)
{
	uint32_t bytes_per_block = 1 << (s->block_size + 7);

	if (find_file(fs, fname)!=NULL) {
		return -1;
	}

	struct index_entry *marker = (struct index_entry *)fs->index_region;
	struct index_entry *entry = add_index_entry(fs, FILE_ENTRY);
	entry->starting_block = marker->first_entry.next_starting_block;
	entry->ending_block = entry->starting_block + (size / bytes_per_block) + 1;
}

int write_file(filesystem *fs, index_entry *entry, char *data, long long len)
{
	uint32_t bytes_per_block = 1 << (s->block_size + 7);

	char *dest = (char*)(fs->data_region + (entry->file.starting_block * bytes_per_block));
	memcpy(dest, data, len);
}

int read_file(filesystem *fs, index_entry *entry, char *buf, long long bytes)
{
	char *src = (char*)(fs->data_region + (entry->file.starting_block * bytes_per_block));
	memcpy(buf, src, bytes);
	return 0;
}
