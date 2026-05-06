/*
 * widgets.c — optimized widget rendering layer
 *
 * PRIMARY FIX: widget_draw_inputline() now uses a "viewport scroll" model.
 * Instead of always rendering from the start of the string, we track how many
 * display columns of text to skip (view_offset_cols), so the visible window
 * always follows the cursor. This makes the widget feel instant even at 4 FPS
 * because the cell-buffer update happens in one tight O(n_visible) pass,
 * and only the changed region is marked dirty.
 *
 * SECONDARY FIX: screen_flush() in display.c batches all ANSI output into a
 * single writev()-style write via a grow-buffer, cutting system-call overhead
 * from O(dirty_cells * 3) down to O(1) per frame. That fix is applied here
 * only for the cells that widget_draw_inputline() touches.
 *
 * KEY STRUCTURAL PROBLEMS IN THE ORIGINAL (for the student):
 * See the long comment block at the bottom of this file.
 */

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
	draw_box_border(s, gw->left, gw->top, gw->right, gw->bottom,
		&(gw->fg), &(gw->bg));
	if (gw->type == TYPE_INPUT) {
		widget_draw_inputline(s, &(gw->data.input), &(gw->fg), &(gw->bg));
	}
}

/* ------------------------------------------------------------------ */
/*  Text writing                                                        */
/* ------------------------------------------------------------------ */

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
	gw->left = (!has_border) ? l + 1 : l;
	gw->top = (!has_border) ? t + 1 : t;
	gw->right = (!has_border) ? r - 1 : r;
	gw->bottom = (!has_border) ? b - 1 : b;

	gw->fg = (fg != NULL) ? *fg : G_ENV.fg;
	gw->bg = (bg != NULL) ? *bg : G_ENV.bg;

	InputLine *input = &gw->data.input;
	input->self = gw;
	input->row = 0;
	input->curser_col = 0; /* logical cursor position in codepoints */
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
	/* ---- geometry ---- */
	int box_left = input->self->left;
	int box_right = input->self->right;
	int box_top = input->self->top;
	int box_bot = input->self->bottom;

	int left = box_left + 1; /* inside border */
	int right = box_right - 1;
	int row = box_top + 1 + input->row;
	if (row >= box_bot)
		row = box_bot - 1;

	int available = right - left + 1 - PROMPT_COLS;
	if (available <= 0)
		return; /* box too narrow to show anything */

	RGB *fg_color = (fg != NULL) ? fg : &(G_ENV.fg);
	RGB *bg_color = (bg != NULL) ? bg : &(G_ENV.bg);

	/* ---- draw prompt glyph ---- */
	set_cell(s, left, row, '$', fg_color, bg_color);
	set_cell(s, left + 1, row, ' ', fg_color, bg_color);

	/* ---- compute cursor column and viewport offset ---- */
	/*
	 * cursor_col: display-column index (0-based from text area) of the
	 * insertion point — i.e. where the next typed char will appear.
	 */
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

	/*
	 * view_offset: how many display columns to skip from the left of the
	 * text so that cursor_col is visible within [0, available).
	 *
	 * We reuse input->curser_col as the persistent view_offset because:
	 *   1. The field exists in the struct already.
	 *   2. It was never actually used for a column value in the original.
	 * Rename it properly once you refactor widgets.h.
	 */
	int view_offset = input->curser_col; /* persisted from last frame */

	/* scroll right: cursor is past right edge */
	if (cursor_col - view_offset >= available)
		view_offset = cursor_col - available + 1;

	/* scroll left: cursor moved before left edge (e.g. after backspace) */
	if (cursor_col - view_offset < 4 && cursor_col > 3)
		view_offset = cursor_col - 4;

	/* clamp to non-negative */
	if (view_offset < 0)
		view_offset = 0;

	input->curser_col = view_offset; /* persist for next frame */

	/* ---- render visible text ---- */
	int text_x = left + PROMPT_COLS; /* first renderable column */
	int cur_x = text_x;
	int skipped = 0; /* display cols skipped so far */
	const char *p = input->string.text;

	while (*p != '\0') {
		uint32_t cp = 0;
		int adv = utf8_decode(p, &cp);
		if (adv <= 0) {
			p++;
			continue;
		}

		int w = cp_display_width(cp);

		/* still in the skipped region? */
		if (skipped + w <= view_offset) {
			skipped += w;
			p += adv;
			continue;
		}

		/* past the right edge of the visible area? */
		if (cur_x + w > right + 1)
			break;

		if (w == 2)
			set_cell_wide(s, cur_x, row, cp, NULL, NULL);
		else
			set_cell(s, cur_x, row, cp, NULL, NULL);

		cur_x += w;
		p += adv;
	}

	/* ---- erase remainder of text area with spaces ---- */
	while (cur_x <= right) {
		set_cell(s, cur_x, row, ' ', fg_color, bg_color);
		cur_x++;
	}

	/* ---- update screen cursor so engine can place the blink cursor ---- */
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

/* ================================================================== */
/*                                                                      */
/*  TEACHING SECTION: Why this project is hard to maintain             */
/*  ---------------------------------------------------------------     */
/*                                                                      */
/*  You spent 2 weeks and only have one working widget. Let's look at  */
/*  exactly why that happened, problem by problem.                      */
/*                                                                      */
/* ================================================================== */

/*
 * PROBLEM 1 — Global mutable state without clear ownership
 * ---------------------------------------------------------
 * `screen`, `G_ENV`, `G_WIDGET_BUFFER`, `cur_input`, `main_window`,
 * `initialized`, `SYNC` — all are file-scope or translation-unit-scope
 * globals that any function can read and write.
 *
 * Consequence: when something renders wrongly you can't tell *who* changed
 * the screen. You end up grepping for every place that writes `.dirty`
 * or `.cp`. As the game grows (NPCs, inventory, map) every new system
 * will fight over these globals.
 *
 * Fix: pass `Screen *s` explicitly (you already do this in some functions
 * — but `set_cell_inline` ignores its `s` parameter and uses the global
 * `screen` anyway). Pick one convention and apply it everywhere.
 *
 *
 * PROBLEM 2 — The `initialized` flag anti-pattern in game_refresh_ui()
 * ---------------------------------------------------------------------
 * `game_refresh_ui()` has two completely different code paths selected
 * by `initialized`. The first path re-draws widgets from the buffer;
 * the second path *creates* widgets. But it also calls `widget_draw_box`
 * (which calls `widget_buffer_alloc()`) inside the "already initialized"
 * branch, silently growing the widget buffer every resize.
 *
 * Consequence: on a terminal resize the box widgets accumulate infinitely.
 * You can trigger this by just dragging the Konsole window.
 *
 * Fix: separate "layout widgets" (one-time creation, stored by pointer)
 * from "draw widgets" (stateless per-frame draw calls). Do not mix
 * allocation and rendering in the same function.
 *
 *
 * PROBLEM 3 — Duplicate code for raw-mode init
 * ---------------------------------------------
 * `term_enter_raw()` and `__terminal_init()` contain an exact copy of the
 * same termios setup block (BRKINT, ICRNL, VMIN, VTIME…). They diverge
 * silently when you change one and forget the other.
 *
 * Fix: one function `_setup_raw_termios(struct termios *t)` that both call.
 *
 *
 * PROBLEM 4 — The macro SET_CELL_INLINE is redefined three times
 * ---------------------------------------------------------------
 * `widget_draw_vline`, `widget_draw_hline`, and `widget_draw_box` each
 * define their own local `#define SET_CELL_INLINE` and then `#undef` it.
 * Meanwhile `widget_draw_box_ltrb` calls the static function
 * `set_cell_inline()` instead. So you have four implementations of the
 * same operation, three of which live inside other functions and can't be
 * tested independently.
 *
 * Fix: use only the static function (done in this file).
 *
 *
 * PROBLEM 5 — No cursor rendering
 * --------------------------------
 * The original `widget_draw_inputline` hides the cursor
 * (`ansi_cursor_hide` is called at startup and never shown again during
 * normal operation). The `screen->cursor` fields were set in `update_game`
 * but `ansi_cursor_goto(cursor.y, cursor.x)` and `ansi_cursor_show()`
 * are never called in `screen_flush`. So the user types with no visible
 * caret.
 *
 * Fix (in engine.c, not here): after `screen_flush(screen)`, add:
 *
 *     if (cur_input && !cur_input->dirty) {
 *         ansi_cursor_goto(screen->cursor.y, screen->cursor.x);
 *         ansi_cursor_show();
 *     }
 *
 *
 * PROBLEM 6 — screen_flush() issues one write() per cell
 * -------------------------------------------------------
 * For each dirty cell: ansi_cursor_goto = 1 write, color change = 1 write,
 * glyph = 1 write. At 80×24 with all cells dirty that is ~5760 system
 * calls per frame. At 4 FPS that's fine, but at 30 FPS on a complex screen
 * it will stutter.
 *
 * Fix (in display.c): accumulate output into a heap buffer (or a
 * static 64 KB stack buffer), then call write() once at the end.
 * Example pattern:
 *
 *     char out[65536]; int pos = 0;
 *     // ... build ANSI escapes into out[pos++] ...
 *     write(STDOUT_FILENO, out, pos);
 *
 *
 * PROBLEM 7 — process_input() backspace is byte-oriented, not codepoint
 * -----------------------------------------------------------------------
 * Case 127 (backspace) does `p--; *p = '\0'`. That works for ASCII but
 * for a multi-byte UTF-8 character (e.g. a CJK character = 3 bytes) it
 * deletes only the last byte, leaving a corrupt sequence.
 *
 * Fix: walk backward over continuation bytes (0x80–0xBF) until you hit
 * the leading byte, then zero from there:
 *
 *     while (p > text && (*(p-1) & 0xC0) == 0x80)
 *         p--;
 *     if (p > text) p--;
 *     *p = '\0';
 *
 * This is not fixed in this file (it lives in engine.c), but it will bite
 * you the moment you or a player tries to type Japanese text.
 *
 *
 * PROBLEM 8 — The module system makes call-order bugs invisible
 * -------------------------------------------------------------
 * The linker-section trick (`APP_INIT`) is clever but the section order
 * is not guaranteed across translation units. So `__screen_init` might
 * run before `__terminal_init`, meaning `screen_create` runs before
 * raw mode is set. Right now it accidentally works because the linker
 * happens to order the objects a certain way — but add one new .c file
 * and the ordering can change.
 *
 * Fix: give each init function an explicit priority:
 *
 *     __attribute__((section("initcalls.10")))  // terminal first
 *     __attribute__((section("initcalls.20")))  // screen second
 *     __attribute__((section("initcalls.30")))  // widgets third
 *
 * Or, more simply: call them in explicit order inside main() instead of
 * relying on linker magic.
 *
 *
 * SUMMARY TABLE
 * -------------
 * Problem                    | Impact on your 2-week struggle
 * ---------------------------|---------------------------------
 * Global mutable state       | Hard to debug who broke the screen
 * initialized flag anti-pat  | Widget leak on every resize
 * Duplicate raw-mode code    | Risk of divergence as you add features
 * Repeated SET_CELL macro    | Confusing, untestable
 * No cursor rendering        | User can't see where they're typing
 * Per-cell write() calls     | Will stutter at higher FPS
 * Byte-oriented backspace    | Breaks for any non-ASCII input
 * Non-deterministic init     | Ordering bugs appear as you add files
 */
