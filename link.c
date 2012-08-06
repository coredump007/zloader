#include "debug.h"
#include "common.h"
#include "link.h"

int relocate_image(soinfo_t *si)
{
	int r = 0;

	D("Called.");

	if (si->plt_rel) {
		r = do_relocation(si, si->plt_rel, si->n_plt_rel);
		if (r) {
			E("fail to relocate plt rel.");
			return -1;
		}
	}

	if (si->rel) {
		r = do_relocation(si, si->rel, si->n_rel);
		if (r) {
			E("fail to relocate rel");
			return -1;
		}
	}

	return r;
}

int link_image(soinfo_t *si)
{
	int r;

	D("Called.");

	r = process_dyn_section(si);
	if (r) {
		E("fail to process dynamic section.");
		return -1;
	}

	r = relocate_image(si);
	if (r) {
		E("fail to relocate image.");
		return -1;
	}

	return 0;
}
