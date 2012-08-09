#include "debug.h"
#include "common.h"
#include "list.h"
#include "link.h"

struct load_lib_data *g_load_lib_data;

int relocate_image(struct linkinfo *lki)
{
	int r = 0;

	D("Called.");

	if (lki->plt_rel) {
		r = do_relocation(lki, lki->plt_rel, lki->n_plt_rel);
		if (r) {
			E("fail to relocate plt rel.");
			return -1;
		}
	}

	D("Called.");

	if (lki->rel) {
		r = do_relocation(lki, lki->rel, lki->n_rel);
		if (r) {
			E("fail to relocate rel");
			return -1;
		}
	}

	return r;
}

int link_image(struct linkinfo *lki)
{
	int r;

	D("Called.");

	if (!g_load_lib_data) {
		E("No library loading routines.");
		return -1;
	}

	r = process_dyn_section(lki);
	if (r) {
		E("fail to process dynamic section.");
		return -1;
	}

	r = relocate_image(lki);
	if (r) {
		E("fail to relocate image.");
		return -1;
	}

	return 0;
}

void construct_image(struct linkinfo *lki)
{
	Elf32_Dyn *d;
	sc_func_t *func;

	struct linkinfo *l;
	struct list_head *pos;

	char *p;

	int r;
	int i;

	D("Called.");

	if (lki->b_constructed)
		return;

	D("Called.");

	lki->b_constructed = 1;

	if (lki->dyn_section) {
		if (lki->n_dt_needed) {
			list_for_each(pos, &lki->dt_needed_head) {
				l = container_of(pos, struct linkinfo, dt_needed_list);
				if (!check_magic(l->magic)) {
					E("not a lininfo struct.");
					continue;
				}

				construct_image(l);
			}
		}

		if (lki->init_func) {
			D("Call init func: %p.", lki->init_func);
			lki->init_func();
		}

		if (lki->init_func_array) {
			for (i = 0; i < lki->n_init_func; i++) {
				D("Call init func arrary: %p.", lki->init_func_array[i]);
				lki->init_func_array[i]();
			}
		}
	}

	return;
}
