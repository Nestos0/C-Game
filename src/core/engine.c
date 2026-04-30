#include "core/engine.h"
#include "text/rich_text.h"
#include "ui/screen.h"
#include <stdio.h>

extern struct GameState game_state;

void game_loop()
{
	bool is_running = game_state.is_running;
	int width = 0;
	fprintf(stdout, "%s\x1b[H\x1b[2J", "\x1b[44m");
	fflush(stdout);
	printf("Display Mecrics: W:%d H:%d\n", screen.width, screen.height);
	do {
		char ch = getchar();
		putchar(ch);
		/* printf("%d ", ch); */
		if (ch == 4) {
			screen_clear();
			break;
		} else if (ch == 127) {
			printf("\x1b[1D");
			printf("\x1b[s");
			putchar(' ');
			printf("\x1b[u");
			fflush(stdout);
			width -= 1;
		} else {
			width += 1;
		}
		if (width > 40) {
			putchar('\n');
			width = 0;
		}
	} while (is_running);
}
