/* --- File: ./test/test.c --- */
// file: demo.c
#include <stdio.h>

typedef void (*initcall_t)(void);

/* 声明 section 边界（GNU ld / lld 自动提供） */
extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

/* module_init 宏 */
#define module_init(fn) \
    static initcall_t __init_##fn \
    __attribute__((section("initcalls"), used)) = fn;

/* -------- 测试函数 -------- */

void init_a(void) {
    printf("init_a called\n");
}
module_init(init_a);

void init_b(void) {
    printf("init_b called\n");
}
module_init(init_b);

/* -------- 主程序 -------- */

int main(void) {
    printf("running initcalls...\n");

    for (initcall_t *p = __start_initcalls;
         p < __stop_initcalls;
         ++p) {
        (*p)();
    }

    return 0;
}


/* --- File: ./main.c --- */
#include "core/engine.h"
#include "module.h"
#include "ui/display.h"
#include "ui/widgets.h"
#include <string.h>
#include <unistd.h>

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];
extern initcall_t __start_exitcalls[];
extern initcall_t __stop_exitcalls[];

struct GameState game_state;
struct Screen *screen;
struct Environment G_ENV;
WidgetBuffer *G_WIDGET_BUFFER;

int environment_init(void)
{
	G_ENV.fg = (struct RGB) { 239, 239, 227 };
	G_ENV.bg = (struct RGB) { 16, 16, 28 };
	return 0;
}
APP_INIT(environment_init);

int main(int argc, char *argv[])
{
	all_modules_init();

	game_state.is_running = true;
	for (int i = 0; i < argc; i++) {
		if (strcmp("--quit", argv[i]) == 0) {
			game_state.is_running = false;
		}
	}
	game_loop();

	all_modules_exit();
	return 0;
}


/* --- File: ./include/core/engine.h --- */
#pragma once

typedef struct GameState {
	bool is_running;
} GameState;

extern struct GameState game_state;

void game_loop();

int kbhit();

void process_input(void);

void update_game(void);

void init_game();

void game_refresh_ui();


/* --- File: ./include/log.h --- */
int log4engine(const char *file, const char *format, ...);


/* --- File: ./include/module.h --- */
typedef int (*initcall_t)(void);

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];
extern initcall_t __start_exitcalls[];
extern initcall_t __stop_exitcalls[];

#define APP_INIT(fn)                                                           \
	[[gnu::section("initcalls"), gnu::used, gnu::aligned(sizeof(void *))]] \
	static initcall_t __init_ptr_##fn = fn;

#define APP_EXIT(fn)                                                           \
	[[gnu::section("exitcalls"), gnu::used, gnu::aligned(sizeof(void *))]] \
	static initcall_t __exit_ptr_##fn = fn;

void all_modules_init();
void all_modules_exit();


/* --- File: ./include/ui/display.h --- */
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

typedef struct Screen {
	int width;
	int height;
	Cell *cells;
	struct {
		int x;
		int y;
	} cursor;
} Screen;

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


/* --- File: ./include/ui/terminal.h --- */
#pragma once
#include "ui/display.h"
#include <termios.h>

void get_terminal_size(int *width, int *height);

int term_restore();

bool term_enter_raw(void);

void term_leave_raw(void);

bool term_get_size(int *rows, int *cols);

void ansi_cursor_hide(void);

void ansi_cursor_show(void);

void ansi_clear_screen(void);

void ansi_cursor_goto(int row, int col);

void ansi_set_fg_rgb(int r, int g, int b);

void ansi_set_bg_rgb(int r, int g, int b);

void ansi_reset(void);

void term_set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg);


/* --- File: ./include/ui/widgets.h --- */
#pragma once
#include "ui/display.h"
#include <stdint.h>

struct InputLine;
struct BoxLTRB;

typedef struct BoxLTRB {
	RGB fg;
	RGB bg;
} BoxLTRB;

typedef struct InputLine {
	struct GenericWidget *self;
	int row;
	int curser_col;
	bool dirty : 1;
	struct {
		char *p;
		char *text;
		size_t cap;
	} string;
} InputLine;

typedef enum {
	TYPE_BOX = 0,
	TYPE_INPUT = 1
} WidgetType;

typedef struct GenericWidget {
	int left;
	int right;
	int top;
	int bottom;
	bool is_active : 1;
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
	int *free_stack;
	int free_top;
} WidgetBuffer;

extern WidgetBuffer *G_WIDGET_BUFFER;

GenericWidget *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg);

GenericWidget *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg);

void widget_write_text(Screen *screen, int x, int y, const char *format, ...);

void widget_draw_vline(Screen *screen, int x, RGB *fg, RGB *bg);

void widget_draw_hline(Screen *screen, int x, RGB *fg, RGB *bg);

int pos_at_margin(int total_size, int margin);

GenericWidget *widget_create_inputline(GenericWidget *parent);
void widget_draw_inputline(Screen *screen, InputLine *input, RGB *fg, RGB *bg);
char *inputline_text_realloc(InputLine *iw);
void widget_buffer_reset(void);

void widget_buffer_pop(GenericWidget *gw);


/* --- File: ./include/text/rich_text.h --- */
#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	BB_PARSE_STACK = 16,
	BB_INIT_CAP = 16,
	BB_DOC_DEFAULT_CAPACITY = 16,
	BB_TAG_NAME_MIN_CAP = 8,
	BB_TAG_LENGTH = 16,
	STRING_DYNAMIC_MIN_CAP = 128
};

typedef struct {
	const char *name;
	const char *code;
} AnsiMap;

// clang-format off
static const AnsiMap ANSI_MAP[] = {
    { "black",   "\x1b[30m" }, { "red",     "\x1b[31m" }, { "green",  "\x1b[32m" },
    { "yellow",  "\x1b[33m" }, { "blue",    "\x1b[34m" }, { "magenta", "\x1b[35m" },
    { "cyan",    "\x1b[36m" }, { "white",   "\x1b[37m" }, { "default", "\x1b[39m" },
    { "reset",   "\x1b[0m"  }, { "bold",    "\x1b[1m"  }, { "dim",     "\x1b[2m"  },
    { "italic",  "\x1b[3m"  }, { "underline","\x1b[4m" }, { "blink",   "\x1b[5m"  },
    { "reverse", "\x1b[7m"  }, { "strike",  "\x1b[9m"  }, { "bg_black","\x1b[40m" },
    { "bg_red",  "\x1b[41m" }, { "bg_green","\x1b[42m" }, { "bg_yellow","\x1b[43m"}
};
// clang-format on

const char *get_ansi_code(const char *name);

const char *get_ansi_code_safe(const char *name, size_t len);

int get_ansi_code_seq(const char *name);

int get_ansi_code_seq_safe(const char *name, size_t len);

char *bbcode_parse_stream(const char *format);

char *bbcode_interpret_spec(const char *format);

int printf_bbcode(const char *format, ...);


/* --- File: ./include/text/utf8.h --- */
#pragma once
#include <stdint.h>

int utf8_decode(const char *s, uint32_t *cp);

int utf8_encode(uint32_t cp, char *out);

int cp_display_width(uint32_t cp);


/* --- File: ./include/render/renderer.h --- */


/* --- File: ./include/calc.h --- */
#pragma once
#include <stddef.h>
size_t next_power_of_2(size_t n);


/* --- File: ./src/log.c --- */
#include <stdarg.h>
#include <stdio.h>

int log4engine(const char *file, const char *format, ...)
{
	FILE *fp = fopen(file, "a+");
	va_list args;

	va_start(args, format);
	vfprintf(fp, format, args);
	va_end(args);
	fclose(fp);
	return 0;
}


/* --- File: ./src/module.c --- */
#include "module.h"
#include <stdio.h>
#include <stdlib.h>

void all_modules_init()
{
	for (initcall_t *p = __start_initcalls;
		p < __stop_initcalls; p++) {
		int ret = (*p)();
		if (ret < 0) {
			fprintf(stderr, "\x1b[0K\x1b[0mError: Initcall at %p failed with code %d\n", (void *)*p, ret);
			exit(ret);
		}
	}
}

void all_modules_exit()
{
	for (initcall_t *p = __start_exitcalls;
		p < __stop_exitcalls; p++) {
		(*p)();
	}
}


/* --- File: ./src/ui/display.c --- */
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



/* --- File: ./src/ui/terminal.c --- */
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
	if (x > s->width)
		return;
	if (y > s->height)
		return;

	int idx = (y)*s->width + (x);
	s->cells[idx].cp = cp;
	/* RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg; */
	/* RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg; */

	s->cells[idx].cp = (cp);
	s->cells[idx].fg = (fg != NULL) ? *fg : G_ENV.fg;;
	s->cells[idx].bg = (bg != NULL) ? *bg : G_ENV.bg;;
	s->cells[idx].dirty = true;
	s->cells[idx].wide = false;
	s->cells[idx].wide_cont = false;
	s->cursor.x = x;
	s->cursor.y = y;

	if (cp_display_width(cp) > 1) {
		s->cells[idx].wide = true;
		s->cells[idx + 1].wide_cont = false;
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


/* --- File: ./src/ui/widgets.c --- */
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

#define DEFAULT_WIDGET_LIMIT 8
#define DEFAULT_INPUTLINE_CAP 32

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

static void widget_buffer_clear(void)
{
	if (!G_WIDGET_BUFFER || !G_WIDGET_BUFFER->widgets)
		return;
	for (size_t i = 0; i < G_WIDGET_BUFFER->count; i++) {
		GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
		if (gw->type == TYPE_INPUT && gw->data.input.string.text) {
			free(gw->data.input.string.text);
		}
	}
	G_WIDGET_BUFFER->count = 0;
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
		fprintf(stderr, "Error: Free stack overflow! Possileb double pop.\n");
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
		int *new_stack = realloc(G_WIDGET_BUFFER->free_stack, sizeof(int) * new_cap);
		G_WIDGET_BUFFER->free_stack = new_stack;
	}
	GenericWidget *gw;
	if (G_WIDGET_BUFFER->free_top >= 0) {
		gw = &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->free_top--];
	} else
		gw = &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->count++];
	if (!gw) {
		fprintf(stderr, "Out of memory!\n");
		return NULL;
	}
	return gw;
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

GenericWidget *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	set_cell_inline(left, top, 0x250C, &fg_color, &bg_color);
	set_cell_inline(right, top, 0x2510, &fg_color, &bg_color);
	set_cell_inline(left, bottom, 0x2514, &fg_color, &bg_color);
	set_cell_inline(right, bottom, 0x2518, &fg_color, &bg_color);
	gw->left = left;
	gw->top = top;
	gw->right = right;
	gw->bottom = bottom;

	for (int x = left + 1; x < right; x++) {
		set_cell_inline(x, top, 0x2500, &fg_color, &bg_color);
		set_cell_inline(x, bottom, 0x2500, &fg_color, &bg_color);
	}

	for (int y = top + 1; y < bottom; y++) {
		set_cell_inline(left, y, 0x2502, &fg_color, &bg_color);
		set_cell_inline(right, y, 0x2502, &fg_color, &bg_color);
	}

	return gw;
}

GenericWidget *widget_create_inputline(GenericWidget *window)
{
	if (window == NULL)
		return NULL;
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_INPUT;
	InputLine *input = &gw->data.input;
	gw->left = window->left;
	gw->top = window->top;
	gw->right = window->right;
	gw->bottom = window->bottom;
	input->self = gw;
	input->row = 0;
	input->string.text = calloc(DEFAULT_INPUTLINE_CAP + 1, sizeof(char));
	input->string.cap = DEFAULT_INPUTLINE_CAP;
	input->string.p = input->string.text;
	input->dirty = false;
	return gw;
}

char *inputline_text_realloc(InputLine *iw)
{
	if (iw->string.text) {
		size_t len = strlen(iw->string.text);
		if (!iw->string.cap) {
			size_t cap = next_power_of_2(len);
			iw->string.cap = cap;
		}
		size_t new_cap = (iw->string.cap >= 32) ? iw->string.cap * 2 : 32;
		char *temp = calloc(new_cap + 1, sizeof(char));
		if (!temp) {
			return iw->string.text;
		}
		memcpy(temp, iw->string.text, len + 1);
		free(iw->string.text);
		iw->string.text = temp;
		iw->string.p = temp + len;
		iw->string.cap = new_cap;
	}
	return iw->string.text;
}

void widget_draw_inputline(Screen *screen, InputLine *input, RGB *fg, RGB *bg)
{
	int left = input->self->left + 1;
	int right = input->self->right - 1;
	int ceiling = input->self->top + 1;
	int row = (ceiling + input->row < input->self->bottom) ? ceiling + input->row : input->self->bottom - 1;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	set_cell_inline(left, row, '$', &fg_color, &bg_color);

	int cur_x = left + 2;
	const char *p = input->string.text;
	while (*p != '\0' && cur_x <= right) {
		uint32_t cp = { 0 };
		p += utf8_decode(p, &cp);
		int width = cp_display_width(cp);

		if (cur_x + width > right + 1)
			break;

		set_cell_inline(cur_x, row, cp, NULL, NULL);
		cur_x += width;
	}

	while (cur_x <= right) {
		set_cell_inline(cur_x, row, ' ', &fg_color, &bg_color);
		cur_x++;
	}

	input->dirty = false;
}

GenericWidget *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	int left = (x < 0) ? 0 : x;
	int right = (x + w > screen->width) ? screen->width : x + w - 1;
	int top = (y < 0) ? 0 : y;
	int bottom = (y + h > screen->height) ? screen->height : y + h - 1;
	gw->left = left;
	gw->top = top;
	gw->right = right;
	gw->bottom = bottom;

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
	return gw;
}

void widget_write_text(Screen *screen, int x, int y, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int len = vsnprintf(NULL, 0, format, args);
	va_end(args);

	if (len < 0)
		return;

	char *buffer = calloc(1, sizeof(char) * (len + 1));
	va_start(args, format);
	vsnprintf(buffer, len + 1, format, args);
	va_end(args);

	for (const char *p = buffer; p < (buffer + len);) {
		uint32_t cp = { 0 };
		p += utf8_decode(p, &cp);
		set_cell_inline(x++, y, cp, NULL, NULL);
	}

	free(buffer);
}

int pos_at_margin(int total_size, int margin)
{
	return (total_size - 1) - margin;
}


/* --- File: ./src/text/rich_text.c --- */
#include "text/rich_text.h"

const char *get_ansi_code(const char *name)
{
	if (name == NULL)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

const char *get_ansi_code_safe(const char *name, size_t len)
{
	if (name == NULL)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strncmp(name, ANSI_MAP[i].name, len) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

int get_ansi_code_seq(const char *name)
{
	if (name == NULL)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return i;
	}
	return -1;
}

int get_ansi_code_seq_safe(const char *name, size_t len)
{
	if (name == NULL)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strncmp(name, ANSI_MAP[i].name, len) == 0)
			return i;
	}
	return -1;
}

char *bbcode_parse_stream(const char *format)
{
	size_t in_len = strlen(format);
	size_t out_cap = in_len * 2 + 64;
	char *ret = calloc(1, out_cap);
	if (!ret)
		return NULL;

	char stack[BB_PARSE_STACK][BB_TAG_LENGTH] = { 0 };
	int top = -1;

	const char *p = format;
	char *dest = ret;
	while (*p) {
		if (*p == '[') {
			const char *start = ++p;
			while (*p && *p != ']')
				++p;

			if (*p == ']') {
				size_t tag_len = p - start;
				if (tag_len < BB_TAG_LENGTH) {
					char tag[BB_TAG_LENGTH] = { 0 };
					strncpy(tag, start, tag_len);

					if (tag[0] != '/') {
						const char *code = get_ansi_code_safe(tag, tag_len);
						if (code[0] == '\0') {
							strncpy(dest, --start, tag_len + 2);
							dest += tag_len + 2;
						}

						if (code[0] != '\0') {
							strcpy(dest, code);
							dest += strlen(code);
							strcpy(stack[++top], tag);
						}
					} else {

						if (strcmp(stack[top], tag + 1) == 0) {
							--top;
							strcpy(dest, "\x1b[0m");
							dest += strlen("\x1b[0m");
						}
					}
				} else {
					--start;
					size_t len = p - start + 1;
					strncpy(dest, start, len);
					dest += len;
				}
			}

			++p;
			continue;
		}
		*dest++ = *p++;
	}

	strcpy(dest, "\x1b[0m");
	dest += strlen("\x1b[0m");

	return ret;
}

char *bbcode_interpret_spec(const char *format)
{
	char *ret = bbcode_parse_stream(format);
	return ret;
}

int printf_bbcode(const char *format, ...)
{
	int done;
	char *interpreted = bbcode_interpret_spec(format);

	va_list arg;
	va_start(arg, format);
	if (interpreted == NULL) {
		done = vfprintf(stdout, format, arg);
	} else
		done = vfprintf(stdout, interpreted, arg);
	va_end(arg);

	if (interpreted != NULL) {
		free(interpreted);
	}

	return done;
}



/* --- File: ./src/text/utf8.c --- */
#include "module.h"
#include <locale.h>
#include <text/utf8.h>

int utf8_decode(const char *s, uint32_t *cp)
{
	unsigned char c = (unsigned char)s[0];
	if (c == 0)
		return 0;

	if (c < 0x80) {
		*cp = c;
		return 1;
	}

	if ((c & 0xE0) == 0xC0) {
		if ((s[1] & 0xC0) != 0x80)
			return -1;
		*cp = ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
		return 2;
	}

	if ((c & 0xF0) == 0xE0) {
		if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80)
			return -1;
		*cp = ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) | (uint32_t)(s[2] & 0x3F);
		return 3;
	}

	if ((c & 0xF8) == 0xF0) {
		if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
			return -1;
		*cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(s[1] & 0x3F) << 12) | ((uint32_t)(s[2] & 0x3F) << 6) | (uint32_t)(s[3] & 0x3F);
		return 4;
	}

	return -1;
}

int utf8_encode(uint32_t cp, char *out)
{
	if (cp < 0x80) {
		out[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	if (cp < 0x110000) {
		out[0] = (char)(0xF0 | (cp >> 18));
		out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[3] = (char)(0x80 | (cp & 0x3F));
		return 4;
	}
	return -1;
}

int __utf8_init(void)
{
	setlocale(LC_ALL, "");
	return 0;
}
APP_INIT(__utf8_init);

int __utf8_uninit(void)
{
	return 0;
}
APP_INIT(__utf8_uninit);

static int in_range_table(uint32_t cp);

int cp_display_width(uint32_t cp)
{
	if (cp == 0 || cp == 0x200B || cp == 0xFEFF || cp == 0x2060)
		return 0;
	if (cp < 0x20)
		return 0;
	if (cp < 0x7F)
		return 1;
	if (cp >= 0x7F && cp < 0xA0)
		return 0;
	if (cp == 0x00AD)
		return 0;
	if (cp >= 0x3000 && cp <= 0x3000)
		return 2;

	if (in_range_table(cp))
		return 2;
	return 1;
}

// clang-format off
static const uint32_t wide_tbl[] = {
	0x1100,	 0x115F,  0x231A,  0x231B,  0x2329,  0x232A,  0x23E9,  0x23F3,
	0x23F8,	 0x23FA,  0x25FD,  0x25FE,  0x2614,  0x2615,  0x2648,  0x2653,
	0x267F,	 0x267F,  0x2693,  0x2693,  0x26A1,  0x26A1,  0x26AA,  0x26AB,
	0x26BD,	 0x26BE,  0x26C4,  0x26C5,  0x26CE,  0x26CE,  0x26D4,  0x26D4,
	0x26EA,	 0x26EA,  0x26F2,  0x26F3,  0x26F5,  0x26F5,  0x26FA,  0x26FA,
	0x26FD,	 0x26FD,  0x2702,  0x2702,  0x2705,  0x2705,  0x2708,  0x270D,
	0x270F,	 0x270F,  0x2712,  0x2712,  0x2714,  0x2714,  0x2716,  0x2716,
	0x271D,	 0x271D,  0x2721,  0x2721,  0x2728,  0x2728,  0x2733,  0x2734,
	0x2744,	 0x2744,  0x2747,  0x2747,  0x274C,  0x274C,  0x274E,  0x274E,
	0x2753,	 0x2755,  0x2757,  0x2757,  0x2763,  0x2764,  0x2795,  0x2797,
	0x27A1,	 0x27A1,  0x27B0,  0x27B0,  0x27BF,  0x27BF,  0x2934,  0x2935,
	0x2B05,	 0x2B07,  0x2B1B,  0x2B1C,  0x2B50,  0x2B50,  0x2B55,  0x2B55,
	0x2E80,	 0x2EFF,  0x2F00,  0x2FDF,  0x2FF0,  0x2FFF,  0x3001,  0x303E,
	0x3041,	 0x3096,  0x3099,  0x30FF,  0x3105,  0x312F,  0x3131,  0x318E,
	0x3190,	 0x31E3,  0x31F0,  0x321E,  0x3220,  0x3247,  0x3250,  0xA4CF,
	0xA960,	 0xA97C,  0xAC00,  0xD7A3,  0xD7B0,  0xD7C6,  0xD7CB,  0xD7FB,
	0xF900,	 0xFAFF,  0xFE10,  0xFE19,  0xFE30,  0xFE6F,  0xFF01,  0xFF60,
	0xFFE0,	 0xFFE6,  0x1B000, 0x1B0FF, 0x1B100, 0x1B12F, 0x1F200, 0x1F2FF,
	0x20000, 0x2FFFD, 0x30000, 0x3FFFD,
};
// clang-format on

#define WIDE_TBL_PAIRS (sizeof(wide_tbl) / sizeof(wide_tbl[0]) / 2)

static int in_range_table(uint32_t cp)
{
	int lo = 0;
	int hi = (int)WIDE_TBL_PAIRS - 1;

	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		uint32_t r_lo = wide_tbl[mid * 2];
		uint32_t r_hi = wide_tbl[mid * 2 + 1];

		if (cp < r_lo)
			hi = mid - 1;
		else if (cp > r_hi)
			lo = mid + 1;
		else
			return 1;
	}
	return 0;
}



/* --- File: ./src/render/renderer.c --- */


/* --- File: ./src/core/engine.c --- */
#include "core/engine.h"
#include "log.h"
#include "module.h"
#include "text/rich_text.h"
#include "ui/display.h"
#include "ui/terminal.h"
#include "ui/widgets.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define TARGET_FPS 4
#define TARGET_FRAME_TIME (1000000 / TARGET_FPS)

bool SYNC = true;
struct InputLine *cur_input = NULL;
struct GenericWidget *main_window = NULL;
bool initialized = false;

static const struct {
	const char *name;
	RGB color;
} theme[] = {
	{ "bg", { 16, 16, 28 } },
	{ "fg", { 155, 155, 155 } },
	{ "border", { 155, 155, 155 } },
	{ "border_bg", { 16, 16, 28 } },
	{ "indicator", { 48, 172, 82 } },
};
#define THEME(name) theme_lookup(name, sizeof(theme) / sizeof(theme[0]))

static const RGB *theme_lookup(const char *name, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (strcmp(theme[i].name, name) == 0)
			return &theme[i].color;
	}
	return NULL;
}

void game_refresh_ui()
{
	widget_draw_box(screen, 0, 0, screen->width, screen->height, (RGB *)THEME("border"), (RGB *)THEME("border_bg")); // Create Windows border

	if (screen->width < 90) {
		if (initialized) {
			for (int i = 0; i < G_WIDGET_BUFFER->count; i++) {
				GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
				switch (gw->type) {
				case TYPE_INPUT:
					widget_draw_inputline(screen, &(gw->data.input), (RGB *)THEME("indicator"), NULL);
					break;
				case TYPE_BOX:
					widget_draw_box_ltrb(screen, gw->left, gw->top, gw->right, gw->bottom, NULL, NULL);
					break;
				}
			}
		} else {
			main_window = widget_draw_box(screen, 2, 1, screen->width - 4, screen->height - 10, NULL, NULL);
			widget_draw_box_ltrb(screen, 2, main_window->bottom + 1,
				pos_at_margin(screen->width, 2),
				pos_at_margin(screen->height, 1), NULL, NULL);
			GenericWidget *inputbox = widget_draw_box_ltrb(screen, main_window->left + 1, main_window->bottom - 3,
				main_window->right - 1, main_window->bottom - 1, NULL, NULL);
			GenericWidget *input = widget_create_inputline(inputbox);
			cur_input = &(input->data.input);

			widget_draw_inputline(screen, cur_input, (RGB *)THEME("indicator"), NULL);
		}
	} else {
		widget_draw_box(screen, 0, 0, screen->width, screen->height, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
		widget_draw_box_ltrb(screen, 2, 1, pos_at_margin(screen->width / 2, 2), (screen->height) - 2, NULL, NULL);
	}
}

int kbhit(void)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

void process_input(void)
{
	while (kbhit()) {
		char prev = getchar();
		if (prev == 4) {
			screen_clear();
			game_state.is_running = false;
			break;
		}
		if (cur_input && prev) {
			size_t len = strlen(cur_input->string.text);
			if (cur_input->string.cap - len <= 2) {
				log4engine("log.txt", "REALLOC cur_input->string.text\n");
				cur_input->string.text = inputline_text_realloc(cur_input);
			}
			if (isprint(prev)) {
				*cur_input->string.p++ = prev;
				*cur_input->string.p = '\0';
			} else {
				switch (prev) {
				case 127:
					if (cur_input->string.p > cur_input->string.text) {
						cur_input->string.p--;
						*cur_input->string.p = '\0';
					} else
						*cur_input->string.p = '\0';
					break;
				case 13:
					int top = main_window->top + 1;
					for (int i = 3; i < main_window->right - 1; i++)
						term_set_cell(screen, i, top, 0, NULL, NULL);
					widget_write_text(screen, 3, top, "%s", cur_input->string.text);
					cur_input->string.p = cur_input->string.text;
					*cur_input->string.p = '\0';
				}
			}
			cur_input->dirty = true;
		}
	}
}

void update_game(void)
{
	int width, height = 0;
	term_get_size(&height, &width);
	if (screen->height != height || screen->width != width) {
		Screen *s = screen_create(width, height);
		if (!s)
			return;

		screen_destroy(screen);
		screen = s;
		for (size_t y = 0; y < s->height; y++) {
			for (size_t x = 0; x < s->width; x++) {
				size_t idx = y * s->width + x;
				s->cells[idx].bg = *THEME("bg");
				s->cells[idx].dirty = true;
			}
		}
		game_refresh_ui();
	}
	if (cur_input && cur_input->dirty) {
		widget_draw_inputline(screen, cur_input, (RGB *)THEME("indicator"), NULL);
		screen->cursor.x = cur_input->self->left;
		screen->cursor.y = cur_input->self->top + cur_input->row;
	}
}

long long get_microtime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void init_game()
{
	screen_clear();
	screen_set_fg(screen, get_anti_color(*THEME("bg")));
	screen_set_bg(screen, *THEME("bg"));
	game_refresh_ui();

	initialized = true;

	screen_flush(screen);
}

void game_loop()
{
	init_game();
	/* long long last_frame_time = get_microtime(); */
	/* float delta_time = 0.0f; */
	fflush(stdout);
	do {
		long long frame_start = get_microtime();
		process_input();
		update_game();
		screen_flush(screen);

		if (SYNC) {
			long long frame_end = get_microtime();
			long long actual_frame_duration = frame_end - frame_start;

			if (actual_frame_duration < TARGET_FRAME_TIME) {
				usleep(TARGET_FRAME_TIME - actual_frame_duration);
			}
		}
	} while (game_state.is_running);
}


/* --- File: ./src/calc.c --- */
#include "calc.h"
#include <stddef.h>

size_t next_power_of_2(size_t n)
{
	if (n == 0)
		return 1;
	n--; // 如果 n 本身是 2 的幂，减 1 可以保证结果还是 n
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF // 如果是 64 位系统
	n |= n >> 32;
#endif
	return n + 1;
}


/* --- File: ./merged_all.c --- */
/* START: ./test/test.c */
// file: demo.c
#include <stdio.h>

typedef void (*initcall_t)(void);

/* 声明 section 边界（GNU ld / lld 自动提供） */
extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

/* module_init 宏 */
#define module_init(fn) \
    static initcall_t __init_##fn \
    __attribute__((section("initcalls"), used)) = fn;

/* -------- 测试函数 -------- */

void init_a(void) {
    printf("init_a called\n");
}
module_init(init_a);

void init_b(void) {
    printf("init_b called\n");
}
module_init(init_b);

/* -------- 主程序 -------- */

int main(void) {
    printf("running initcalls...\n");

    for (initcall_t *p = __start_initcalls;
         p < __stop_initcalls;
         ++p) {
        (*p)();
    }

    return 0;
}
/* END: ./test/test.c */
/* START: ./main.c */
#include "core/engine.h"
#include "module.h"
#include "ui/display.h"
#include "ui/widgets.h"
#include <string.h>
#include <unistd.h>

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];
extern initcall_t __start_exitcalls[];
extern initcall_t __stop_exitcalls[];

struct GameState game_state;
struct Screen *screen;
struct Environment G_ENV;
WidgetBuffer *G_WIDGET_BUFFER;

int environment_init(void)
{
	G_ENV.fg = (struct RGB) { 239, 239, 227 };
	G_ENV.bg = (struct RGB) { 16, 16, 28 };
	return 0;
}
APP_INIT(environment_init);

int main(int argc, char *argv[])
{
	all_modules_init();

	game_state.is_running = true;
	for (int i = 0; i < argc; i++) {
		if (strcmp("--quit", argv[i]) == 0) {
			game_state.is_running = false;
		}
	}
	game_loop();

	all_modules_exit();
	return 0;
}
/* END: ./main.c */
/* START: ./include/core/engine.h */
#pragma once

typedef struct GameState {
	bool is_running;
} GameState;

extern struct GameState game_state;

void game_loop();

int kbhit();

void process_input(void);

void update_game(void);

void init_game();

void game_refresh_ui();
/* END: ./include/core/engine.h */
/* START: ./include/log.h */
int log4engine(const char *file, const char *format, ...);
/* END: ./include/log.h */
/* START: ./include/module.h */
typedef int (*initcall_t)(void);

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];
extern initcall_t __start_exitcalls[];
extern initcall_t __stop_exitcalls[];

#define APP_INIT(fn)                                                           \
	[[gnu::section("initcalls"), gnu::used, gnu::aligned(sizeof(void *))]] \
	static initcall_t __init_ptr_##fn = fn;

#define APP_EXIT(fn)                                                           \
	[[gnu::section("exitcalls"), gnu::used, gnu::aligned(sizeof(void *))]] \
	static initcall_t __exit_ptr_##fn = fn;

void all_modules_init();
void all_modules_exit();
/* END: ./include/module.h */
/* START: ./include/ui/display.h */
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

typedef struct Screen {
	int width;
	int height;
	Cell *cells;
	struct {
		int x;
		int y;
	} cursor;
} Screen;

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
/* END: ./include/ui/display.h */
/* START: ./include/ui/terminal.h */
#pragma once
#include "ui/display.h"
#include <termios.h>

void get_terminal_size(int *width, int *height);

int term_restore();

bool term_enter_raw(void);

void term_leave_raw(void);

bool term_get_size(int *rows, int *cols);

void ansi_cursor_hide(void);

void ansi_cursor_show(void);

void ansi_clear_screen(void);

void ansi_cursor_goto(int row, int col);

void ansi_set_fg_rgb(int r, int g, int b);

void ansi_set_bg_rgb(int r, int g, int b);

void ansi_reset(void);

void term_set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg);
/* END: ./include/ui/terminal.h */
/* START: ./include/ui/widgets.h */
#pragma once
#include "ui/display.h"
#include <stdint.h>

struct InputLine;
struct BoxLTRB;

typedef struct BoxLTRB {
	RGB fg;
	RGB bg;
} BoxLTRB;

typedef struct InputLine {
	struct GenericWidget *self;
	int row;
	int curser_col;
	bool dirty : 1;
	struct {
		char *p;
		char *text;
		size_t cap;
	} string;
} InputLine;

typedef enum {
	TYPE_BOX = 0,
	TYPE_INPUT = 1
} WidgetType;

typedef struct GenericWidget {
	int left;
	int right;
	int top;
	int bottom;
	bool is_active : 1;
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
	int *free_stack;
	int free_top;
} WidgetBuffer;

extern WidgetBuffer *G_WIDGET_BUFFER;

GenericWidget *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg);

GenericWidget *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg);

void widget_write_text(Screen *screen, int x, int y, const char *format, ...);

void widget_draw_vline(Screen *screen, int x, RGB *fg, RGB *bg);

void widget_draw_hline(Screen *screen, int x, RGB *fg, RGB *bg);

int pos_at_margin(int total_size, int margin);

GenericWidget *widget_create_inputline(GenericWidget *parent);
void widget_draw_inputline(Screen *screen, InputLine *input, RGB *fg, RGB *bg);
char *inputline_text_realloc(InputLine *iw);
void widget_buffer_reset(void);

void widget_buffer_pop(GenericWidget *gw);
/* END: ./include/ui/widgets.h */
/* START: ./include/text/rich_text.h */
#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	BB_PARSE_STACK = 16,
	BB_INIT_CAP = 16,
	BB_DOC_DEFAULT_CAPACITY = 16,
	BB_TAG_NAME_MIN_CAP = 8,
	BB_TAG_LENGTH = 16,
	STRING_DYNAMIC_MIN_CAP = 128
};

typedef struct {
	const char *name;
	const char *code;
} AnsiMap;

// clang-format off
static const AnsiMap ANSI_MAP[] = {
    { "black",   "\x1b[30m" }, { "red",     "\x1b[31m" }, { "green",  "\x1b[32m" },
    { "yellow",  "\x1b[33m" }, { "blue",    "\x1b[34m" }, { "magenta", "\x1b[35m" },
    { "cyan",    "\x1b[36m" }, { "white",   "\x1b[37m" }, { "default", "\x1b[39m" },
    { "reset",   "\x1b[0m"  }, { "bold",    "\x1b[1m"  }, { "dim",     "\x1b[2m"  },
    { "italic",  "\x1b[3m"  }, { "underline","\x1b[4m" }, { "blink",   "\x1b[5m"  },
    { "reverse", "\x1b[7m"  }, { "strike",  "\x1b[9m"  }, { "bg_black","\x1b[40m" },
    { "bg_red",  "\x1b[41m" }, { "bg_green","\x1b[42m" }, { "bg_yellow","\x1b[43m"}
};
// clang-format on

const char *get_ansi_code(const char *name);

const char *get_ansi_code_safe(const char *name, size_t len);

int get_ansi_code_seq(const char *name);

int get_ansi_code_seq_safe(const char *name, size_t len);

char *bbcode_parse_stream(const char *format);

char *bbcode_interpret_spec(const char *format);

int printf_bbcode(const char *format, ...);
/* END: ./include/text/rich_text.h */
/* START: ./include/text/utf8.h */
#pragma once
#include <stdint.h>

int utf8_decode(const char *s, uint32_t *cp);

int utf8_encode(uint32_t cp, char *out);

int cp_display_width(uint32_t cp);
/* END: ./include/text/utf8.h */
/* START: ./include/render/renderer.h */
/* END: ./include/render/renderer.h */
/* START: ./include/calc.h */
#pragma once
#include <stddef.h>
size_t next_power_of_2(size_t n);
/* END: ./include/calc.h */
/* START: ./src/log.c */
#include <stdarg.h>
#include <stdio.h>

int log4engine(const char *file, const char *format, ...)
{
	FILE *fp = fopen(file, "a+");
	va_list args;

	va_start(args, format);
	vfprintf(fp, format, args);
	va_end(args);
	fclose(fp);
	return 0;
}
/* END: ./src/log.c */
/* START: ./src/module.c */
#include "module.h"
#include <stdio.h>
#include <stdlib.h>

void all_modules_init()
{
	for (initcall_t *p = __start_initcalls;
		p < __stop_initcalls; p++) {
		int ret = (*p)();
		if (ret < 0) {
			fprintf(stderr, "\x1b[0K\x1b[0mError: Initcall at %p failed with code %d\n", (void *)*p, ret);
			exit(ret);
		}
	}
}

void all_modules_exit()
{
	for (initcall_t *p = __start_exitcalls;
		p < __stop_exitcalls; p++) {
		(*p)();
	}
}
/* END: ./src/module.c */
/* START: ./src/ui/display.c */
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

/* END: ./src/ui/display.c */
/* START: ./src/ui/terminal.c */
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
	if (x > s->width)
		return;
	if (y > s->height)
		return;

	int idx = (y)*s->width + (x);
	s->cells[idx].cp = cp;
	/* RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg; */
	/* RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg; */

	s->cells[idx].cp = (cp);
	s->cells[idx].fg = (fg != NULL) ? *fg : G_ENV.fg;;
	s->cells[idx].bg = (bg != NULL) ? *bg : G_ENV.bg;;
	s->cells[idx].dirty = true;
	s->cells[idx].wide = false;
	s->cells[idx].wide_cont = false;
	s->cursor.x = x;
	s->cursor.y = y;

	if (cp_display_width(cp) > 1) {
		s->cells[idx].wide = true;
		s->cells[idx + 1].wide_cont = false;
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
/* END: ./src/ui/terminal.c */
/* START: ./src/ui/widgets.c */
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

#define DEFAULT_WIDGET_LIMIT 8
#define DEFAULT_INPUTLINE_CAP 32

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

static void widget_buffer_clear(void)
{
	if (!G_WIDGET_BUFFER || !G_WIDGET_BUFFER->widgets)
		return;
	for (size_t i = 0; i < G_WIDGET_BUFFER->count; i++) {
		GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
		if (gw->type == TYPE_INPUT && gw->data.input.string.text) {
			free(gw->data.input.string.text);
		}
	}
	G_WIDGET_BUFFER->count = 0;
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
		fprintf(stderr, "Error: Free stack overflow! Possileb double pop.\n");
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
		int *new_stack = realloc(G_WIDGET_BUFFER->free_stack, sizeof(int) * new_cap);
		G_WIDGET_BUFFER->free_stack = new_stack;
	}
	GenericWidget *gw;
	if (G_WIDGET_BUFFER->free_top >= 0) {
		gw = &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->free_top--];
	} else
		gw = &G_WIDGET_BUFFER->widgets[G_WIDGET_BUFFER->count++];
	if (!gw) {
		fprintf(stderr, "Out of memory!\n");
		return NULL;
	}
	return gw;
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

GenericWidget *widget_draw_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	set_cell_inline(left, top, 0x250C, &fg_color, &bg_color);
	set_cell_inline(right, top, 0x2510, &fg_color, &bg_color);
	set_cell_inline(left, bottom, 0x2514, &fg_color, &bg_color);
	set_cell_inline(right, bottom, 0x2518, &fg_color, &bg_color);
	gw->left = left;
	gw->top = top;
	gw->right = right;
	gw->bottom = bottom;

	for (int x = left + 1; x < right; x++) {
		set_cell_inline(x, top, 0x2500, &fg_color, &bg_color);
		set_cell_inline(x, bottom, 0x2500, &fg_color, &bg_color);
	}

	for (int y = top + 1; y < bottom; y++) {
		set_cell_inline(left, y, 0x2502, &fg_color, &bg_color);
		set_cell_inline(right, y, 0x2502, &fg_color, &bg_color);
	}

	return gw;
}

GenericWidget *widget_create_inputline(GenericWidget *window)
{
	if (window == NULL)
		return NULL;
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_INPUT;
	InputLine *input = &gw->data.input;
	gw->left = window->left;
	gw->top = window->top;
	gw->right = window->right;
	gw->bottom = window->bottom;
	input->self = gw;
	input->row = 0;
	input->string.text = calloc(DEFAULT_INPUTLINE_CAP + 1, sizeof(char));
	input->string.cap = DEFAULT_INPUTLINE_CAP;
	input->string.p = input->string.text;
	input->dirty = false;
	return gw;
}

char *inputline_text_realloc(InputLine *iw)
{
	if (iw->string.text) {
		size_t len = strlen(iw->string.text);
		if (!iw->string.cap) {
			size_t cap = next_power_of_2(len);
			iw->string.cap = cap;
		}
		size_t new_cap = (iw->string.cap >= 32) ? iw->string.cap * 2 : 32;
		char *temp = calloc(new_cap + 1, sizeof(char));
		if (!temp) {
			return iw->string.text;
		}
		memcpy(temp, iw->string.text, len + 1);
		free(iw->string.text);
		iw->string.text = temp;
		iw->string.p = temp + len;
		iw->string.cap = new_cap;
	}
	return iw->string.text;
}

void widget_draw_inputline(Screen *screen, InputLine *input, RGB *fg, RGB *bg)
{
	int left = input->self->left + 1;
	int right = input->self->right - 1;
	int ceiling = input->self->top + 1;
	int row = (ceiling + input->row < input->self->bottom) ? ceiling + input->row : input->self->bottom - 1;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	set_cell_inline(left, row, '$', &fg_color, &bg_color);

	int cur_x = left + 2;
	const char *p = input->string.text;
	while (*p != '\0' && cur_x <= right) {
		uint32_t cp = { 0 };
		p += utf8_decode(p, &cp);
		int width = cp_display_width(cp);

		if (cur_x + width > right + 1)
			break;

		set_cell_inline(cur_x, row, cp, NULL, NULL);
		cur_x += width;
	}

	while (cur_x <= right) {
		set_cell_inline(cur_x, row, ' ', &fg_color, &bg_color);
		cur_x++;
	}

	input->dirty = false;
}

GenericWidget *widget_draw_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg)
{
	GenericWidget *gw = widget_buffer_alloc();
	if (!gw)
		return NULL;
	gw->type = TYPE_BOX;

	RGB fg_color = (fg != NULL) ? *fg : G_ENV.fg;
	RGB bg_color = (bg != NULL) ? *bg : G_ENV.bg;

	int left = (x < 0) ? 0 : x;
	int right = (x + w > screen->width) ? screen->width : x + w - 1;
	int top = (y < 0) ? 0 : y;
	int bottom = (y + h > screen->height) ? screen->height : y + h - 1;
	gw->left = left;
	gw->top = top;
	gw->right = right;
	gw->bottom = bottom;

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
	return gw;
}

void widget_write_text(Screen *screen, int x, int y, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int len = vsnprintf(NULL, 0, format, args);
	va_end(args);

	if (len < 0)
		return;

	char *buffer = calloc(1, sizeof(char) * (len + 1));
	va_start(args, format);
	vsnprintf(buffer, len + 1, format, args);
	va_end(args);

	for (const char *p = buffer; p < (buffer + len);) {
		uint32_t cp = { 0 };
		p += utf8_decode(p, &cp);
		set_cell_inline(x++, y, cp, NULL, NULL);
	}

	free(buffer);
}

int pos_at_margin(int total_size, int margin)
{
	return (total_size - 1) - margin;
}
/* END: ./src/ui/widgets.c */
/* START: ./src/text/rich_text.c */
#include "text/rich_text.h"

const char *get_ansi_code(const char *name)
{
	if (name == NULL)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

const char *get_ansi_code_safe(const char *name, size_t len)
{
	if (name == NULL)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strncmp(name, ANSI_MAP[i].name, len) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

int get_ansi_code_seq(const char *name)
{
	if (name == NULL)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return i;
	}
	return -1;
}

int get_ansi_code_seq_safe(const char *name, size_t len)
{
	if (name == NULL)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strncmp(name, ANSI_MAP[i].name, len) == 0)
			return i;
	}
	return -1;
}

char *bbcode_parse_stream(const char *format)
{
	size_t in_len = strlen(format);
	size_t out_cap = in_len * 2 + 64;
	char *ret = calloc(1, out_cap);
	if (!ret)
		return NULL;

	char stack[BB_PARSE_STACK][BB_TAG_LENGTH] = { 0 };
	int top = -1;

	const char *p = format;
	char *dest = ret;
	while (*p) {
		if (*p == '[') {
			const char *start = ++p;
			while (*p && *p != ']')
				++p;

			if (*p == ']') {
				size_t tag_len = p - start;
				if (tag_len < BB_TAG_LENGTH) {
					char tag[BB_TAG_LENGTH] = { 0 };
					strncpy(tag, start, tag_len);

					if (tag[0] != '/') {
						const char *code = get_ansi_code_safe(tag, tag_len);
						if (code[0] == '\0') {
							strncpy(dest, --start, tag_len + 2);
							dest += tag_len + 2;
						}

						if (code[0] != '\0') {
							strcpy(dest, code);
							dest += strlen(code);
							strcpy(stack[++top], tag);
						}
					} else {

						if (strcmp(stack[top], tag + 1) == 0) {
							--top;
							strcpy(dest, "\x1b[0m");
							dest += strlen("\x1b[0m");
						}
					}
				} else {
					--start;
					size_t len = p - start + 1;
					strncpy(dest, start, len);
					dest += len;
				}
			}

			++p;
			continue;
		}
		*dest++ = *p++;
	}

	strcpy(dest, "\x1b[0m");
	dest += strlen("\x1b[0m");

	return ret;
}

char *bbcode_interpret_spec(const char *format)
{
	char *ret = bbcode_parse_stream(format);
	return ret;
}

int printf_bbcode(const char *format, ...)
{
	int done;
	char *interpreted = bbcode_interpret_spec(format);

	va_list arg;
	va_start(arg, format);
	if (interpreted == NULL) {
		done = vfprintf(stdout, format, arg);
	} else
		done = vfprintf(stdout, interpreted, arg);
	va_end(arg);

	if (interpreted != NULL) {
		free(interpreted);
	}

	return done;
}

/* END: ./src/text/rich_text.c */
/* START: ./src/text/utf8.c */
#include "module.h"
#include <locale.h>
#include <text/utf8.h>

int utf8_decode(const char *s, uint32_t *cp)
{
	unsigned char c = (unsigned char)s[0];
	if (c == 0)
		return 0;

	if (c < 0x80) {
		*cp = c;
		return 1;
	}

	if ((c & 0xE0) == 0xC0) {
		if ((s[1] & 0xC0) != 0x80)
			return -1;
		*cp = ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
		return 2;
	}

	if ((c & 0xF0) == 0xE0) {
		if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80)
			return -1;
		*cp = ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) | (uint32_t)(s[2] & 0x3F);
		return 3;
	}

	if ((c & 0xF8) == 0xF0) {
		if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
			return -1;
		*cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(s[1] & 0x3F) << 12) | ((uint32_t)(s[2] & 0x3F) << 6) | (uint32_t)(s[3] & 0x3F);
		return 4;
	}

	return -1;
}

int utf8_encode(uint32_t cp, char *out)
{
	if (cp < 0x80) {
		out[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	if (cp < 0x110000) {
		out[0] = (char)(0xF0 | (cp >> 18));
		out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[3] = (char)(0x80 | (cp & 0x3F));
		return 4;
	}
	return -1;
}

int __utf8_init(void)
{
	setlocale(LC_ALL, "");
	return 0;
}
APP_INIT(__utf8_init);

int __utf8_uninit(void)
{
	return 0;
}
APP_INIT(__utf8_uninit);

static int in_range_table(uint32_t cp);

int cp_display_width(uint32_t cp)
{
	if (cp == 0 || cp == 0x200B || cp == 0xFEFF || cp == 0x2060)
		return 0;
	if (cp < 0x20)
		return 0;
	if (cp < 0x7F)
		return 1;
	if (cp >= 0x7F && cp < 0xA0)
		return 0;
	if (cp == 0x00AD)
		return 0;
	if (cp >= 0x3000 && cp <= 0x3000)
		return 2;

	if (in_range_table(cp))
		return 2;
	return 1;
}

// clang-format off
static const uint32_t wide_tbl[] = {
	0x1100,	 0x115F,  0x231A,  0x231B,  0x2329,  0x232A,  0x23E9,  0x23F3,
	0x23F8,	 0x23FA,  0x25FD,  0x25FE,  0x2614,  0x2615,  0x2648,  0x2653,
	0x267F,	 0x267F,  0x2693,  0x2693,  0x26A1,  0x26A1,  0x26AA,  0x26AB,
	0x26BD,	 0x26BE,  0x26C4,  0x26C5,  0x26CE,  0x26CE,  0x26D4,  0x26D4,
	0x26EA,	 0x26EA,  0x26F2,  0x26F3,  0x26F5,  0x26F5,  0x26FA,  0x26FA,
	0x26FD,	 0x26FD,  0x2702,  0x2702,  0x2705,  0x2705,  0x2708,  0x270D,
	0x270F,	 0x270F,  0x2712,  0x2712,  0x2714,  0x2714,  0x2716,  0x2716,
	0x271D,	 0x271D,  0x2721,  0x2721,  0x2728,  0x2728,  0x2733,  0x2734,
	0x2744,	 0x2744,  0x2747,  0x2747,  0x274C,  0x274C,  0x274E,  0x274E,
	0x2753,	 0x2755,  0x2757,  0x2757,  0x2763,  0x2764,  0x2795,  0x2797,
	0x27A1,	 0x27A1,  0x27B0,  0x27B0,  0x27BF,  0x27BF,  0x2934,  0x2935,
	0x2B05,	 0x2B07,  0x2B1B,  0x2B1C,  0x2B50,  0x2B50,  0x2B55,  0x2B55,
	0x2E80,	 0x2EFF,  0x2F00,  0x2FDF,  0x2FF0,  0x2FFF,  0x3001,  0x303E,
	0x3041,	 0x3096,  0x3099,  0x30FF,  0x3105,  0x312F,  0x3131,  0x318E,
	0x3190,	 0x31E3,  0x31F0,  0x321E,  0x3220,  0x3247,  0x3250,  0xA4CF,
	0xA960,	 0xA97C,  0xAC00,  0xD7A3,  0xD7B0,  0xD7C6,  0xD7CB,  0xD7FB,
	0xF900,	 0xFAFF,  0xFE10,  0xFE19,  0xFE30,  0xFE6F,  0xFF01,  0xFF60,
	0xFFE0,	 0xFFE6,  0x1B000, 0x1B0FF, 0x1B100, 0x1B12F, 0x1F200, 0x1F2FF,
	0x20000, 0x2FFFD, 0x30000, 0x3FFFD,
};
// clang-format on

#define WIDE_TBL_PAIRS (sizeof(wide_tbl) / sizeof(wide_tbl[0]) / 2)

static int in_range_table(uint32_t cp)
{
	int lo = 0;
	int hi = (int)WIDE_TBL_PAIRS - 1;

	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		uint32_t r_lo = wide_tbl[mid * 2];
		uint32_t r_hi = wide_tbl[mid * 2 + 1];

		if (cp < r_lo)
			hi = mid - 1;
		else if (cp > r_hi)
			lo = mid + 1;
		else
			return 1;
	}
	return 0;
}

/* END: ./src/text/utf8.c */
/* START: ./src/render/renderer.c */
/* END: ./src/render/renderer.c */
/* START: ./src/core/engine.c */
#include "core/engine.h"
#include "log.h"
#include "module.h"
#include "text/rich_text.h"
#include "ui/display.h"
#include "ui/terminal.h"
#include "ui/widgets.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define TARGET_FPS 4
#define TARGET_FRAME_TIME (1000000 / TARGET_FPS)

bool SYNC = true;
struct InputLine *cur_input = NULL;
struct GenericWidget *main_window = NULL;
bool initialized = false;

static const struct {
	const char *name;
	RGB color;
} theme[] = {
	{ "bg", { 16, 16, 28 } },
	{ "fg", { 155, 155, 155 } },
	{ "border", { 155, 155, 155 } },
	{ "border_bg", { 16, 16, 28 } },
	{ "indicator", { 48, 172, 82 } },
};
#define THEME(name) theme_lookup(name, sizeof(theme) / sizeof(theme[0]))

static const RGB *theme_lookup(const char *name, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (strcmp(theme[i].name, name) == 0)
			return &theme[i].color;
	}
	return NULL;
}

void game_refresh_ui()
{
	widget_draw_box(screen, 0, 0, screen->width, screen->height, (RGB *)THEME("border"), (RGB *)THEME("border_bg")); // Create Windows border

	if (screen->width < 90) {
		if (initialized) {
			for (int i = 0; i < G_WIDGET_BUFFER->count; i++) {
				GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
				switch (gw->type) {
				case TYPE_INPUT:
					widget_draw_inputline(screen, &(gw->data.input), (RGB *)THEME("indicator"), NULL);
					break;
				case TYPE_BOX:
					widget_draw_box_ltrb(screen, gw->left, gw->top, gw->right, gw->bottom, NULL, NULL);
					break;
				}
			}
		} else {
			main_window = widget_draw_box(screen, 2, 1, screen->width - 4, screen->height - 10, NULL, NULL);
			widget_draw_box_ltrb(screen, 2, main_window->bottom + 1,
				pos_at_margin(screen->width, 2),
				pos_at_margin(screen->height, 1), NULL, NULL);
			GenericWidget *inputbox = widget_draw_box_ltrb(screen, main_window->left + 1, main_window->bottom - 3,
				main_window->right - 1, main_window->bottom - 1, NULL, NULL);
			GenericWidget *input = widget_create_inputline(inputbox);
			cur_input = &(input->data.input);

			widget_draw_inputline(screen, cur_input, (RGB *)THEME("indicator"), NULL);
		}
	} else {
		widget_draw_box(screen, 0, 0, screen->width, screen->height, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
		widget_draw_box_ltrb(screen, 2, 1, pos_at_margin(screen->width / 2, 2), (screen->height) - 2, NULL, NULL);
	}
}

int kbhit(void)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

void process_input(void)
{
	while (kbhit()) {
		char prev = getchar();
		if (prev == 4) {
			screen_clear();
			game_state.is_running = false;
			break;
		}
		if (cur_input && prev) {
			size_t len = strlen(cur_input->string.text);
			if (cur_input->string.cap - len <= 2) {
				log4engine("log.txt", "REALLOC cur_input->string.text\n");
				cur_input->string.text = inputline_text_realloc(cur_input);
			}
			if (isprint(prev)) {
				*cur_input->string.p++ = prev;
				*cur_input->string.p = '\0';
			} else {
				switch (prev) {
				case 127:
					if (cur_input->string.p > cur_input->string.text) {
						cur_input->string.p--;
						*cur_input->string.p = '\0';
					} else
						*cur_input->string.p = '\0';
					break;
				case 13:
					int top = main_window->top + 1;
					for (int i = 3; i < main_window->right - 1; i++)
						term_set_cell(screen, i, top, 0, NULL, NULL);
					widget_write_text(screen, 3, top, "%s", cur_input->string.text);
					cur_input->string.p = cur_input->string.text;
					*cur_input->string.p = '\0';
				}
			}
			cur_input->dirty = true;
		}
	}
}

void update_game(void)
{
	int width, height = 0;
	term_get_size(&height, &width);
	if (screen->height != height || screen->width != width) {
		Screen *s = screen_create(width, height);
		if (!s)
			return;

		screen_destroy(screen);
		screen = s;
		for (size_t y = 0; y < s->height; y++) {
			for (size_t x = 0; x < s->width; x++) {
				size_t idx = y * s->width + x;
				s->cells[idx].bg = *THEME("bg");
				s->cells[idx].dirty = true;
			}
		}
		game_refresh_ui();
	}
	if (cur_input && cur_input->dirty) {
		widget_draw_inputline(screen, cur_input, (RGB *)THEME("indicator"), NULL);
		screen->cursor.x = cur_input->self->left;
		screen->cursor.y = cur_input->self->top + cur_input->row;
	}
}

long long get_microtime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void init_game()
{
	screen_clear();
	screen_set_fg(screen, get_anti_color(*THEME("bg")));
	screen_set_bg(screen, *THEME("bg"));
	game_refresh_ui();

	initialized = true;

	screen_flush(screen);
}

void game_loop()
{
	init_game();
	/* long long last_frame_time = get_microtime(); */
	/* float delta_time = 0.0f; */
	fflush(stdout);
	do {
		long long frame_start = get_microtime();
		process_input();
		update_game();
		screen_flush(screen);

		if (SYNC) {
			long long frame_end = get_microtime();
			long long actual_frame_duration = frame_end - frame_start;

			if (actual_frame_duration < TARGET_FRAME_TIME) {
				usleep(TARGET_FRAME_TIME - actual_frame_duration);
			}
		}
	} while (game_state.is_running);
}
/* END: ./src/core/engine.c */
/* START: ./src/calc.c */
#include "calc.h"
#include <stddef.h>

size_t next_power_of_2(size_t n)
{
	if (n == 0)
		return 1;
	n--; // 如果 n 本身是 2 的幂，减 1 可以保证结果还是 n
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF // 如果是 64 位系统
	n |= n >> 32;
#endif
	return n + 1;
}
/* END: ./src/calc.c */
/* START: ./merged_all.c */
/* END: ./merged_all.c */


/* --- File: ./combined.c --- */


