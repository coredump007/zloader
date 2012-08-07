#include <stdlib.h>
#include <limits.h>

#include <elf.h>

#include "list.h"
#include "debug.h"
#include "common.h"
#include "load.h"
#include "lib.h"

static LIST_HEAD(g_lib_list);

void *load_library(const char *name, int flag)
{
	struct libinfo *lib;

	char path[PATH_MAX];

	int r;

	D("Called.");

	r = check_init();
	if (r) {
		E("fail to check init.");
		return NULL;
	}

	lib = (struct libinfo *)calloc(1, sizeof(*lib));
	if (!lib) {
		E("fail to allocate memory.");
		return NULL;
	}

	r = locate_lib(name, path);
	if (r) {
		E("fail to locate library.");
		return NULL;
	}

	r = open_lib(lib, name);
	if (r) {
		E("fail to open library.");
		return NULL;
	}

	r = load_image(&lib->load);
	if (r) {
		E("fail to load library.");
		return NULL;
	}

	r = make_linkinfo(lib);
	if (r) {
		E("fail to make linkinfo.");
		return NULL;
	}

	r = link_image(&lib->link);
	if (r) {
		E("fail to link image.");
		return NULL;
	}

	INIT_LIST_HEAD(&lib->list);

	list_add_tail(&lib->list, &g_lib_list);

	return lib;
}
