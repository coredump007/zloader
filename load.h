#ifndef LOAD_H
#define LOAD_H

#include <elf.h>
#include "common.h"

struct loadinfo {
	int fd;

	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr[N_MAX_PHDR];

	void *phdr_mstart;
	Elf32_Word phdr_msize;

	void *load_start;
	Elf32_Word load_size;
	Elf32_Word load_bias;
};

#endif
