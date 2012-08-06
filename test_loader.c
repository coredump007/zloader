#include <dlfcn.h>
#include <stdio.h>

void (*foo1)(void);
void (*foo2)(void);

int main(void)
{
	void *h2;

	h2 = loader_dlopen("/home/inno/work/linker/libtest_loader.so", 0);
	if (!h2) {
		fprintf(stderr, "fail to open so with loader_dlopen.\n");
		return -1;
	}

	return 0;
}
