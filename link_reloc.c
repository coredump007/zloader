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

static int relocation_sym_lookup(struct linkinfo *lki, const char *name, uint32_t *sym_addr)
{
	Elf32_Sym *s = NULL;

	struct linkinfo *l;
	struct list_head *pos;

	s = sym_lookup(lki, name);
	if (s) {
		D("Match LOCAL.");

		*sym_addr = (s->st_value + lki->load_bias);
		return 0;
	}

	if (lki->n_dt_needed) {
		list_for_each(pos, &lki->dt_needed_head) {
			l = container_of(pos, struct linkinfo, dt_needed_list);
			if (!check_magic(l->magic)) {
				E("Not a linkinfo but appeared in dependence list.");
				continue;
			}

			s = sym_lookup(l, name);
			if (s) {
				D("Match Other.");

				*sym_addr = (s->st_value + l->load_bias);
				return 0;
			}
		}
	}

	return -1;
}

int do_relocation(struct linkinfo *lki, Elf32_Rel *rel, uint32_t count)
{
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

			r = relocation_sym_lookup(lki, sym_name, &sym_addr);
			if (r != -1) {
				r = do_sym_relocation(lki, type, sym_addr, target);
				if (r) {
					return -1;
				}

				D("RELOC sym: %s, target: %x, from 0x%x to %x", sym_name, target, sym_addr, *(uint32_t *)target);
			}
		}
	}

	return 0;
}
