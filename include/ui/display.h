#pragma once
#include <stdint.h>
#include <sys/types.h>

typedef struct POS {
	int x;
	int y;
} POS;

typedef struct RGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGB;

typedef struct {
	uint32_t cp;
	struct RGB fg;
	struct RGB bg;
	bool wide : 1;
	bool wide_cont : 1;
	bool dirty : 1;
} Cell;

typedef struct Screen Screen;

typedef struct Environment {
	RGB fg;
	RGB bg;
	Screen *screen;
} Environment;

extern struct Screen *screen;

extern struct Environment G_ENV;

void screen_destroy(Screen *s);

RGB get_anti_color(RGB color);

int screen_clear(void);

Screen *screen_create(int width, int height);

int screen_set_fg(Screen *s, RGB rgb);
int screen_set_bg(Screen *s, RGB rgb);

void screen_flush(Screen *s);

void set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg);

void set_cell_wide(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg);

void screen_get_size(Screen *s, int *width, int *height);

int screen_get_width(Screen *s);
int screen_get_height(Screen *s);

void screen_set_cursor(struct Screen *s, int *x, int *y);
