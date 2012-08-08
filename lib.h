#ifndef LIB_H
#define LIB_H

#include "list.h"
#include "load.h"
#include "link.h"

struct libinfo {
	unsigned long magic;

	int refcount;

	struct loadinfo load;
	struct linkinfo link;

	struct list_head list;
};

int check_init(void);

struct libinfo *alloc_lib(void);

int check_lib(struct libinfo *lib);

void free_lib(struct libinfo *lib);

int locate_lib(const char *name, char *path);

int open_lib(struct libinfo *lib, const char *name);

void close_lib(struct libinfo *lib);

int make_linkinfo(struct libinfo *lib);

void *load_library(const char *name, int flag);

#endif
