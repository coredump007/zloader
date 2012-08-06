#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <elf.h>
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "loader.h"
#include "link.h"

struct libinfo {
	int fd;

	Elf32_Ehdr ehdr;
	Elf32_Phdr *phdr[N_MAX_PHDR];

	void *phdr_mstart;
	Elf32_Word phdr_msize;

	void *load_start;
	Elf32_Word load_size;
	Elf32_Word load_bias;
};

static pthread_mutex_t dl_lock = PTHREAD_MUTEX_INITIALIZER;

static int check_elf_header(const Elf32_Ehdr* hdr)
{
	D("Called.");

	if (hdr->e_ident[EI_MAG0] != ELFMAG0) return -1;
	if (hdr->e_ident[EI_MAG1] != ELFMAG1) return -1;
	if (hdr->e_ident[EI_MAG2] != ELFMAG2) return -1;
	if (hdr->e_ident[EI_MAG3] != ELFMAG3) return -1;

	D("Called.");

	return 0;
}

static int open_library(struct libinfo *li, const char *name)
{
	int r;

	D("Called.");

	memset(li, 0, sizeof(*li));

	li->fd = open(name, O_RDONLY);
	if (li->fd == -1) {
		E("fail to open file.");

		return -1;
	}

	D("Called.");

	r = read(li->fd, &li->ehdr, sizeof(li->ehdr));
	if (r != sizeof(li->ehdr)) {
		E("fail to read ELF header.");

		close(li->fd);
		return -1;
	}

	D("Called.");

	if (check_elf_header(&li->ehdr) < 0) {
		E("not an ELF file.");

		close(li->fd);
		return -1;
	}

	return 0;
}

static void close_library(struct libinfo *li)
{
	D("Called.");

	close(li->fd);

	memset(li, 0, sizeof(*li));

	return;
}

static int map_phdr(struct libinfo *li)
{
	Elf32_Ehdr *ehdr = &li->ehdr;
	Elf32_Word p_start, p_end, p_offset;
	Elf32_Word s;

	int i;
	void *m;

	D("Called.");

	s = ehdr->e_phnum * sizeof(Elf32_Phdr);

	D("PHDR size: %u.", s);

	p_start = PAGE_START(ehdr->e_phoff);
	p_end = PAGE_END(ehdr->e_phoff + s);
	p_offset = PAGE_OFFSET(ehdr->e_phoff);

	s = p_end - p_start;

	D("p_start: %u, p_end: %u, p_offset: %u, size: %u.",
		p_start, p_end, p_offset, s);

	m = mmap(NULL, s, PROT_READ, MAP_PRIVATE, li->fd, p_start);
	if (m == MAP_FAILED) {
		E("fail to map phdr.");
		return -1;
	}

	li->phdr_mstart = m;
	li->phdr_msize = s;

	m = m + p_offset;

	for (i = 0; i < ehdr->e_phnum; i++)
		li->phdr[i] = m + i * ehdr->e_phentsize;

	return 0;
}

static void unmap_phdr(struct libinfo *li)
{
	D("Called.");

	munmap(li->phdr_mstart, li->phdr_msize);

	li->phdr_mstart = NULL;

	return;
}

static Elf32_Addr calc_load_size(struct libinfo *li)
{
	Elf32_Ehdr *ehdr = &li->ehdr;

	Elf32_Addr min_vaddr = 0xFFFFFFFFU;
	Elf32_Addr max_vaddr = 0x00000000U;

	int i;

	D("Called.");

	if (!li->phdr)
		return -1;

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr* phdr = li->phdr[i];

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

	li->load_size = max_vaddr - min_vaddr;

	D("load_size: %u.", li->load_size);

	return 0;
}

#define MAYBE_MAP_FLAG(x,from,to)    (((x) & (from)) ? (to) : 0)
#define PFLAGS_TO_PROT(x)            (MAYBE_MAP_FLAG((x), PF_X, PROT_EXEC) | \
                                      MAYBE_MAP_FLAG((x), PF_R, PROT_READ) | \
                                      MAYBE_MAP_FLAG((x), PF_W, PROT_WRITE))

static int map_seg(struct libinfo *li)
{
	Elf32_Ehdr *ehdr = &li->ehdr;

	Elf32_Addr m_start, m_end, mp_start, mp_end;
	Elf32_Addr f_start, f_end, fp_start, fp_end;
	Elf32_Addr mf_end;

	void *m;

	unsigned long s;
	int i;

	D("Called.");

	li->load_start = mmap(NULL, li->load_size,
			PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0L);

	if (li->load_start == MAP_FAILED) {
		E("Fail to reserve virtual address.");
		return -1;
	}

	D("load_start: %p.", li->load_start);

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr* phdr = li->phdr[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		if (!li->load_bias) {
			D("Calc load_bias, first phdr->p_vaddr: 0x%x",
				phdr->p_vaddr);

			li->load_bias = (Elf32_Addr)li->load_start - phdr->p_vaddr;

			D("li->load_bias: 0x%x.", li->load_bias);
		}

		m_start = phdr->p_vaddr + li->load_bias;
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
                        MAP_FIXED|MAP_PRIVATE, li->fd, fp_start);

		if (m == MAP_FAILED) {
			E("fail to map segments.");
			return -1;
		}

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
		}
	}

	return 0;
}

static int load_seg(struct libinfo *li)
{
	int r;

	D("Called.");

	r = calc_load_size(li);
	if (r) {
		E("fail to calc load size.");
		return -1;
	}

	D("Called.");

	r = map_seg(li);
	if (r) {
		E("fail to map seg.");
		return -1;
	}

	return 0;
}

static soinfo_t *make_soinfo(struct libinfo *li)
{
	soinfo_t *si;

	int i;

	D("Called.");

	si = malloc(sizeof(soinfo_t));
	if (!si) {
		E("fail to allocate soinfo.");
		return NULL;
	}

	si->load_start = li->load_start;
	si->load_bias = li->load_bias;
	si->load_size = li->load_size;
	si->n_phdr = li->ehdr.e_phnum;

	for (i = 0; i < si->n_phdr; i++)
		memcpy(&si->phdr[i], li->phdr[i], sizeof(Elf32_Phdr));

	return si;
}

static soinfo_t *load_library(const char *name)
{
	struct libinfo li;
	soinfo_t *si;

	int r;

	D("Called.");

	r = open_library(&li, name);
	if (r == -1) {
		E("fail to open library.");
		return NULL;
	}

	r = map_phdr(&li);
	if (r) {
		E("fail to map phdr.");
		return NULL;
	}

	r = load_seg(&li);
	if (r) {
		E("fail to map segments.");
		return NULL;
	}

	si = make_soinfo(&li);
	if (!si) {
		E("fail to get soinfo.");
		return NULL;
	}

	return si;
}

static soinfo_t *link_library(soinfo_t *si)
{
	int r;

	D("Called.");

	r = link_image(si);
	if (r) {
		E("fail to link image.");

		return NULL;
	}

	return si;
}

static soinfo_t *find_library(const char *name)
{
	soinfo_t *si;

	D("Called.");

	si = load_library(name);
	if(si == NULL) {
		E("fail to load library.");
		return NULL;
	}

	return link_library(si);
}

void *loader_dlopen(const char *filename, int flag)
{
	soinfo_t *si;

	D("Called.");

	pthread_mutex_lock(&dl_lock);
	si = find_library(filename);
	if (si) {
		si->refcount++;
	}
	pthread_mutex_unlock(&dl_lock);
	return si;
}
