#include "../common/sfs_kern.h"

// get_index_region will locate the blocks containing the
// entire SFS index. Those blocks will be copied into a 
// buffer and returned.
static unsigned char *cached_index_region = NULL;

/**
 * get_index_region will return the copy of the index cached in
 * memory. If the index hasn't been cached yet, the index will
 * be read and stored, then returned.
 */
unsigned char *get_index_region(struct super_block *sb)
{
    superblock *s = sb->s_fs_info;
    struct buffer_head *bh;
    uint32_t bytes_per_block = 1 << (s->block_size + 7);
    sector_t index_block = ((s->total_blocks * bytes_per_block) - s->index_bytes) / bytes_per_block;
    unsigned int index_offset;
    unsigned int index_blocks = (s->index_bytes / bytes_per_block) + 1;
    uint8_t i;

    if (cached_index_region==NULL) {
        // If we haven't created the region yet, then allocate it
        // and load it from disk.
        unsigned char *index_region = (char*)kzalloc(s->index_bytes, GFP_KERNEL);
        unsigned char *ptr = index_region;

        if (index_region==NULL) {
            return NULL;
        }

        if (s->index_bytes <= bytes_per_block) {
             index_offset = bytes_per_block - s->index_bytes;
        } else {
             index_offset = bytes_per_block - (s->index_bytes - bytes_per_block);
        }

        for (i = 0; i < index_blocks; i++) {
            bh = sb_bread(sb, index_block + i);
            if (i == 0) {
                // We want to copy to the beginning of the index_region memory, 
                // but our actual index is probably somewhere in the middle of
                // the block we read. So copy from the beginning of the index,
                // not the whole block. This only matters for the first block.
                memcpy(ptr, bh->b_data + index_offset, bytes_per_block - index_offset);
                ptr += (bytes_per_block - index_offset);
            } else {
                // If this is a subsequent block holding our index, then we know
                // the entire block contains index data.
                memcpy(ptr, bh->b_data, bytes_per_block);
                ptr += bytes_per_block;
            }
            brelse(bh);
        }
        
        cached_index_region = index_region;
    }

    return cached_index_region;
}

/**
 * get_entry_by_name will locate an index entry by file|directory
 * name and return it.
 */
index_entry *get_entry_by_name(struct super_block *sb, const unsigned char *name)
{
    uint8_t i;
    char *index_region;
    struct index_entry *ientry;

    index_region = get_index_region(sb);
    if (index_region==NULL) {
        printk(KERN_ERR "SFS: Could not find index region\n");
        return 0;
    }

    for (i=0; i<(SFS_SB(sb)->index_bytes / INDEX_ENTRY_SIZE)+1;i++) {
        ientry = (struct index_entry *)(index_region + (i * INDEX_ENTRY_SIZE));

        if (ientry->type == VOLUME_ID_ENTRY) {
            break;
        } else if (ientry->type == DIRECTORY_ENTRY) {
            if (strcmp(ientry->dir.dir_name, name)==0) {
                return ientry;
            }
        } else if (ientry->type == FILE_ENTRY) {
            if (strcmp(ientry->file.file_name, name)==0) {
                return ientry;
            }
        }

    }

    return NULL;
}

/**
 * sfs_read will read a file and copy data into userspace. This is
 * called when a file is open(2)'d and read(2) from.
 */
ssize_t sfs_read(struct file * filp, char __user * buf, size_t len,
		      loff_t * ppos)
{
    struct buffer_head *bh;
    struct index_entry *entry;
    struct super_block *sb = filp->f_inode->i_sb;
    superblock *s = SFS_SB(sb);

    entry = get_entry_by_name(sb, filp->f_path.dentry->d_name.name);
    if (entry==NULL) {
        printk(KERN_ERR "SFS: Could not find entry for %s\n", filp->f_path.dentry->d_name.name);
        return -EINVAL;
    }

    // Update the inode's last access time.
    filp->f_inode->i_atime = CURRENT_TIME;

    if (*ppos >= entry->file.length) {
        return 0;
    }

    if (*ppos + len > entry->file.length) {
        len = entry->file.length - *ppos;
    }

    bh = sb_bread(sb, entry->file.starting_block + s->reserved_blocks);
    if (bh==NULL) {
        printk(KERN_ERR "SFS: Error reading %s\n", filp->f_path.dentry->d_name.name);
        return -EFAULT;
    }
    
    if (copy_to_user(buf, (char*)bh->b_data, len) > 0) {
        printk(KERN_ERR "SFS: Error writing bytes to buffer\n");
        brelse(bh);
        return -EFAULT;
    }

    brelse(bh);

    *ppos += min((size_t)entry->file.length, len);

    return min((size_t)entry->file.length, len);
}

/**
 * Iterate over a directory and emit each entry. The first time this runs,
 * each file will have an inode looked up.
 * We are working with a flat directory structure. So we always iterate the
 * root inode. Even if you list a subdirectory, we emit the root listing.
 */
static int sfs_read_dir(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);
    char *index_region;
    struct index_entry *dir_entry;

    if (ctx->pos<2) {
        // Position 0 and 1 will be . and ..
        if (!dir_emit_dots(file, ctx)) {
            return 0;
        }

        return 0;
    }

    index_region = get_index_region(inode->i_sb);
    if (index_region==NULL) {
        printk(KERN_ERR "SFS: Could not find index region\n");
        return 0;
    }

    // Find the next viable entry, or the final entry (volume_id_entry). This
    // skips over any deleted records.
    while (1) {
        dir_entry = (struct index_entry *)(index_region + ((ctx->pos-2) * INDEX_ENTRY_SIZE));
        if (dir_entry->type == DIRECTORY_ENTRY) {
            if (!dir_emit(ctx, dir_entry->dir.dir_name, 
                    strlen(dir_entry->dir.dir_name), inode->i_ino, DT_DIR)) {
                return 0;
            }
        } else if (dir_entry->type == FILE_ENTRY) {
            if (!dir_emit(ctx, dir_entry->file.file_name, 
                    strlen(dir_entry->file.file_name), inode->i_ino, DT_REG)) {
                return 0;
            }
        }else if (dir_entry->type == VOLUME_ID_ENTRY) {
            break;
        }

        ctx->pos++;
    }

    return 0;
}

const struct file_operations sfs_dir_operations = {
    .open = dcache_dir_open,
    .release = dcache_dir_close,
    .read = generic_read_dir,
    .iterate = sfs_read_dir,
    .llseek = dcache_dir_lseek,
};

const struct file_operations sfs_file_operations = {
    .read = sfs_read,
};
