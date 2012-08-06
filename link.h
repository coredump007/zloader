#ifndef LINK_H
#define LINK_H

#include "common.h"
#include "list.h"

typedef void (*sc_func_t)(void);
typedef void *(load_lib_func_t)(const char *, int);

typedef struct {
	int refcount;

	Elf32_Phdr phdr[N_MAX_PHDR];
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

	load_lib_func_t *load_lib;

	int constructed;

	struct list_head list;
}soinfo_t;

#endif
