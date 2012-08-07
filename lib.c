#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <elf.h>

#include "list.h"
#include "debug.h"
#include "common.h"
#include "load.h"
#include "lib.h"

static struct load_lib_data local_load_lib_data = {
	.load_lib_func = load_library,
	.flag = 0,
};

#define N_MAX_LDPATH 8

struct ldpath_data {
	char *ldpath[N_MAX_LDPATH + 1];
	char *ldpath_string;
};

static struct ldpath_data g_ldpath_data;

static char *sopath[] = {
	"/usr/lib",
	NULL,
};

static LIST_HEAD(g_lib_list);

static int check_elf_header(const Elf32_Ehdr* hdr)
{
	D("Called.");

	if (hdr->e_ident[EI_MAG0] != ELFMAG0) return -1;
	if (hdr->e_ident[EI_MAG1] != ELFMAG1) return -1;
	if (hdr->e_ident[EI_MAG2] != ELFMAG2) return -1;
	if (hdr->e_ident[EI_MAG3] != ELFMAG3) return -1;

	D("Called.");

	return 0;
}

static int open_lib(struct libinfo *lib, const char *name)
{
	struct loadinfo *ldi = &lib->load;
	Elf32_Ehdr *ehdr = &ldi->ehdr;

	unsigned long s;
	int r;

	D("Called.");

	memset(ldi, 0, sizeof(*ldi));

	ldi->fd = open(name, O_RDONLY);
	if (ldi->fd == -1) {
		E("fail to open file.");

		return -1;
	}

	D("Called.");

	do {
		r = read(ldi->fd, ehdr, sizeof(*ehdr));
	}while (r == -1 && errno == -EINTR);

	if (r != sizeof(*ehdr)) {
		E("fail to read ELF header.");

		goto fail;
	}

	D("Called.");

	if (check_elf_header(ehdr) < 0) {
		E("not an ELF file.");

		goto fail;
	}

	if (ehdr->e_phnum > N_MAX_PHDR) {
		E("Can not support more than %d PHDR.", N_MAX_PHDR);

		goto fail;
	}

	lseek(ldi->fd, SEEK_SET, ehdr->e_phoff);

	do {
		r = read(ldi->fd, ldi->phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));
	}while (r == -1 && errno == -EINTR);

	if (r != (ehdr->e_phnum * sizeof(Elf32_Phdr))) {
		E("Can not support more than %d PHDR.", N_MAX_PHDR);
		goto fail;
	}

	return 0;

fail:
	if (ldi->fd != -1)
		close(ldi->fd);

	return -1;
}

static void close_lib(struct libinfo *lib)
{
	struct loadinfo *ldi = &lib->load;

	D("Called.");

	close(ldi->fd);

	memset(ldi, 0, sizeof(*ldi));

	return;
}

static int make_linkinfo(struct libinfo *lib)
{
	struct linkinfo *lki = &lib->link;
	struct loadinfo *ldi = &lib->load;

	int i;

	D("Called.");

	lki->load_start = ldi->load_start;
	lki->load_bias = ldi->load_bias;
	lki->load_size = ldi->load_size;
	lki->n_phdr = ldi->ehdr.e_phnum;
	lki->constructed = 0;

	for (i = 0; i < lki->n_phdr; i++)
		lki->phdr[i] = ldi->phdr + i;

	return 0;
}

static inline int check_init(void)
{
	struct ldpath_data *lp = &g_ldpath_data;

	char *s, *p;
	int i;

	if (!g_load_lib_data)
		g_load_lib_data = &local_load_lib_data;

	if (!lp->ldpath_string) {
		s = getenv("LD_LIBRARY_PATH");

		lp->ldpath_string = strdup(s);
		if (!lp->ldpath_string) {
			E("fail to allocate memory.");
			return -1;
		}

		s = p = lp->ldpath_string;

		for (i = 0; i < N_MAX_LDPATH; i++) {
			s = strchr(p, ':');
			lp->ldpath[i] = p;

			if (!s)
				break;

			*s = '\0';
			s = s + 1;
			p = s;
		}

		if (D_TAG(T_L_LDPATH)) {
			DT(T_L_LDPATH, "LD_LIBRARY_PATH:");

			for (i = 0; lp->ldpath[i] != NULL; i++)
				DT(T_L_LDPATH, "%s", lp->ldpath[i]);
		}
	}

	return;
}

void *load_library(const char *name, int flag)
{
	struct libinfo *lib;

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

	r = open_lib(lib, name);
	if (r == -1) {
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


	local_load_lib_data.flag = flag;

	r = link_image(&lib->link);
	if (r) {
		E("fail to link image.");
		return NULL;
	}

	INIT_LIST_HEAD(&lib->list);

	list_add_tail(&lib->list, &g_lib_list);

	return lib;
}
