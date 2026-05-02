#pragma once
#include "ui/display.h"
#include <stdint.h>

struct InputLine;
struct BoxLTRB;

typedef struct BoxLTRB {
	int left;
	int right;
	int top;
	int bottom;
	struct InputLine *child;
} BoxLTRB;

typedef struct InputLine {
	struct BoxLTRB *parent;
	int row;
	bool dirty : 1;
	char *text;
	char *start;
	char *end;
} InputLine;

typedef enum {
	TYPE_BOX = 0,
	TYPE_INPUT = 1
} WidgetType;

typedef struct GenericWidget {
	WidgetType type;
	union {
		BoxLTRB box;
		InputLine input;
	} data;
} GenericWidget;

typedef struct WidgetBuffer {
	GenericWidget *widgets;
	size_t count;
	size_t cap;
} WidgetBuffer;

extern WidgetBuffer *G_WIDGET_BUFFER;

BoxLTRB *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg);

BoxLTRB *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg);

void widget_write_text(Screen *screen, int x, int y, const char *format, ...);

void widget_draw_vline(Screen *screen, int x, RGB *fg, RGB *bg);

void widget_draw_hline(Screen *screen, int x, RGB *fg, RGB *bg);

int pos_at_margin(int total_size, int margin);

InputLine *widget_create_inputline(BoxLTRB *parent);
void widget_draw_inputline(InputLine *input, RGB *fg, RGB *bg);
