#ifndef LOAD_H
#define LOAD_H

#include "common.h"

struct vma {
	int used;

	void *area;
	Elf32_Word size;
};

struct loadinfo {
	unsigned long magic;

	int fd;

	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr[N_MAX_PHDR];

	void *phdr_mstart;
	Elf32_Word phdr_msize;

	void *load_start;
	Elf32_Word load_size;
	Elf32_Word load_bias;

	int n_vma;
	struct vma **vma;
};

#endif
