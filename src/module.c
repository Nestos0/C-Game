#include "module.h"
#include <stdio.h>
#include <stdlib.h>

void all_modules_init()
{
	for (initcall_t *p = __start_initcalls;
		p < __stop_initcalls; p++) {
		int ret = (*p)();
		if (ret < 0) {
			fprintf(stderr, "\x1b[0K\x1b[0mError: Initcall at %p failed with code %d\n", (void *)*p, ret);
			exit(ret);
		}
	}
}

void all_modules_exit()
{
	for (initcall_t *p = __start_exitcalls;
		p < __stop_exitcalls; p++) {
		(*p)();
	}
}
