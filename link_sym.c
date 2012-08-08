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

Elf32_Sym *sym_lookup(struct linkinfo *lki, const char *name)
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
