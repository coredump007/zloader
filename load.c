#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/mman.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "load.h"

static int setup_vma(struct loadinfo *ldi)
{
	Elf32_Ehdr *ehdr = &ldi->ehdr;

	int n = 0;

	void *m;

	unsigned long s;
	int i;

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr* phdr = &ldi->phdr[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		n++;
	}

	n *= 2;

	s = sizeof(struct vma *) * n;
	s += sizeof(struct vma) * n;

	m = calloc(1, s);
	if (!m) {
		E("fail to allocate memory.");
		return -1;
	}

	ldi->n_vma = n;
	ldi->vma = m;

	m += n * sizeof(struct vma *);

	for (i = 0; i < n; i++) {
		ldi->vma[i] = m;

		m += sizeof(struct vma);
	}

	return 0;
}

static Elf32_Addr calc_load_size(struct loadinfo *ldi)
{
	Elf32_Ehdr *ehdr = &ldi->ehdr;

	Elf32_Addr min_vaddr = 0xFFFFFFFFU;
	Elf32_Addr max_vaddr = 0x00000000U;

	int i;

	D("Called.");

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr* phdr = &ldi->phdr[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_vaddr < min_vaddr)
			min_vaddr = phdr->p_vaddr;

		if (phdr->p_vaddr + phdr->p_memsz > max_vaddr)
			max_vaddr = phdr->p_vaddr + phdr->p_memsz;
	}

	if (min_vaddr > max_vaddr) {
		return -1;
	}

	min_vaddr = PAGE_START(min_vaddr);
	max_vaddr = PAGE_END(max_vaddr);

	ldi->load_size = max_vaddr - min_vaddr;

	D("load_size: %u.", ldi->load_size);

	return 0;
}

#define MAYBE_MAP_FLAG(x,from,to)    (((x) & (from)) ? (to) : 0)
#define PFLAGS_TO_PROT(x)            (MAYBE_MAP_FLAG((x), PF_X, PROT_EXEC) | \
                                      MAYBE_MAP_FLAG((x), PF_R, PROT_READ) | \
                                      MAYBE_MAP_FLAG((x), PF_W, PROT_WRITE))

static int map_image(struct loadinfo *ldi)
{
	Elf32_Ehdr *ehdr = &ldi->ehdr;

	Elf32_Addr m_start, m_end, mp_start, mp_end;
	Elf32_Addr f_start, f_end, fp_start, fp_end;
	Elf32_Addr mf_end;

	int n_vma = 0;

	void *m;

	unsigned long s;
	int i;

	D("Called.");

	ldi->load_start = mmap(NULL, ldi->load_size,
			PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0L);

	if (ldi->load_start == MAP_FAILED) {
		E("Fail to reserve virtual address.");
		return -1;
	}

	D("load_start: %p.", ldi->load_start);

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr* phdr = &ldi->phdr[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		if (!ldi->load_bias) {
			D("Calc load_bias, first phdr->p_vaddr: 0x%x",
				phdr->p_vaddr);

			ldi->load_bias = (Elf32_Addr)ldi->load_start - phdr->p_vaddr;

			D("ldi->load_bias: 0x%x.", ldi->load_bias);
		}

		m_start = phdr->p_vaddr + ldi->load_bias;
		m_end = m_start + phdr->p_memsz;

		mp_start = PAGE_START(m_start);
		mp_end = PAGE_END(m_end);

		mf_end = m_start + phdr->p_filesz;

		f_start = phdr->p_offset;
		f_end = f_start + phdr->p_filesz;

		fp_start = PAGE_START(f_start);
		fp_end = PAGE_END(f_end);

		s = f_end - fp_start;
		m = mmap((void *)mp_start, s, PFLAGS_TO_PROT(phdr->p_flags),
                        MAP_FIXED|MAP_PRIVATE, ldi->fd, fp_start);

		if (m == MAP_FAILED) {
			E("fail to map segments.");
			return -1;
		}

		ldi->vma[n_vma]->used = 1;
		ldi->vma[n_vma]->area = m;
		ldi->vma[n_vma]->size = s;

		n_vma++;

		D("Map SEG p_vaddr: 0x%x to %p",
			phdr->p_vaddr, m);

		if ((phdr->p_flags & PF_W) && PAGE_OFFSET(mf_end)) {
			s = PAGE_SIZE - PAGE_OFFSET(mf_end);
			memset((void*)mf_end, 0, s);
		}

		mf_end = PAGE_END(mf_end);

		if (mp_end > mf_end) {
			s = mp_end - mf_end;
			m = mmap((void*)mf_end, s,
					PFLAGS_TO_PROT(phdr->p_flags),
					MAP_FIXED|MAP_ANONYMOUS|MAP_PRIVATE,
					-1, 0);
			if (m == MAP_FAILED) {
				E("fail to map segments.");
				return -1;
			}

			ldi->vma[n_vma]->used = 1;
			ldi->vma[n_vma]->area = m;
			ldi->vma[n_vma]->size = s;

			n_vma++;
		}
	}

	return 0;
}

int load_image(struct loadinfo *ldi)
{
	int r;

	D("Called.");

	r = setup_vma(ldi);
	if (r) {
		E("fail to setup vma.");
		return -1;
	}

	r = calc_load_size(ldi);
	if (r) {
		E("fail to calc load size.");
		return -1;
	}

	D("Called.");

	r = map_image(ldi);
	if (r) {
		E("fail to map image.");
		return -1;
	}

	return 0;
}
