#include "ui/screen.h"
#include <stdio.h>

int screen_clear(void)
{
	int done = 0;
	done = fprintf(stdout, "\x1b[H\x1b[2J");
	fflush(stdout);
	return done;
}

void screen_init(void)
{
	screen_clear();
}
init_register(screen_init);
