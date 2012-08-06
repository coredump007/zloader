CC := gcc
CFLAGS := -g --shared -fPIC -Wl,--hash-style=sysv
.PHONY: all build clean

all: build

build: libzloader.so libtest_loader.so test_loader

libzloader.so: zloader.c link.c link_dyn.c link_reloc.c debug.c lib.c
	$(CC) $(CFLAGS) -o $@ $?

libtest_loader.so: test_loader_so.c
	$(CC) $(CFLAGS) -o $@ $?

test_loader: test_loader.c
	$(CC) -g -o $@ $< -L. -lzloader

clean:
	rm -rf *.o libzloader.so test_loader libtest_loader.so
