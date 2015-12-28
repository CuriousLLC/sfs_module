# SFS Toy Filesystem

This repo contains a Linux kernel module, and some userland tools, to build a
[Simple File System](https://www.d-rift.nl/combuster/vdisk/sfs.html). The Linux module
is a small, read only, implementation. It is simply an excersize to learn more about the Linux VFS
and how to implement a file system.

Creation time is stored when the filesystem is created. Access times are updated when the filesystem
is mounted. SFS doesn't support permissions, so directories are 755 and files are 644.

The userland tools allow you to create an image with some default directories and files. 

The kernel module will list all entries in the root directory.

```bash
make
sudo insmod module/sfs_mod.ko
cli/mksfs -c -f test.img
mkdir test_dir
sudo mount -o loop -t sfs test.img test_dir/
ls test_dir/
cat test_dir/second_file
sudo umount test_dir
sudo rmmod sfs_mod
```
