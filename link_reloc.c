#include "debug.h"
#include "common.h"
#include "link.h"

static unsigned elfhash(const char *_name)
{
	const unsigned char *name = (const unsigned char *) _name;
	unsigned h = 0, g;

	while(*name) {
		h = (h << 4) + *name++;
		g = h & 0xf0000000;
		h ^= g;
		h ^= g >> 24;
	}
	return h;
}

static Elf32_Sym *do_sym_lookup(soinfo_t *si, const char *name)
{
	Elf32_Sym *symtab = si->symtab;

	const char *strtab = si->strtab;
	unsigned hash = elfhash(name);

	Elf32_Sym *s;
	unsigned n;

	D("Called.");

	for(n = si->bucket[hash % si->nbucket]; n != 0; n = si->chain[n]){
		s = symtab + n;
		if(strcmp(strtab + s->st_name, name)) continue;
		/* only concern ourselves with global and weak symbol definitions */
		switch(ELF32_ST_BIND(s->st_info)){
			case STB_GLOBAL:
			case STB_WEAK:
				if(s->st_shndx == SHN_UNDEF)
					continue;

				return s;
		}
	}

	D("Called.");

	return NULL;
}

static int do_sym_relocation(soinfo_t *si, uint32_t type, uint32_t sym_addr, uint32_t target)
{
	uint32_t *dst = (uint32_t *)target;

	int r = 0;

	D("Called.");

	switch (type) {
//		case R_386_JUMP_SLOT:
		case R_386_JMP_SLOT:
			*dst = sym_addr;
			break;

		case R_386_GLOB_DAT:
			*dst = sym_addr;
			break;

		case R_386_RELATIVE:
			*dst += (uint32_t)si->load_start;
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

int do_relocation(soinfo_t *si, Elf32_Rel *rel, uint32_t count)
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

		target = (rel[i].r_offset + si->load_bias);

		if (sym_idx) {
			sym_name = (char *)(si->strtab + si->symtab[sym_idx].st_name);

			sym = do_sym_lookup(si, sym_name);
			if (sym) {
				sym_addr = (unsigned)(sym->st_value + si->load_bias);
				r = do_sym_relocation(si, type, sym_addr, target);
				if (r) {
					return -1;
				}
			}
		}
	}

	return 0;
}
