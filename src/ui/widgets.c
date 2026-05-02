#include "ui/widgets.h"
#include "module.h"
#include "text/utf8.h"
#include "ui/display.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_WIDGET_LIMIT 8

int __widgets_init(void)
{
	G_WIDGET_BUFFER = calloc(1, sizeof(WidgetBuffer));
	if (!G_WIDGET_BUFFER)
		return -1;
	G_WIDGET_BUFFER->widgets = calloc(DEFAULT_WIDGET_LIMIT, sizeof(GenericWidget));
	if (!G_WIDGET_BUFFER->widgets) {
		free(G_WIDGET_BUFFER);
		G_WIDGET_BUFFER = NULL;
		return -1;
	}
	G_WIDGET_BUFFER->count = 0;
	G_WIDGET_BUFFER->cap = DEFAULT_WIDGET_LIMIT;
	return 0;
}
APP_INIT(__widgets_init);

int __widgets_exit(void)
{
	if (G_WIDGET_BUFFER) {
		if (G_WIDGET_BUFFER->widgets) {
			for (size_t i = 0; i < G_WIDGET_BUFFER->count; i++) {
				GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
				if (gw->type == TYPE_INPUT && gw->data.input.text) {
					free(gw->data.input.text);
				}
			}
			free(G_WIDGET_BUFFER->widgets);
		}
		free(G_WIDGET_BUFFER);
	}
	G_WIDGET_BUFFER = NULL;
	return 0;
}
APP_EXIT(__widgets_exit);

static void set_cell_inline(int x, int y, uint32_t cp, RGB *fg, RGB *bg)
{
	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;
	int idx = y * screen->width + x;
	if (x >= 0 && x < screen->width && y >= 0 && y < screen->height) {
		screen->cells[idx].cp = cp;
		screen->cells[idx].fg = fg_color;
		screen->cells[idx].bg = bg_color;
		screen->cells[idx].dirty = true;
		screen->cells[idx].wide = false;
		screen->cells[idx].wide_cont = false;
	}
}

static GenericWidget *widget_buffer_alloc(void)
{
	if (G_WIDGET_BUFFER == NULL) {
		__widgets_init();
	}
	if (G_WIDGET_BUFFER->count >= G_WIDGET_BUFFER->cap) {
		size_t new_cap = (G_WIDGET_BUFFER->cap == 0) ? DEFAULT_WIDGET_LIMIT : G_WIDGET_BUFFER->cap * 2;
		void *temp = realloc(G_WIDGET_BUFFER->widgets, sizeof(GenericWidget) * new_cap);
		if (temp == NULL) {
			fprintf(stderr, "Out of memory!\n");
			return NULL;
		}
		G_WIDGET_BUFFER->widgets = (GenericWidget *)temp;
		G_WIDGET_BUFFER->cap = new_cap;
	}
	return &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->count++];
}

void widget_draw_vline(Screen *screen, int x, RGB *fg, RGB *bg)
{
	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

#define SET_CELL_INLINE(_x, _y, _cp)                                                           \
	do {                                                                                   \
		if ((_x) >= 0 && (_x) < screen->width && (_y) >= 0 && (_y) < screen->height) { \
			int idx = (_y) * screen->width + (_x);                                 \
			screen->cells[idx].cp = (_cp);                                         \
			screen->cells[idx].fg = fg_color;                                      \
			screen->cells[idx].bg = bg_color;                                      \
			screen->cells[idx].dirty = true;                                       \
			screen->cells[idx].wide = false;                                       \
			screen->cells[idx].wide_cont = false;                                  \
		}                                                                              \
	} while (0)

	int start_x = (x > screen->width) ? screen->width : x;
	start_x = (start_x < 0) ? 0 : start_x;
	for (int y = 0; y < screen->height; y++) {
		SET_CELL_INLINE(start_x, y, 0x2502);
	}

#undef SET_CELL_INLINE
}

void widget_draw_hline(Screen *screen, int y, RGB *fg, RGB *bg)
{
	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

#define SET_CELL_INLINE(_x, _y, _cp)                                                           \
	do {                                                                                   \
		if ((_x) >= 0 && (_x) < screen->width && (_y) >= 0 && (_y) < screen->height) { \
			int idx = (_y) * screen->width + (_x);                                 \
			screen->cells[idx].cp = (_cp);                                         \
			screen->cells[idx].fg = fg_color;                                      \
			screen->cells[idx].bg = bg_color;                                      \
			screen->cells[idx].dirty = true;                                       \
			screen->cells[idx].wide = false;                                       \
			screen->cells[idx].wide_cont = false;                                  \
		}                                                                              \
	} while (0)

	for (int x = 0; x <= screen->width; x++) {
		SET_CELL_INLINE(x, y, 0x2500);
	}

#undef SET_CELL_INLINE
}

BoxLTRB *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;
	BoxLTRB *ret = &gw->data.box;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	set_cell_inline(left, top, 0x250C, &fg_color, &bg_color);
	set_cell_inline(right, top, 0x2510, &fg_color, &bg_color);
	set_cell_inline(left, bottom, 0x2514, &fg_color, &bg_color);
	set_cell_inline(right, bottom, 0x2518, &fg_color, &bg_color);
	ret->left = left;
	ret->top = top;
	ret->right = right;
	ret->bottom = bottom;

	for (int x = left + 1; x < right; x++) {
		set_cell_inline(x, top, 0x2500, &fg_color, &bg_color);
		set_cell_inline(x, bottom, 0x2500, &fg_color, &bg_color);
	}

	for (int y = top + 1; y < bottom; y++) {
		set_cell_inline(left, y, 0x2502, &fg_color, &bg_color);
		set_cell_inline(right, y, 0x2502, &fg_color, &bg_color);
	}

	return ret;
}

InputLine *widget_create_inputline(BoxLTRB *parent)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_INPUT;
	InputLine *ret = &gw->data.input;
	if (parent == NULL)
		return NULL;
	ret->parent = parent;
	ret->row = 0;
	ret->text = calloc(32, sizeof(char));
	ret->p = ret->text;
	return ret;
}

void widget_draw_inputline(InputLine *input, RGB *fg, RGB *bg)
{
	int input_left = input->parent->left + 1;
	int input_ceiling = (input->parent->top + 1);
	int input_row = (input_ceiling + input->row < input->parent->bottom) ? input_ceiling + input->row : input->parent->bottom - 1;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	set_cell_inline(input_left, input_row, '$', &fg_color, &bg_color);
}

BoxLTRB *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;
	BoxLTRB *ret = &gw->data.box;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	int left = (x < 0) ? 0 : x;
	int right = (x + w > screen->width) ? screen->width : x + w - 1;
	int top = (y < 0) ? 0 : y;
	int bottom = (y + h > screen->height) ? screen->height : y + h - 1;
	ret->left = left;
	ret->top = top;
	ret->right = right;
	ret->bottom = bottom;

#define SET_CELL_INLINE(_x, _y, _cp)                                                           \
	do {                                                                                   \
		if ((_x) >= 0 && (_x) < screen->width && (_y) >= 0 && (_y) < screen->height) { \
			int idx = (_y) * screen->width + (_x);                                 \
			screen->cells[idx].cp = (_cp);                                         \
			screen->cells[idx].fg = fg_color;                                      \
			screen->cells[idx].bg = bg_color;                                      \
			screen->cells[idx].dirty = true;                                       \
			screen->cells[idx].wide = false;                                       \
			screen->cells[idx].wide_cont = false;                                  \
		}                                                                              \
	} while (0)

	SET_CELL_INLINE(left, top, 0x250C);
	SET_CELL_INLINE(right, top, 0x2510);
	SET_CELL_INLINE(left, bottom, 0x2514);
	SET_CELL_INLINE(right, bottom, 0x2518);

	for (int x = left + 1; x < right; x++) {
		SET_CELL_INLINE(x, top, 0x2500);
		SET_CELL_INLINE(x, bottom, 0x2500);
	}

	for (int y = top + 1; y < bottom; y++) {
		SET_CELL_INLINE(left, y, 0x2502);
		SET_CELL_INLINE(right, y, 0x2502);
	}

#undef SET_CELL_INLINE
	return ret;
}

void widget_write_text(Screen *screen, int x, int y, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int len = vsnprintf(NULL, 0, format, args);
	va_end(args);

	if (len < 0)
		return;

	if (len < 0)
		return;
	char *buffer = calloc(1, sizeof(char) * (len + 1));
	va_start(args, format);
	vsnprintf(buffer, len + 1, format, args);
	va_end(args);

	FILE *fp = fopen("./log.txt", "a");
	{
		int x = 0;
		for (const char *p = buffer; p < (buffer + len); p++) {
			uint32_t cp = { 0 };
			utf8_decode(p, &cp);
			set_cell_inline(x, 0, cp, NULL, NULL);
			fprintf(fp, "%c", cp);
			++x;
		}
		screen_flush(screen);
	}
	fclose(fp);

	free(buffer);
}

int pos_at_margin(int total_size, int margin)
{
	return (total_size - 1) - margin;
}
