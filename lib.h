#ifndef LIB_H
#define LIB_H

#include "load.h"
#include "link.h"

struct libinfo {
	struct loadinfo load;
	struct linkinfo link;
};

struct libinfo *load_library(const char *name, int flag);

#endif
