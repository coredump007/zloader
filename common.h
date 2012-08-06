#ifndef COMMON_H
#define COMMON_H

#undef PAGE_MASK
#undef PAGE_SIZE
#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)

/* Returns the address of the page starting at address 'x' */
#define PAGE_START(x)  ((x) & ~PAGE_MASK)

/* Returns the offset of address 'x' in its memory page, i.e. this is the
 * same than 'x' - PAGE_START(x) */
#define PAGE_OFFSET(x) ((x) & PAGE_MASK)

/* Returns the address of the next page after address 'x', unless 'x' is
 * itself at the start of a page. Equivalent to:
 *
 *  (x == PAGE_START(x)) ? x : PAGE_START(x)+PAGE_SIZE
 */
#define PAGE_END(x)    PAGE_START((x) + (PAGE_SIZE-1))

#include <elf.h>

#define N_MAX_PHDR (65536 / sizeof(Elf32_Phdr))

#endif
