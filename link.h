#ifndef LINK_H
#define LINK_H

#include "common.h"

struct load_lib_data {
	void *(*load_lib_func)(const char *, int);
	int flag;
};

extern struct load_lib_data *g_load_lib_data;

typedef void (*sc_func_t)(void);

struct linkinfo {
	int refcount;

	Elf32_Phdr *phdr[N_MAX_PHDR];
	Elf32_Word n_phdr;

	void *load_start;
	Elf32_Word load_size;
	Elf32_Word load_bias;

	Elf32_Dyn *dyn_section;

	Elf32_Word *hash_table;
	Elf32_Word nbucket;
	Elf32_Word nchain;
	Elf32_Word *bucket;
	Elf32_Word *chain;

	void *strtab;
	Elf32_Sym *symtab;

	Elf32_Word n_plt_rel;
	Elf32_Rel *plt_rel;

	Elf32_Word n_rel;
	Elf32_Rel *rel;

	sc_func_t *init_func;
	sc_func_t *fini_func;

	Elf32_Word n_init_func;
	sc_func_t *init_func_array;

	Elf32_Word n_fini_func;
	sc_func_t *fini_func_array;

	Elf32_Word n_preinit_func;
	sc_func_t *preinit_func_array;

	int constructed;
};

#endif
