/* 
 * File:   sfskern.h
 * Author: rday
 *
 * Created on December 21, 2015, 6:49 PM
 */

#ifndef SFSKERN_H
#define	SFSKERN_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/statfs.h>

#include "sfs.h"

extern const struct inode_operations sfs_inode_operations;
extern const struct file_operations sfs_dir_operations;
extern const struct file_operations sfs_file_operations;
extern const struct super_operations sfs_super_ops;

struct inode *sfs_get_inode(struct super_block *sb, umode_t mode);
unsigned char *get_index_region(struct super_block *sb);
index_entry *get_entry_by_name(struct super_block *sb, const unsigned char *name);

static inline struct superblock *SFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

#endif

