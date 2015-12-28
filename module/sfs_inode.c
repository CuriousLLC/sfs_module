#include <linux/version.h>

#include "../common/sfs_kern.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
static inline struct inode *d_inode(const struct dentry *dentry);
/**
 * d_inode - Get the actual inode of this dentry
 * @dentry: The dentry to query
 *
 * This is the helper normal filesystems should use to get at their own inodes
 * in their own dentries and ignore the layering superimposed upon them.
 */
static inline struct inode *d_inode(const struct dentry *dentry)
{
    return dentry->d_inode;
}
#endif


static void milli_to_timespec(unsigned long long ms, struct timespec *ts)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms - (ts->tv_sec * 1000)) * 1000000;
}

/**
 * When an inode is first looked up, this function is called. The index
 * is searched for a matching entry, and an inode is created for that
 * entry.
 * @param dir Parent inode
 * @param entry Child entry
 * @param flags
 * @return 
 */
static struct dentry *sfs_inode_lookup(struct inode *dir, struct dentry *entry, unsigned int flags)
{
    uint8_t i;
    unsigned char *index_region;
    struct inode *new_inode=NULL;
    struct index_entry *ientry;

    index_region = get_index_region(dir->i_sb);
    if (index_region==NULL) {
        printk(KERN_ERR "SFS: Could not find index region\n");
        return 0;
    }

    // Find a matching entry in our index. Populate as much info as we can
    // about that entry. SFS doesn't have support for permissons in the
    // spec, so we are rather limited.
    for (i=0; i<(SFS_SB(dir->i_sb)->index_bytes / INDEX_ENTRY_SIZE)+1;i++) {
        ientry = (struct index_entry *)(index_region + (i * INDEX_ENTRY_SIZE));

        if (ientry->type == DIRECTORY_ENTRY) {
            if (strcmp(ientry->dir.dir_name, entry->d_name.name)==0) {
                new_inode = sfs_get_inode(dir->i_sb, S_IFDIR | 0755);
                milli_to_timespec(ientry->dir.timestamp, &new_inode->i_mtime);
                milli_to_timespec(ientry->dir.timestamp, &new_inode->i_ctime);
                break;
            }
        }

        if (ientry->type == FILE_ENTRY) {
            if (strcmp(ientry->file.file_name, entry->d_name.name)==0) {
                new_inode = sfs_get_inode(dir->i_sb, S_IFREG | 0644);
                new_inode->i_size = ientry->file.length;
                milli_to_timespec(ientry->file.timestamp, &new_inode->i_mtime);
                milli_to_timespec(ientry->file.timestamp, &new_inode->i_ctime);
                break;
            }
        }

        if (ientry->type == VOLUME_ID_ENTRY) {
            break;
        }
    }

    if (new_inode==NULL) {
        printk(KERN_ERR "SFS: Could not find entry for %s\n", entry->d_name.name);
    } else {
        new_inode->i_atime = CURRENT_TIME;
        d_add(entry, new_inode);
    }

    return NULL;
}

/**
 * Create a new inode and assign the default attributes
 */
struct inode *sfs_get_inode(struct super_block *sb, umode_t mode)
{
    struct inode *inode = new_inode(sb);
    
    if (inode==NULL) {
        return NULL;
    }
    
    inode->i_mode = mode;
    if (sb->s_root) {
        inode->i_uid = d_inode(sb->s_root)->i_uid;
        inode->i_gid = d_inode(sb->s_root)->i_gid;
    }
    
    inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
    inode->i_ino = get_next_ino();
    inode->i_op = &sfs_inode_operations;

    if (S_ISDIR(mode)) {
        set_nlink(inode, 2);
        inode->i_fop = &sfs_dir_operations;
    } else if (S_ISREG(mode)) {
        inode->i_fop = &sfs_file_operations;
    }

    return inode;
}

const struct inode_operations sfs_inode_operations = {
    .lookup = sfs_inode_lookup,
};
