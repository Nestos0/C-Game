/* --- File: ./src/ui/display.c --- */
#include "ui/display.h"
#include "module.h"
#include "text/utf8.h"
#include "ui/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct GenericWidget GenericWidget;

struct Screen {
	int width, height;
	Cell *cells;
	GenericWidget *root;
	struct {
		int x;
		int y;
	} cursor;
};

int screen_add_root(Screen *s, GenericWidget *gw)
{
	if (!s) {
		fprintf(stderr, "Exception thrown: read access violation. s was nullptr.\n");
		return -1;
	}
	if (!gw) {
		fprintf(stderr, "Exception thrown: read access violation. gw was nullptr.\n");
		return -2;
	}
	s->root = gw;
	return 0;
}

void screen_set_cursor(struct Screen *s, int *x, int *y)
{
	if (s == NULL)
		return;

	if (x != NULL) {
		s->cursor.x = *x;
	}

	if (y != NULL) {
		s->cursor.y = *y;
	}
}

void screen_get_size(Screen *s, int *width, int *height)
{
	if (width)
		*width = s->width;
	if (height)
		*height = s->height;
}

int screen_get_width(Screen *s)
{
	return s->width;
}

int screen_get_height(Screen *s)
{
	return s->height;
}

void set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg)
{
	if (x < 0 || x >= screen->width || y < 0 || y >= screen->height)
		return;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	int idx = y * screen->width + x;
	s->cells[idx].cp = cp;
	s->cells[idx].fg = fg_color;
	s->cells[idx].bg = bg_color;
	s->cells[idx].dirty = true;
	s->cells[idx].wide = false;
	s->cells[idx].wide_cont = false;
}

void set_cell_wide(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg)
{
	if (x < 0 || x + 1 >= screen->width || y < 0 || y >= screen->height)
		return;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	int idx = y * screen->width + x;
	s->cells[idx].cp = cp;
	s->cells[idx].fg = fg_color;
	s->cells[idx].bg = bg_color;
	s->cells[idx].dirty = true;
	s->cells[idx].wide = true;
	s->cells[idx].wide_cont = false;

	/* continuation cell */
	s->cells[idx + 1].cp = 0;
	s->cells[idx + 1].fg = fg_color;
	s->cells[idx + 1].bg = bg_color;
	s->cells[idx + 1].dirty = true;
	s->cells[idx + 1].wide = false;
	s->cells[idx + 1].wide_cont = true;
}

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
	s->root = NULL;
	return s;
}

int __screen_init(void)
{
	int width, height;
	term_get_size(&width, &height);
	screen = screen_create(width, height);
	return 0;
}
APP_INIT(__screen_init);

int screen_clear(void)
{
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

// 辅助函数：安全的缓冲区写入
static void buf_write(int fd, const char *data, int len, char *buf, int *buf_pos, int buf_size)
{
	if (len <= 0)
		return;

	if (*buf_pos + len > buf_size) {
		// 缓冲区即将溢出，先写入现有内容
		write(fd, buf, *buf_pos);
		*buf_pos = 0;

		if (len > buf_size) {
			// 如果单个数据块比缓冲区还大，直接写入
			write(fd, data, len);
			return;
		}
	}

	// 复制到缓冲区
	memcpy(buf + *buf_pos, data, len);
	*buf_pos += len;
}

// 辅助函数：将整数格式化为ANSI颜色代码
static int fmt_ansi_color(char *out, int fg_r, int fg_g, int fg_b, int bg_r, int bg_g, int bg_b)
{
	return sprintf(out, "\x1b[38;2;%d;%d;%d;48;2;%d;%d;%dm", fg_r, fg_g, fg_b, bg_r, bg_g, bg_b);
}

void screen_flush(Screen *s)
{
	int last_fg[3] = { -1, -1, -1 };
	int last_bg[3] = { -1, -1, -1 };
	int cur_x = 0, cur_y = 0; // 跟踪逻辑光标位置

	// 使用本地缓冲区减少系统调用
	char buffer[4096];
	int buf_pos = 0;

	for (int y = 0; y < s->height; y++) {
		char cmd[32];
		int len = snprintf(cmd, sizeof(cmd), "\x1b[%d;1H", 0);
		buf_write(STDOUT_FILENO, cmd, len, buffer, &buf_pos, sizeof(buffer));
		cur_x = 0;
		for (int x = 0; x < s->width; x++) {
			Cell *c = &s->cells[y * s->width + x];

			if (!c->dirty)
				continue;
			if (c->wide_cont) {
				continue;
			}

			if (y != cur_y || x != cur_x) {
				char cmd[32];
				int len = snprintf(cmd, sizeof(cmd), "\x1b[%d;%dH", y + 1, x + 1);
				buf_write(STDOUT_FILENO, cmd, len, buffer, &buf_pos, sizeof(buffer));
				cur_x = x;
				cur_y = y;
			}

			if (c->fg.r != last_fg[0] || c->fg.g != last_fg[1] || c->fg.b != last_fg[2]
				|| c->bg.r != last_bg[0] || c->bg.g != last_bg[1] || c->bg.b != last_bg[2]) {

				char color_cmd[48];
				int len = fmt_ansi_color(color_cmd, c->fg.r, c->fg.g, c->fg.b,
					c->bg.r, c->bg.g, c->bg.b);
				buf_write(STDOUT_FILENO, color_cmd, len, buffer, &buf_pos, sizeof(buffer));

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
				buf_write(STDOUT_FILENO, utf8, len, buffer, &buf_pos, sizeof(buffer));

				cur_x += 1;
				if (c->wide) {
					cur_x += 1;
				}
			} else {
				buf_write(STDOUT_FILENO, " ", 1, buffer, &buf_pos, sizeof(buffer));
				cur_x += 1;
			}

			c->dirty = false;
		}
	}

	buf_write(STDOUT_FILENO, "\x1b[0m", 4, buffer, &buf_pos, sizeof(buffer));
	if (buf_pos > 0) {
		write(STDOUT_FILENO, buffer, buf_pos);
	}
}

void screen_destroy(Screen *s)
{
	if (s) {
		free(s->cells);
		free(s);
	}
}

int __screen_exit(void)
{
	screen_destroy(screen);
	screen = NULL;
	term_restore();
	return 0;
}
APP_EXIT(__screen_exit);
