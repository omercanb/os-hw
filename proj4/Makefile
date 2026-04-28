all: mfs make_mfs

mfs: mfs.c
	gcc -O2 -Wall -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=31 -c -o mfs.o mfs.c
	gcc mfs.o -O2 -Wall -lfuse3 -D_FILE_OFFSET_BITS=64  -DFUSE_USE_VERSION=31 -o mfs
	
make_mfs: make_mfs.c
	gcc -Wall -g  -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=31 make_mfs.c   -o make_mfs

clean:
	rm -f *~ mfs.o mfs make_mfs
	
