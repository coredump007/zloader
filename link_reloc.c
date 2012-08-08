#include "debug.h"
#include "common.h"
#include "link.h"

static int do_sym_relocation(struct linkinfo *lki, uint32_t type, uint32_t sym_addr, uint32_t target)
{
	uint32_t *dst = (uint32_t *)target;

	int r = 0;

	D("Called.");

	switch (type) {
#ifndef R_386_JUMP_SLOT
#ifdef R_386_JMP_SLOT
#define R_386_JUMP_SLOT R_386_JMP_SLOT
#endif
#endif
		case R_386_JUMP_SLOT:
			*dst = sym_addr;
			break;

		case R_386_GLOB_DAT:
			*dst = sym_addr;
			break;

		case R_386_RELATIVE:
			*dst += (uint32_t)lki->load_start;
			break;

		case R_386_32:
			*dst += sym_addr;
			break;

		case R_386_PC32:
			*dst += sym_addr - target;
			break;

		default:
			E("unknown relocation type.");
			r = -1;

			break;
	}

	return r;
}

int do_relocation(struct linkinfo *lki, Elf32_Rel *rel, uint32_t count)
{
	Elf32_Sym *sym;

	uint32_t type, sym_idx, sym_addr;
	uint32_t target;

	char *sym_name;

	uint32_t i;
	int r;

	D("Called.");

	for (i = 0; i < count; i++) {
		type = ELF32_R_TYPE(rel[i].r_info);
		sym_idx = ELF32_R_SYM(rel[i].r_info);

		target = (rel[i].r_offset + lki->load_bias);

		if (sym_idx) {
			sym_name = (char *)(lki->strtab + lki->symtab[sym_idx].st_name);

			sym = sym_lookup(lki, sym_name);
			if (sym) {
				sym_addr = (unsigned)(sym->st_value + lki->load_bias);
				r = do_sym_relocation(lki, type, sym_addr, target);
				if (r) {
					return -1;
				}
			}
		}
	}

	return 0;
}
