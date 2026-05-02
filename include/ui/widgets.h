#pragma once
#include "ui/display.h"
#include <stdint.h>

struct InputBox;
struct BoxLTRB;

typedef struct BoxLTRB {
	int left;
	int right;
	int top;
	int bottom;
	struct InputBox *child;
} BoxLTRB;

typedef struct InputBox {
	struct BoxLTRB *parent;
	int row;
	char *text;
	char *p;
} InputBox;

typedef enum {
	TYPE_BOX = 0,
	TYPE_INPUT = 1
} WidgetType;

typedef struct GenericWidget {
	int *type;
	union {
		InputBox *input;
		BoxLTRB *box;
	} data;
} GenericWidget;

typedef struct BoxBuffer {
	GenericWidget **widget;
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
