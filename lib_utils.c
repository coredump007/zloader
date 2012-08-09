#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

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

static char *g_sopath[] = {
	"/lib",
	"/usr/lib",
	"/system/lib",
	NULL,
};

static int init_ldpath(void)
{
	struct ldpath_data *lp = &g_ldpath_data;

	char *s, *p;
	int i;

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

	return 0;
}

int check_init(void)
{
	struct ldpath_data *lp = &g_ldpath_data;

	int r;

	if (!g_load_lib_data)
		g_load_lib_data = &local_load_lib_data;

	if (!lp->ldpath_string) {
		r = init_ldpath();
		if (r) {
			return -1;
		}
	}

	return 0;
}

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

struct libinfo *alloc_lib(void)
{
	struct libinfo *lib;

	lib = (struct libinfo *)calloc(1, sizeof(*lib));
	if (!lib) {
		E("fail to allocate memory.");
		return NULL;
	}

	lib->magic = LOADER_MAGIC;
	lib->refcount = 0;

	INIT_LIST_HEAD(&lib->list);

	return lib;
}

void free_lib(struct libinfo *lib)
{
	int r;

	if (!check_magic(lib->magic)) {
		E("not a libinfo handle.");
		return;
	}

	if (lib->refcount) {
		E("free libinfo with refcount.");
		return;
	}

	free(lib);
	lib = NULL;

	return;
}

int locate_lib(const char *name, char *path)
{
	struct ldpath_data *lp = &g_ldpath_data;

	struct stat st;

	char buf[PATH_MAX];
	char *p;
	int i;

	buf[0] = '\0';

	if (strchr(name, '/')) {
		strncpy(path, name, PATH_MAX);

		D("locate path: %s", path);

		return 0;
	}

	for (i = 0; lp->ldpath[i] != NULL; i++) {
		strncpy(buf, lp->ldpath[i], PATH_MAX);
		strcat(buf, "/");
		strcat(buf, name);

		D("locate buf: %s", buf);

		if (!lstat(buf, &st) && S_ISREG(st.st_mode)) {
			strncpy(path, buf, PATH_MAX);

			D("locate path: %s", path);

			return 0;
		}
	}

	for (i = 0; g_sopath[i] != NULL; i++) {
		strncpy(buf, g_sopath[i], PATH_MAX);
		strcat(buf, "/");
		strcat(buf, name);

		D("locate buf: %s", buf);

		if (!lstat(buf, &st) && S_ISREG(st.st_mode)) {
			strncpy(path, buf, PATH_MAX);

			D("locate path: %s", path);

			return 0;
		}
	}

	return -1;
}

int open_lib(struct libinfo *lib, const char *name)
{
	struct loadinfo *ldi = &lib->load;
	Elf32_Ehdr *ehdr = &ldi->ehdr;

	unsigned long s;
	int r;

	D("Called.");

	memset(ldi, 0, sizeof(*ldi));

	ldi->magic = LOADER_MAGIC;

	ldi->fd = open(name, O_RDONLY);
	if (ldi->fd == -1) {
		E("fail to open file.");
		perror("open():");
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

void close_lib(struct libinfo *lib)
{
	struct loadinfo *ldi = &lib->load;

	D("Called.");

	close(ldi->fd);

	memset(ldi, 0, sizeof(*ldi));

	return;
}

int setup_linkinfo(struct libinfo *lib)
{
	struct linkinfo *lki = &lib->link;
	struct loadinfo *ldi = &lib->load;

	int i;

	D("Called.");

	INIT_LIST_HEAD(&lki->dt_needed_head);
	INIT_LIST_HEAD(&lki->dt_needed_list);

	lki->magic = LOADER_MAGIC;
	lki->load_start = ldi->load_start;
	lki->load_bias = ldi->load_bias;
	lki->load_size = ldi->load_size;
	lki->n_phdr = ldi->ehdr.e_phnum;
	lki->b_constructed = 0;

	for (i = 0; i < lki->n_phdr; i++)
		lki->phdr[i] = ldi->phdr + i;

	return 0;
}
