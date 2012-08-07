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

static void dump_hash_table(struct linkinfo *lki)
{
	int i;

	DT(T_D_HASH, "lki->hash_table: %p, lki->nbucket: %u, lki->nchain: %u,"
			"lki->bucket: %p, lki->chain: %p.", lki->hash_table,
			lki->nbucket, lki->nchain, lki->bucket, lki->chain);

	DT(T_D_HASH, "Bucket:");

	for (i = 0; i < lki->nbucket; i++)
		DT(T_D_HASH, "bucket[%d]: %u.", i, lki->bucket[i]);

	DT(T_D_HASH, "Chain:");

	for (i = 0; i < lki->nchain; i++)
		DT(T_D_HASH, "chain[%d]: %u.", i, lki->chain[i]);

	return;
}

static Elf32_Sym *do_sym_lookup(struct linkinfo *lki, const char *name)
{
	Elf32_Sym *symtab = lki->symtab;

	const char *strtab = lki->strtab;
	unsigned hash = elfhash(name);

	Elf32_Sym *s;
	unsigned n;

	D("Called.");

	if (D_TAG(T_D_HASH))
		dump_hash_table(lki);

	DT(T_D_HASH, "name: %s.", name);
	DT(T_D_HASH, "hash: %u.", hash);
	DT(T_D_HASH, "bucket: %u.", hash % lki->nbucket);

	for(n = lki->bucket[hash % lki->nbucket]; n != 0; n = lki->chain[n]){
		s = symtab + n;

		DT(T_D_HASH, "n: %u, string: %s.", n, strtab + s->st_name);

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

			sym = do_sym_lookup(lki, sym_name);
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
