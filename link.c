#include "debug.h"
#include "common.h"
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
