#include "debug.h"
#include "common.h"
#include "link.h"

int process_dyn_section(soinfo_t *si)
{
	const Elf32_Phdr* phdr;
	const Elf32_Dyn *d;
	int i;

	si->dyn_section = NULL;

	for (i = 0; i < si->n_phdr; i++) {
		phdr = si->phdr + i;
		if (phdr->p_type == PT_DYNAMIC) {
			si->dyn_section = (Elf32_Dyn *)(si->load_bias + phdr->p_vaddr);
			break;
		}
	}

	if (!si->dyn_section) {
		E("fail to get dynamic section.");
		return -1;
	}

	for (d = si->dyn_section; d->d_tag; d++) {
		switch (d->d_tag) {
			case DT_HASH:
				D("DT_HASH, d_ptr: 0x%x.", d->d_un.d_ptr);

				si->hash_table = (Elf32_Word *)(si->load_bias + d->d_un.d_ptr);
				si->nbucket = si->hash_table[0];
				si->nchain = si->hash_table[1];
				si->bucket = (Elf32_Word *)si->hash_table + 2;
				si->chain = si->bucket + si->nbucket;

				break;

			case DT_STRTAB:
				D("DT_STRTAB.");

				si->strtab = (void *)(si->load_bias + d->d_un.d_ptr);

				break;

			case DT_SYMTAB:
				D("DT_SYMTAB.");

				si->symtab = (Elf32_Sym *)(si->load_bias + d->d_un.d_ptr);

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

				si->plt_rel = (Elf32_Rel*)(si->load_bias + d->d_un.d_ptr);

				break;

			case DT_PLTRELSZ:
				D("DT_PLTRELSZ.");

				si->n_plt_rel = d->d_un.d_val / sizeof(Elf32_Rel);

				break;

			case DT_REL:
				D("DT_REL.");

				si->rel = (Elf32_Rel*)(si->load_bias + d->d_un.d_ptr);

				break;

			case DT_RELSZ:
				D("DT_RELSZ.");

				si->n_rel = d->d_un.d_val / sizeof(Elf32_Rel);

				break;

			case DT_PLTGOT:
				D("DT_PLTGOT.");

				break;

			case DT_RELA:
				D("DT_RELA.");

				break;

			case DT_INIT:
				D("DT_INIT.");

				si->init_func = (sc_func_t *)(si->load_bias + d->d_un.d_ptr);
				break;

			case DT_FINI:
				D("DT_FINI.");

				si->fini_func = (sc_func_t *)(si->load_bias + d->d_un.d_ptr);
				break;

			case DT_INIT_ARRAY:
				D("DT_INIT_ARRAY.");

				si->init_func_array = (sc_func_t *)(si->load_bias + d->d_un.d_ptr);
				break;

			case DT_INIT_ARRAYSZ:
				D("DT_INIT_ARRAYSZ.");

				si->n_init_func = (d->d_un.d_val) / sizeof(Elf32_Addr);
				break;

			case DT_FINI_ARRAY:
				D("DT_FINI_ARRAY.");

				si->fini_func_array = (sc_func_t *)(si->load_bias + d->d_un.d_ptr);
				break;

			case DT_FINI_ARRAYSZ:
				D("DT_FINI_ARRAYSZ.");

				si->n_fini_func = (d->d_un.d_val) / sizeof(Elf32_Addr);
				break;

			case DT_PREINIT_ARRAY:
				D("DT_PREINIT_ARRAY.");

				si->preinit_func_array = (sc_func_t *)(si->load_bias + d->d_un.d_ptr);
				break;

			case DT_PREINIT_ARRAYSZ:
				D("DT_PREINIT_ARRAYSZ.");

				si->n_preinit_func = (d->d_un.d_val) / sizeof(Elf32_Addr);
				break;

			case DT_TEXTREL:
				D("DT_TEXTREL.");

				break;
		}
	}

	return 0;
}
