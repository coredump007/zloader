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
	return lib;
}
