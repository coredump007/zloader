#include <dlfcn.h>
#include <stdio.h>

#include "zloader.h"

int (*foo_func)(int, int);

int main(void)
{
	void *h2;

	int r;

	h2 = zloader_dlopen("libtest_loader.so", 0);
	if (!h2) {
		fprintf(stderr, "fail to open so with loader_dlopen.\n");
		return -1;
	}

	foo_func = zloader_dlsym(h2, "foo");

	fprintf(stderr, "Func: %p.\n", foo_func);

	r = foo_func(1, 1);

	fprintf(stderr, "Result: foo1: %d.\n", r);

	return 0;
}
