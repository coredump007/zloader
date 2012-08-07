#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_TAG	(T_D_HASH | T_L_LDPATH)

#include <stdio.h>

extern unsigned long debug_tag;
extern char *debug_tag_text[];

enum {
	T_D_HASH = (1 << 0),
	T_L_LDPATH = (1 << 1),
};

#define D_TAG(tag) (debug_tag & tag)

#define D(fmt, args...) \
	fprintf(stderr, "%s() - %d: "fmt"\n", __func__, __LINE__, ##args)

#define DT(tag, fmt, args...) do { \
	if (debug_tag & tag) { \
	fprintf(stderr, "%s %s() - %d: "fmt"\n", debug_tag_text[tag], \
		__func__, __LINE__, ##args); \
	} \
	}while(0)

#define E(fmt, args...) \
	fprintf(stderr, "%s() - %d: "fmt"\n", __func__, __LINE__, ##args)

#endif
