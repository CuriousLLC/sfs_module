#include "../common/sfs_kern.h"

static void sfs_put_super(struct super_block *sb) {
    unsigned char *index_region = get_index_region(sb);
    if (index_region!=NULL) {
        kfree(index_region);
    }
    
    if (SFS_SB(sb)!=NULL) {
        kfree(SFS_SB(sb));
    }

    printk(KERN_INFO "SFS super block destroyed\n");
}

static int sfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    superblock *s = dentry->d_sb->s_fs_info;
    
    buf->f_type = SFS_MAGIC_NUMBER;
    buf->f_bsize = 1 << (s->block_size + 7);
    buf->f_blocks = s->total_blocks;
    buf->f_bfree = s->total_blocks - s->reserved_blocks;
    buf->f_bavail = s->total_blocks - s->reserved_blocks;
    buf->f_namelen = 64;
    
    return 0;
}

const struct super_operations const sfs_super_ops = {
    .put_super  = sfs_put_super,
    .statfs     = sfs_statfs,
    .drop_inode = generic_delete_inode,
};
