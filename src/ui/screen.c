#include "ui/screen.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct ScreenInfo screen;

int screen_clear(void)
{
	int done = 0;
	done = fprintf(stdout, "%s\x1b[H\x1b[2J", "\x1b[44m");
	fflush(stdout);
	return done;
}

void get_terminal_size(int *width, int *height)
{
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
		*width = w.ws_col;
		*height = w.ws_row;
	} else {
		*width = 80;
		*height = 24;
	}
}

int __screen_init(void)
{
	get_terminal_size(&screen.height, &screen.width);
	return 0;
}
init_register(__screen_init);

struct termios oldt, newt;

int __terminal_init()
{
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;

	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	return 0;
}
init_register(__terminal_init);

void terminal_restore()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
