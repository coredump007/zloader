#include <pthread.h>

#include "debug.h"
#include "common.h"
#include "lib.h"

#include "zloader.h"

static pthread_mutex_t dl_lock = PTHREAD_MUTEX_INITIALIZER;

void *zloader_dlopen(const char *filename, int flag)
{
	struct libinfo *lib;
	struct linkinfo *lki;

	D("Called.");

	pthread_mutex_lock(&dl_lock);
	lki = load_library(filename, flag);
	if (lki) {
		lib = container_of(lki, struct libinfo, link);
		init_library(lib);
	}
	pthread_mutex_unlock(&dl_lock);
	return lki;
}

void *zloader_dlsym(void *handle, const char *symbol)
{
	struct linkinfo *lki;

	Elf32_Sym *sym;
	unsigned bind;

	void *p = NULL;

	D("Called.");

	pthread_mutex_lock(&dl_lock);
	lki = (struct linkinfo *)handle;
	if (!check_magic(lki->magic)) {
		E("invaild handle.");
		goto out;
	}

	sym = sym_lookup(lki, symbol);
	if (sym) {
		D("Called.");
		bind = ELF32_ST_BIND(sym->st_info);

		if((bind == STB_GLOBAL) && (sym->st_shndx != 0)) {
			p = (void *)sym->st_value + lki->load_bias;
		}
	}

	D("Called.");
out:
	pthread_mutex_unlock(&dl_lock);
	return p;
}
