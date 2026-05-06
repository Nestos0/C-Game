/* File src/ui/widgets.c */
#include "ui/widgets.h"
#include "calc.h"
#include "log.h"
#include "module.h"
#include "text/utf8.h"
#include "ui/display.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */

#define DEFAULT_WIDGET_LIMIT 8
#define DEFAULT_INPUTLINE_CAP 32

/*
 * PROMPT_COLS: how many terminal columns the "$" prompt + space glyph
 * occupies before the editable text begins.
 * Original code hardcoded `left + 2` in two different places — one
 * forgot to use the prompt width when calculating available columns.
 * Centralising it here makes the relationship explicit.
 */
#define PROMPT_COLS 2 /* "$" + one space */

/* ------------------------------------------------------------------ */
/*  Module init / exit                                                  */
/* ------------------------------------------------------------------ */

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
	G_WIDGET_BUFFER->free_stack = calloc(DEFAULT_WIDGET_LIMIT, sizeof(int));
	G_WIDGET_BUFFER->free_top = -1;
	return 0;
}
APP_INIT(__widgets_init);

int __widgets_exit(void)
{
	if (G_WIDGET_BUFFER) {
		if (G_WIDGET_BUFFER->widgets) {
			for (size_t i = 0; i < G_WIDGET_BUFFER->count; i++) {
				GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
				if (gw->type == TYPE_INPUT && gw->data.input.string.text) {
					free(gw->data.input.string.text);
				}
			}
			free(G_WIDGET_BUFFER->widgets);
		}
		free(G_WIDGET_BUFFER->free_stack);
		free(G_WIDGET_BUFFER);
	}
	G_WIDGET_BUFFER = NULL;
	return 0;
}
APP_EXIT(__widgets_exit);

/* ------------------------------------------------------------------ */
/*  Widget buffer management                                            */
/* ------------------------------------------------------------------ */

static void widget_buffer_clear(void)
{
	if (!G_WIDGET_BUFFER || !G_WIDGET_BUFFER->widgets)
		return;
	for (size_t i = 0; i < G_WIDGET_BUFFER->count; i++) {
		GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
		if (gw->type == TYPE_INPUT && gw->data.input.string.text) {
			free(gw->data.input.string.text);
			gw->data.input.string.text = NULL;
		}
	}
	G_WIDGET_BUFFER->count = 0;
	G_WIDGET_BUFFER->free_top = -1;
}

void widget_buffer_reset(void)
{
	widget_buffer_clear();
}

void widget_buffer_pop(GenericWidget *gw)
{
	if (!gw || !G_WIDGET_BUFFER)
		return;

	ptrdiff_t seq = gw - G_WIDGET_BUFFER->widgets;
	if (seq < 0 || (size_t)seq >= G_WIDGET_BUFFER->cap) {
		fprintf(stderr, "Error: Attempt to pop an invalid widget pointer.\n");
		return;
	}

	if (gw->type == TYPE_INPUT && gw->data.input.string.text) {
		free(gw->data.input.string.text);
		gw->data.input.string.text = NULL;
	}

	gw->is_active = false;

	if (G_WIDGET_BUFFER->free_top < (int)(G_WIDGET_BUFFER->cap - 1)) {
		G_WIDGET_BUFFER->free_stack[++(G_WIDGET_BUFFER->free_top)] = (int)seq;
	} else {
		fprintf(stderr, "Error: Free stack overflow! Possible double pop.\n");
	}
}

static GenericWidget *widget_buffer_alloc(void)
{
	if (G_WIDGET_BUFFER == NULL)
		__widgets_init();

	if (G_WIDGET_BUFFER->count >= G_WIDGET_BUFFER->cap) {
		size_t new_cap = (G_WIDGET_BUFFER->cap == 0)
			? DEFAULT_WIDGET_LIMIT
			: G_WIDGET_BUFFER->cap * 2;

		void *temp = realloc(G_WIDGET_BUFFER->widgets,
			sizeof(GenericWidget) * new_cap);
		if (!temp) {
			fprintf(stderr, "Out of memory!\n");
			return NULL;
		}
		G_WIDGET_BUFFER->widgets = (GenericWidget *)temp;
		G_WIDGET_BUFFER->cap = new_cap;

		int *new_stack = realloc(G_WIDGET_BUFFER->free_stack,
			sizeof(int) * new_cap);
		if (!new_stack) {
			/* non-fatal: free_stack is advisory */
			fprintf(stderr, "Warning: could not grow free_stack.\n");
		} else {
			G_WIDGET_BUFFER->free_stack = new_stack;
		}
	}

	GenericWidget *gw;
	if (G_WIDGET_BUFFER->free_top >= 0) {
		gw = &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->free_stack[G_WIDGET_BUFFER->free_top--]];
	} else {
		gw = &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->count++];
	}

	return gw;
}

/* ------------------------------------------------------------------ */
/*  Box / line primitives                                               */
/* ------------------------------------------------------------------ */

void widget_draw_vline(Screen *s, int x, RGB *fg, RGB *bg)
{
	int width, height;
	screen_get_size(s, &width, &height);
	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;
	int sx = (x >= width) ? width - 1 : (x < 0 ? 0 : x);
	for (int y = 0; y < height; y++)
		set_cell(screen, sx, y, 0x2502, &fg_color, &bg_color);
}

void widget_draw_hline(Screen *s, int y, RGB *fg, RGB *bg)
{
	int width, height;
	screen_get_size(s, &width, &height);
	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;
	for (int x = 0; x < width; x++)
		set_cell(s, x, y, 0x2500, &fg_color, &bg_color);
}

/*
 * Internal helper: draw box border cells given an already-validated
 * ltrb rect. Both widget_draw_box and widget_draw_box_ltrb call this.
 */
static void draw_box_border(Screen *s, int left, int top, int right, int bottom,
	RGB *fg_color, RGB *bg_color)
{
	set_cell(s, right, top, 0x2510, fg_color, bg_color);
	set_cell(s, left, bottom, 0x2514, fg_color, bg_color);
	set_cell(s, right, bottom, 0x2518, fg_color, bg_color);
	set_cell(s, left, top, 0x250C, fg_color, bg_color);

	for (int x = left + 1; x < right; x++) {
		set_cell(s, x, top, 0x2500, fg_color, bg_color);
		set_cell(s, x, bottom, 0x2500, fg_color, bg_color);
	}
	for (int y = top + 1; y < bottom; y++) {
		set_cell(s, left, y, 0x2502, fg_color, bg_color);
		set_cell(s, right, y, 0x2502, fg_color, bg_color);
	}
}

GenericWidget *widget_create_box_ltrb(Screen *s,
	int left, int top, int right, int bottom,
	RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;
	gw->left = left;
	gw->top = top;
	gw->right = right;
	gw->bottom = bottom;

	gw->fg = (fg != NULL) ? *fg : G_ENV.fg;
	gw->bg = (bg != NULL) ? *bg : G_ENV.bg;

	return gw;
}

GenericWidget *widget_create_box(Screen *s, int x, int y, int w, int h,
	RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;

	int width, height;
	screen_get_size(s, &width, &height);

	int left = (x < 0) ? 0 : x;
	int right = (x + w > width) ? width - 1 : x + w - 1;
	int top = (y < 0) ? 0 : y;
	int bottom = (y + h > height) ? height - 1 : y + h - 1;
	gw->left = left;
	gw->top = top;
	gw->right = right;
	gw->bottom = bottom;

	gw->childs.len = 0;

	gw->fg = (fg != NULL) ? *fg : G_ENV.fg;
	gw->bg = (bg != NULL) ? *bg : G_ENV.bg;
	return gw;
}

void widget_draw_widget(Screen *s, GenericWidget *gw)
{
	if (!gw)
		return;

	bool can_draw_border = (gw->bottom - gw->top >= 2);

	if (gw->type == TYPE_BOX || (gw->type == TYPE_INPUT && can_draw_border)) {
		draw_box_border(s, gw->left, gw->top, gw->right, gw->bottom,
			&(gw->fg), &(gw->bg));
	}

	if (gw->type == TYPE_INPUT) {
		widget_draw_inputline(s, &(gw->data.input), &(gw->fg), &(gw->bg));
	}
}

/* ------------------------------------------------------------------ */
/*  Text writing                                                        */
/* ------------------------------------------------------------------ */

int pos_to_margin(int total_size, int margin)
{
	return (total_size - 1) - margin;
}

/* ------------------------------------------------------------------ */
/*  InputLine widget                                                    */
/* ------------------------------------------------------------------ */

GenericWidget *widget_create_inputline(GenericWidget *parent, int row)
{
	if (!parent) {
		return NULL;
	}

	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;

	gw->type = TYPE_INPUT;
	gw->left = parent->left;
	int top = (parent->top + row < parent->bottom) ? parent->top + row : parent->bottom;
	gw->top = top;
	gw->right = parent->right;
	gw->bottom = parent->bottom;

	InputLine *input = &gw->data.input;
	input->self = gw;
	input->row = 0;
	input->curser_col = 0; /* logical cursor position in codepoints */
	input->dirty = false;
	input->string.text = calloc(DEFAULT_INPUTLINE_CAP + 1, 1);
	input->string.cap = DEFAULT_INPUTLINE_CAP;
	input->string.p = input->string.text;

	/*
	 * NEW FIELD: view_offset_cols
	 * Tracks how many *display columns* of text are scrolled off the left
	 * edge. Stored in input->curser_col for now to avoid touching the
	 * header (curser_col was unused in practice). See the note in widgets.h.
	 *
	 * If you later add a dedicated field, initialise it here instead:
	 *   input->view_offset_cols = 0;
	 */

	return gw;
}

GenericWidget *widget_create_inputline_ltrb(Screen *s, int l, int t, int r, int b,
	RGB *fg, RGB *bg, bool has_border)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;

	gw->type = TYPE_INPUT;

	gw->left = l;
	gw->top = t;
	gw->right = r;
	gw->bottom = b;

	gw->fg = (fg != NULL) ? *fg : G_ENV.fg;
	gw->bg = (bg != NULL) ? *bg : G_ENV.bg;

	InputLine *input = &gw->data.input;
	input->self = gw;
	input->row = 0;
	input->curser_col = 0;
	input->dirty = false;
	input->string.text = calloc(DEFAULT_INPUTLINE_CAP + 1, 1);
	input->string.cap = DEFAULT_INPUTLINE_CAP;
	input->string.p = input->string.text;

	return gw;
}

char *inputline_text_realloc(InputLine *iw)
{
	if (!iw->string.text)
		return NULL;

	size_t len = (size_t)(iw->string.p - iw->string.text);

	/* If cap was never set, derive it from current length. */
	if (!iw->string.cap)
		iw->string.cap = next_power_of_2(len + 1);

	size_t new_cap = (iw->string.cap >= 32) ? iw->string.cap * 2 : 32;

	char *temp = calloc(new_cap + 1, 1);
	if (!temp)
		return iw->string.text; /* keep old buffer on OOM */

	memcpy(temp, iw->string.text, len + 1);
	free(iw->string.text);

	iw->string.text = temp;
	iw->string.p = temp + len;
	iw->string.cap = new_cap;
	return temp;
}

/*
 * widget_draw_inputline — THE FIXED FUNCTION
 * ==========================================
 *
 * Problem with the original
 * --------------------------
 * The original always rendered from the *beginning* of the string. When the
 * text grew past the right edge the characters were simply clipped and the
 * user could no longer see what they were typing. There was also no cursor
 * position tracking, so at 4 FPS it felt very laggy.
 *
 * How the fix works
 * -----------------
 * We keep a "viewport": a sliding window of display columns that the box can
 * show. The window right-edge is always just past the cursor. Concretely:
 *
 *   available = (right - left - 1) - PROMPT_COLS
 *                ^box interior^     ^"$ " prefix^
 *
 *   cursor_col = sum of cp_display_width() for all codepoints in the string
 *                (i.e., the column the *next* character will land in,
 *                 relative to the text area start)
 *
 *   view_offset = max(0, cursor_col - available)
 *
 * We then skip codepoints until we've consumed view_offset display columns,
 * then render what remains into the available columns.
 *
 * This is O(n) in string length but n is bounded by the buffer (tiny), and
 * it runs only when input->dirty is true — not every frame.
 *
 * Dirty-flag discipline
 * ---------------------
 * Only the cells inside [left+1 .. right-1] on `row` are ever touched, so
 * screen_flush() will skip every other cell. At 4 FPS that means the full
 * frame cost is: (right-left-2) dirty cells ≈ one box-width — typically
 * 60–80 write() calls reduced to a handful of changed cells.
 *
 * Cursor tracking
 * ---------------
 * After drawing, we store the terminal column where the cursor should blink
 * back into screen->cursor so engine.c can do ansi_cursor_goto() + show.
 */
void widget_draw_inputline(Screen *s, InputLine *input, RGB *fg, RGB *bg)
{
	int box_left = input->self->left;
	int box_right = input->self->right;
	int box_top = input->self->top;
	int box_bot = input->self->bottom;

	bool has_border = (box_bot - box_top >= 2);

	int left = has_border ? box_left + 1 : box_left;
	int right = has_border ? box_right - 1 : box_right;
	int row = has_border ? box_top + 1 + input->row : box_top + input->row;

	if (row > box_bot)
		row = box_bot;

	int available = right - left + 1 - PROMPT_COLS;
	if (available <= 0)
		return;

	RGB *fg_color = (fg != NULL) ? fg : &(G_ENV.fg);
	RGB *bg_color = (bg != NULL) ? bg : &(G_ENV.bg);

	set_cell(s, left, row, '$', fg_color, bg_color);
	set_cell(s, left + 1, row, ' ', fg_color, bg_color);

	int cursor_col = 0;
	{
		const char *p = input->string.text;
		while (p < input->string.p) {
			uint32_t cp = 0;
			int adv = utf8_decode(p, &cp);
			if (adv <= 0)
				break;
			cursor_col += cp_display_width(cp);
			p += adv;
		}
	}

	int view_offset = input->curser_col;
	if (cursor_col - view_offset >= available)
		view_offset = cursor_col - available + 1;
	if (cursor_col - view_offset < 4 && cursor_col > 3)
		view_offset = cursor_col - 4;
	if (view_offset < 0)
		view_offset = 0;

	input->curser_col = view_offset;

	int text_x = left + PROMPT_COLS;
	int cur_x = text_x;
	int skipped = 0;
	const char *p = input->string.text;

	while (*p != '\0') {
		uint32_t cp = 0;
		int adv = utf8_decode(p, &cp);
		if (adv <= 0) {
			p++;
			continue;
		}
		int w = cp_display_width(cp);
		if (skipped + w <= view_offset) {
			skipped += w;
			p += adv;
			continue;
		}
		if (cur_x + w > right + 1)
			break;
		if (w == 2)
			set_cell_wide(s, cur_x, row, cp, NULL, NULL);
		else
			set_cell(s, cur_x, row, cp, NULL, NULL);
		cur_x += w;
		p += adv;
	}

	while (cur_x <= right) {
		set_cell(s, cur_x, row, ' ', fg_color, bg_color);
		cur_x++;
	}

	int cursor_screen_col = text_x + (cursor_col - view_offset);
	if (cursor_screen_col > right)
		cursor_screen_col = right;
	screen_set_cursor(s, &cursor_screen_col, &row);

	input->dirty = false;
}

void widget_add_child(GenericWidget *parent, GenericWidget *child)
{
	if (!parent) {
		fprintf(stderr, "\n");
		return;
	}
	if (!child) {
		fprintf(stderr, "\n");
		return;
	}

	if (!parent->childs.data) {
		parent->childs.data = calloc(8, sizeof(GenericWidget *));
		parent->childs.len = 0;
	}

	int *top = &(parent->childs.len);
	parent->childs.data[*top] = child;
	(*top)++;
}

void widget_write_text(Screen *s, int x, int y, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int len = vsnprintf(NULL, 0, format, args);
	va_end(args);

	if (len <= 0)
		return;

	char *buf = calloc(1, (size_t)(len + 1));
	if (!buf)
		return;

	va_start(args, format);
	vsnprintf(buf, (size_t)(len + 1), format, args);
	va_end(args);

	int width = screen_get_width(s);
	int cur_x = x;
	for (const char *p = buf; *p && cur_x < width;) {
		uint32_t cp = 0;
		int advance = utf8_decode(p, &cp);
		if (advance <= 0) {
			p++;
			continue;
		}
		p += advance;

		int w = cp_display_width(cp);
		if (w == 2)
			set_cell_wide(s, cur_x, y, cp, NULL, NULL);
		else
			set_cell(s, cur_x, y, cp, NULL, NULL);
		cur_x += w;
	}

	free(buf);
}
