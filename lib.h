#ifndef LIB_H
#define LIB_H

#include "list.h"
#include "load.h"
#include "link.h"

struct libinfo {
	struct loadinfo load;
	struct linkinfo link;

	struct list_head list;
};

void *load_library(const char *name, int flag);

#endif
