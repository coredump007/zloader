CC := gcc
CFLAGS := -g --shared -fPIC -Wl,--hash-style=sysv
.PHONY: all build clean

all: build

build: libloader.so libtest_loader.so test_loader

libloader.so: loader.c link.c link_dyn.c link_reloc.c debug.c
	$(CC) $(CFLAGS) -o $@ $?

libtest_loader.so: test_loader_so.c
	$(CC) $(CFLAGS) -o $@ $?

test_loader: test_loader.c
	$(CC) -g -o $@ $< -L. -lloader

clean:
	rm -rf *.o libloader.so test_loader libtest_loader.so
