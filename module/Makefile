obj-m := sfs_mod.o
sfs_mod-objs := sfs_init.o sfs_super.o sfs_root.o sfs_inode.o

KDIR=/lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
