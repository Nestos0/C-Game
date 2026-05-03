#include "ui/display.h"
#include "module.h"
#include "text/utf8.h"
#include "ui/terminal.h"
#include "ui/widgets.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

RGB get_anti_color(RGB color)
{
	RGB ret = { 0 };
	ret.r = (255 - color.r);
	ret.g = (255 - color.g);
	ret.b = (255 - color.b);
	return ret;
}

Screen *screen_create(int width, int height)
{
	Screen *s = (Screen *)calloc(1, sizeof(Screen));
	if (!s)
		return NULL;

	s->cells = (Cell *)calloc((size_t)(width * height), sizeof(Cell));
	if (!s->cells) {
		free(s);
		return NULL;
	}

	s->width = width;
	s->height = height;
	return s;
}

int __screen_init(void)
{
	int width, height;
	term_get_size(&height, &width);
	screen = screen_create(width, height);
	return 0;
}
APP_INIT(__screen_init);

int screen_clear(void)
{
	// TODO add display environment check
	ansi_clear_screen();
	return 0;
}

int screen_set_fg(Screen *s, RGB rgb)
{
	for (size_t y = 0; y < s->height; y++) {
		for (size_t x = 0; x < s->width; x++) {
			int idx = y * s->width + x;
			s->cells[idx].fg = rgb;
		}
	}
	return 0;
}

int screen_set_bg(Screen *s, RGB rgb)
{
	for (size_t y = 0; y < s->height; y++) {
		for (size_t x = 0; x < s->width; x++) {
			size_t idx = y * s->width + x;
			s->cells[idx].bg = rgb;
			s->cells[idx].dirty = true;
		}
	}
	return 0;
}

Screen s_buffer;
void screen_flush(Screen *s)
{
	int last_fg[3] = { -1, -1, -1 };
	int last_bg[3] = { -1, -1, -1 };

	for (int y = 0; y < s->height; y++) {
		for (int x = 0; x < s->width; x++) {
			Cell *c = &s->cells[y * s->width + x];

			if (!c->dirty)
				continue;
			if (c->wide_cont)
				continue;
			ansi_cursor_goto(y, x);

			if (c->fg.r != last_fg[0] || c->fg.g != last_fg[1] || c->fg.b != last_fg[2]
				|| c->bg.r != last_bg[0] || c->bg.g != last_bg[1] || c->bg.b != last_bg[2]) {
				char buf[48];
				int n = snprintf(
					buf, sizeof(buf),
					"\x1b[38;2;%d;%d;%d;48;2;%d;%d;%dm",
					c->fg.r, c->fg.g, c->fg.b, c->bg.r,
					c->bg.g, c->bg.b);
				write(STDOUT_FILENO, buf, (size_t)n);
				last_fg[0] = c->fg.r;
				last_fg[1] = c->fg.g;
				last_fg[2] = c->fg.b;
				last_bg[0] = c->bg.r;
				last_bg[1] = c->bg.g;
				last_bg[2] = c->bg.b;
			}

			if (c->cp > 0) {
				char utf8[8];
				int len = utf8_encode(c->cp, utf8);
				write(STDOUT_FILENO, utf8, (size_t)len);
				if (c->wide)
					x++;
			} else {
				write(STDOUT_FILENO, " ", 1);
			}

			c->dirty = false;
		}
	}
	write(STDOUT_FILENO, "\x1b[0m", 4);
}

void screen_destroy(Screen *s)
{
	if (s) {
		free(s->cells);
		free(s);
	}
}

int __screen_uninit(void)
{
	screen_destroy(screen);
	screen = NULL;
	term_restore();
	return 0;
}
APP_EXIT(__screen_uninit);

