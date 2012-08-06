#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define D(fmt, args...) \
	fprintf(stderr, "%s() - %d: "fmt"\n", __func__, __LINE__, ##args)

#define E(fmt, args...) \
	fprintf(stderr, "%s() - %d: "fmt"\n", __func__, __LINE__, ##args)

#endif
