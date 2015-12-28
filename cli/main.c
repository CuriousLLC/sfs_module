#include <stdio.h>
#include <inttypes.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include "common.h"
#include "../common/sfs.h"


int create_fs(char *fname)
{
    int fd;
    superblock s;
    filesystem *fs;
    
    memset(&s, 0, sizeof(superblock));
    s.block_size = 2;
    s.total_blocks = 100;
    s.data_blocks = 80;

    fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        perror("File open");
        return -1;
    }

    fs = create_filesystem(fd, &s);
    if (fs == NULL) {
        close(fd);
        return -1;
    }
    close_filesystem(fs);
    
    return 1;
}

int open_fs(char *fname)
{
    filesystem *fs;
    superblock *s;
    
    fs = open_filesystem(fname);
    if (fs == NULL) {
        exit(1);
    }

    s = fs->s_block;
    struct index_entry *vei_ptr = (struct index_entry *)(fs->index_region + (s->index_bytes - sizeof(struct index_entry)));
    long long create_time = vei_ptr->volume_id.timestamp / 1000;
    struct timeval tv;
    tv.tv_sec = create_time;

    char fmt[64];
    struct tm *tm = localtime(&tv.tv_sec);
    strftime(fmt, sizeof(fmt), "%Y-%m-%d %H:%M:%S", tm);
    printf("Opened filesystem ID %s\n", vei_ptr->volume_id.volume_name);
    printf("Created at %s\n", fmt);
    close_filesystem(fs);
    
    return 0;
}

int main(int argc, char **argv)
{
    int c;
    int create_flag = 0;
    int open_flag = 0;
    char *fname = NULL;

    while ((c = getopt(argc, argv, "cof:")) != -1) {
        switch (c) {
            case 'c':
                create_flag = 1;
                break;
            case 'o':
                open_flag = 1;
                break;
            case 'f':
                fname = optarg;
                break;
            default:
                abort();
        }
    }
    
    if (create_flag && open_flag) {
        printf("Can only open, or create new. Can't choose both!\n");
        exit(1);
    }
    
    if (fname == NULL) {
        printf("Please specify a filename with -f\n");
        exit(1);
    }

    if (create_flag) {
        create_fs(fname);
        exit(1);
    }
    
    if (open_flag) {
        open_fs(fname);
        exit(1);
    }
}
