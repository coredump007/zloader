#include <pthread.h>

#include "debug.h"
#include "common.h"
#include "link.h"
#include "lib.h"

#include "zloader.h"

static pthread_mutex_t dl_lock = PTHREAD_MUTEX_INITIALIZER;

void *zloader_dlopen(const char *filename, int flag)
{
	soinfo_t *si;

	D("Called.");

	pthread_mutex_lock(&dl_lock);
	si = load_library(filename, flag);
	pthread_mutex_unlock(&dl_lock);
	return si;
}
