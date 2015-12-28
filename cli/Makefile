CC=gcc
CFLAGS=-std=c99 -Wall -g -ggdb
TARGET=mksfs

default: all

all: $(TARGET)

$(TARGET): main.o common.o sfs.o
	$(CC) common.o sfs.o main.o -o $(TARGET) $(CFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

sfs.o: sfs.c ../common/sfs.h
	$(CC) $(CFLAGS) -c sfs.c

clean:
	rm -rf *.o mksfs
