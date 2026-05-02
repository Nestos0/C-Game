#pragma once
#include "ui/display.h"
#include <stdint.h>

typedef struct BoxLTRB {
	int left;
	int right;
	int top;
	int bottom;
} BoxLTRB;

typedef struct BoxBuffer {
	BoxLTRB **box;
	size_t count;
	size_t cap;
} BoxBuffer;

extern struct BoxBuffer *G_BOX_BUFFER;

BoxLTRB *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg);

BoxLTRB *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg);

void widget_write_text(Screen *screen, int x, int y, const char *format, ...);

void widget_draw_vline(Screen *screen, int x, RGB *fg, RGB *bg);

void widget_draw_hline(Screen *screen, int x, RGB *fg, RGB *bg);

int pos_at_margin(int total_size, int margin);
