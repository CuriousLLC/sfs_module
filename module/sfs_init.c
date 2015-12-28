// https://www.kernel.org/doc/Documentation/filesystems/vfs.txt
// http://www2.comp.ufscar.br/~helio/fs/rkfs.html
// http://www.ccs-labs.org/teaching/os/2014w/exercise-09.pdf

#include "../common/sfs_kern.h"

/**
 * Fill super is the callback for the generic mount_bdev(). This will
 * be called on filesystem registration.
 */
static int sfs_fill_super(struct super_block *sb, void *data, int silent) {
    superblock *sfs_sb;
    struct buffer_head *bh;
    struct inode *root = NULL;

    // Allocate our SFS superblock
    sfs_sb = kzalloc(sizeof (superblock), GFP_KERNEL);
    if (!sfs_sb) {
        return -ENOMEM;
    }

    sb->s_fs_info = sfs_sb;
    sb->s_magic = SFS_MAGIC_NUMBER;
    sb->s_op = &sfs_super_ops;

    // 512 is a hardcoded value here. This is configurable in the
    // filesystem however.
    if (!sb_set_blocksize(sb, 512)) {
        printk(KERN_ERR "device does not support %d byte blocks\n", 512);
        kfree(sfs_sb);
        return -EINVAL;
    }

    bh = sb_bread(sb, 0);
    printk(KERN_INFO "Read buffer from sb of size %zu\n", bh->b_size);
    
    memcpy(sfs_sb, bh->b_data + SUPERBLOCK_OFFSET, sizeof(superblock));
    if (sfs_sb->version != SFS_MAGIC_NUMBER) {
        kfree(sfs_sb);
        printk(KERN_ERR "Invalid magic in superblock: %x\n", sfs_sb->version);
        return -EINVAL;
    }

    root = sfs_get_inode(sb, S_IFDIR | 0755);
    if (!root) {
        kfree(sfs_sb);
        printk(KERN_ERR "inode allocation failed\n");
        return -ENOMEM;
    }

    root->i_fop = &sfs_dir_operations;
    root->i_op = &sfs_inode_operations;

    sb->s_root = d_make_root(root);
    if (!sb->s_root) {
        kfree(sfs_sb);
        printk(KERN_ERR "root creation failed\n");
        return -ENOMEM;
    }

    return 0;
}

/**
 * This function is called once the SFS filesystem is registered
 * with the kernel.
 */
static struct dentry *sfs_mount(struct file_system_type *type, int flags,
        char const *dev, void *data) 
{
    struct dentry * entry;

    printk(KERN_INFO "Request to mount %s\n", dev);
    
    entry = mount_bdev(type, flags, dev, data, sfs_fill_super);

    if (IS_ERR(entry))
        printk(KERN_ERR "SFS mounting failed\n");
    else
        printk(KERN_INFO "SFS mounted\n");

    return entry;
}

MODULE_ALIAS_FS("sfs");

static struct file_system_type sfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "sfs",
    .mount = sfs_mount,
    .kill_sb = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
    .next = NULL,
};


int init_module(void) {
    int err;

    err = register_filesystem(&sfs_fs_type);
    if (err) {
        printk(KERN_ERR "SFS: Error registering filesystem\n");
        return err;
    }

    printk(KERN_INFO "SFS: Loaded\n");
    return 0;
}

void cleanup_module(void) {
    unregister_filesystem(&sfs_fs_type);

    printk(KERN_INFO "SFS Filesystem removed\n");
}

MODULE_LICENSE("GPL");
