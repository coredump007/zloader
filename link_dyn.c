#include <stdlib.h>

#include "debug.h"
#include "common.h"
#include "link.h"

static char *g_excluded_lib_list[] = {
	"libc.so.6",
	NULL,
};

static int is_excluded_lib(const char *name)
{
	char *p = g_excluded_lib_list[0];
	int i;

	for (i = 0; *p; i++, p++) {
		if (!strcmp(p, name))
			return 1;
	}

	return 0;
}

int process_dyn_section(struct linkinfo *lki)
{
	struct load_lib_data *lld = g_load_lib_data;

	const Elf32_Phdr* phdr;
	const Elf32_Dyn *d;

	struct linkinfo *l;

	char *p;
	int r;
	int i;

	lki->dyn_section = NULL;

	for (i = 0; i < lki->n_phdr; i++) {
		phdr = lki->phdr[i];
		if (phdr->p_type == PT_DYNAMIC) {
			lki->dyn_section = (Elf32_Dyn *)(lki->load_bias + phdr->p_vaddr);
			break;
		}
	}

	if (!lki->dyn_section) {
		E("fail to get dynamic section.");
		return -1;
	}

	for (d = lki->dyn_section; d->d_tag; d++) {
		switch (d->d_tag) {
			case DT_NEEDED:
				lki->n_dt_needed++;
				break;

			case DT_HASH:
				D("DT_HASH, d_ptr: 0x%x.", d->d_un.d_ptr);

				lki->hash_table = (Elf32_Word *)(lki->load_bias + d->d_un.d_ptr);
				lki->nbucket = lki->hash_table[0];
				lki->nchain = lki->hash_table[1];
				lki->bucket = (Elf32_Word *)lki->hash_table + 2;
				lki->chain = lki->bucket + lki->nbucket;

				break;

			case DT_STRTAB:
				D("DT_STRTAB.");

				lki->strtab = (void *)(lki->load_bias + d->d_un.d_ptr);

				break;

			case DT_SYMTAB:
				D("DT_SYMTAB.");

				lki->symtab = (Elf32_Sym *)(lki->load_bias + d->d_un.d_ptr);

				break;

			case DT_PLTREL:
				D("DT_PLTREL.");

				if(d->d_un.d_val != DT_REL) {
					E("DT_RELA not supported");
					return -1;
				}

				break;

			case DT_JMPREL:
				D("DT_JMPREL.");

				lki->plt_rel = (Elf32_Rel*)(lki->load_bias + d->d_un.d_ptr);

				break;

			case DT_PLTRELSZ:
				D("DT_PLTRELSZ.");

				lki->n_plt_rel = d->d_un.d_val / sizeof(Elf32_Rel);

				break;

			case DT_REL:
				D("DT_REL.");

				lki->rel = (Elf32_Rel*)(lki->load_bias + d->d_un.d_ptr);

				break;

			case DT_RELSZ:
				D("DT_RELSZ.");

				lki->n_rel = d->d_un.d_val / sizeof(Elf32_Rel);

				break;

			case DT_PLTGOT:
				D("DT_PLTGOT.");

				break;

			case DT_RELA:
				D("DT_RELA.");

				break;

			case DT_INIT:
				D("DT_INIT.");

				lki->init_func = (sc_func_t)(lki->load_bias + d->d_un.d_ptr);
				break;

			case DT_FINI:
				D("DT_FINI.");

				lki->fini_func = (sc_func_t)(lki->load_bias + d->d_un.d_ptr);
				break;

			case DT_INIT_ARRAY:
				D("DT_INIT_ARRAY.");

				lki->init_func_array = (sc_func_t *)(lki->load_bias + d->d_un.d_ptr);
				break;

			case DT_INIT_ARRAYSZ:
				D("DT_INIT_ARRAYSZ.");

				lki->n_init_func = (d->d_un.d_val) / sizeof(Elf32_Addr);
				break;

			case DT_FINI_ARRAY:
				D("DT_FINI_ARRAY.");

				lki->fini_func_array = (sc_func_t *)(lki->load_bias + d->d_un.d_ptr);
				break;

			case DT_FINI_ARRAYSZ:
				D("DT_FINI_ARRAYSZ.");

				lki->n_fini_func = (d->d_un.d_val) / sizeof(Elf32_Addr);
				break;

			case DT_PREINIT_ARRAY:
				D("DT_PREINIT_ARRAY.");

				lki->preinit_func_array = (sc_func_t *)(lki->load_bias + d->d_un.d_ptr);
				break;

			case DT_PREINIT_ARRAYSZ:
				D("DT_PREINIT_ARRAYSZ.");

				lki->n_preinit_func = (d->d_un.d_val) / sizeof(Elf32_Addr);
				break;

			case DT_TEXTREL:
				D("DT_TEXTREL.");

				break;
		}
	}

	if (lki->n_dt_needed) {
		D("n_dt_needed: %lu", lki->n_dt_needed);

		for (i = 0, d = lki->dyn_section; d->d_tag; d++) {
			if (d->d_tag == DT_NEEDED) {
				if (!lld) {
					E("load_lib_data is not initialized.");
					return -1;
				}

				p = lki->strtab + d->d_un.d_ptr;

				if (is_excluded_lib(p))
					continue;

				l = lld->load_lib_func(p, lld->flag);
				if (!l) {
					E("fail to call load_lib_func.");
					return -1;
				}

				list_add_tail(&l->dt_needed_list, &lki->dt_needed_head);

				i++;
			}
		}

		lki->n_dt_needed = i;

		D("n_dt_needed loaded: %lu", lki->n_dt_needed);
	}

	return 0;
}
