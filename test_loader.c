#include <dlfcn.h>
#include <stdio.h>

#include "zloader.h"

int main(void)
{
	void *h2;

	h2 = zloader_dlopen("libtest_loader.so", 0);
	if (!h2) {
		fprintf(stderr, "fail to open so with loader_dlopen.\n");
		return -1;
	}

	return 0;
}
