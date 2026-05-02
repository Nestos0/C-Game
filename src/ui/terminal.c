#include "ui/terminal.h"
#include "module.h"
#include "text/utf8.h"
#include "ui/display.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

static bool raw_mode_active = false;
struct termios oldt, newt;

int term_restore()
{
	write(STDIN_FILENO, "\e[?1049l", 8);
	ansi_cursor_show();
	if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) < 0) {
		return -1;
	}
	return 0;
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

/* int *term_get_size(int *width, int *height) */
/* { */
/* 	int w, h; */
/* 	struct winsize win; */
/* 	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0) { */
/* 		*width = win.ws_col; */
/* 		*height = win.ws_row; */
/* 	} else { */
/* 		*width = 80; */
/* 		*height = 24; */
/* 	} */
/* } */

bool term_enter_raw(void)
{
	if (!isatty(STDIN_FILENO))
		return false;
	if (raw_mode_active)
		return true;

	if (tcgetattr(STDIN_FILENO, &oldt) < 0)
		return false;

	struct termios raw = oldt;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= CS8;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
		return false;
	raw_mode_active = true;
	return true;
}

int __terminal_init()
{
	write(STDIN_FILENO, "\e[?1049h", 8);
	ansi_cursor_hide();

	if (!isatty(STDIN_FILENO))
		return -1;
	if (raw_mode_active)
		return 0;

	if (tcgetattr(STDIN_FILENO, &oldt) < 0)
		return -1;

	struct termios raw = oldt;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= CS8;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
		return -1;
	raw_mode_active = true;
	return 0;
}
APP_INIT(__terminal_init);

void term_leave_raw(void)
{
	if (raw_mode_active) {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
		raw_mode_active = false;
	}
}

bool term_get_size(int *rows, int *cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)
		return false;
	*rows = (int)ws.ws_row;
	*cols = (int)ws.ws_col;
	return true;
}

void term_set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg)
{
	if (x > screen->width)
		return;
	if (y > screen->height)
		return;

	int idx = (y)*screen->width + (x);
	s->cells[idx].cp = cp;

	screen->cells[idx].cp = (cp);
	screen->cells[idx].fg = (fg) ? *fg : G_ENV.fg;
	screen->cells[idx].bg = (fg) ? *bg : G_ENV.bg;
	screen->cells[idx].dirty = true;
	screen->cells[idx].wide = false;
	screen->cells[idx].wide_cont = false;
	screen->cursor.x = x;
	screen->cursor.y = y;

	if (cp_display_width(cp) > 1) {
		screen->cells[idx].wide = true;
		screen->cells[idx + 1].wide_cont = false;
	}
}

void ansi_cursor_hide(void)
{
	write(STDOUT_FILENO, "\x1b[?25l", 6);
}

void ansi_cursor_show(void)
{
	write(STDOUT_FILENO, "\x1b[?25h", 6);
}

int __terminal_exit(void)
{
	term_restore();
	return 0;
}
APP_EXIT(__terminal_exit);

void ansi_clear_screen(void)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
}

void ansi_cursor_goto(int row, int col)
{
	char buf[32];
	int n = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row + 1, col + 1);
	write(STDOUT_FILENO, buf, (size_t)n);
}

void ansi_set_fg_rgb(int r, int g, int b)
{
	char buf[32];
	int n = snprintf(buf, sizeof(buf), "\x1b[38;2;%d;%d;%dm", r, g, b);
	write(STDOUT_FILENO, buf, (size_t)n);
}

void ansi_set_bg_rgb(int r, int g, int b)
{
	char buf[32];
	int n = snprintf(buf, sizeof(buf), "\x1b[48;2;%d;%d;%dm", r, g, b);
	write(STDOUT_FILENO, buf, (size_t)n);
}

void ansi_reset(void)
{
	write(STDOUT_FILENO, "\x1b[0m", 4);
}
