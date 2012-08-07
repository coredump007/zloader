#include "debug.h"

unsigned long debug_tag = DEBUG_TAG;

char *debug_tag_text[] = {
	[T_D_HASH] = "DYN HASH",
	[T_L_LDPATH] = "LIB LDPATH",
};
