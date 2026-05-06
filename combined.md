Project Tree
```
tree -I "miniaudio" -I "combined.md" -I "test"
.
├── architecture_analysis.md
├── build
│   ├── compile_commands.json
│   └── Zi-Game
├── include
│   ├── calc.h
│   ├── core
│   │   └── engine.h
│   ├── log.h
│   ├── module.h
│   ├── render
│   │   └── renderer.h
│   ├── text
│   │   ├── rich_text.h
│   │   └── utf8.h
│   └── ui
│       ├── display.h
│       ├── terminal.h
│       └── widgets.h
├── log.txt
├── main.c
├── makefile
└── src
    ├── calc.c
    ├── core
    │   └── engine.c
    ├── log.c
    ├── module.c
    ├── render
    │   └── renderer.c
    ├── text
    │   ├── rich_text.c
    │   └── utf8.c
    └── ui
        ├── display.c
        ├── terminal.c
        └── widgets.c

12 directories, 26 files

```

/* --- File: ./main.c --- */
#include "core/engine.h"
#include "module.h"
#include "ui/display.h"
#include "ui/widgets.h"
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
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
	bool initialized;
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


/* --- File: ./include/ui/terminal.h --- */
#pragma once
#include "ui/display.h"
#include <termios.h>

void get_terminal_size(int *width, int *height);

int term_restore();

bool term_enter_raw(void);

void term_leave_raw(void);

bool term_get_size(int *cols, int *rows);

void ansi_cursor_hide(void);

void ansi_cursor_show(void);

void ansi_clear_screen(void);

void ansi_cursor_goto(int row, int col);

void ansi_set_fg_rgb(int r, int g, int b);

void ansi_set_bg_rgb(int r, int g, int b);

void ansi_reset(void);

void term_set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg);


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

typedef struct GenericWidget GenericWidget;
int screen_add_root(Screen *s, GenericWidget *gw);


/* --- File: ./include/ui/widgets.h --- */
#pragma once
#include "ui/display.h"
#include <stdint.h>

struct InputLine;
struct BoxLTRB;

typedef struct BoxLTRB {
} BoxLTRB;

typedef struct InputLine {
	struct GenericWidget *self;
	int row;
	int curser_col;
	bool dirty : 1;
	RGB symbol;
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
	struct GenericWidget *parent;
	struct {
		GenericWidget **data;
		int len;
	} childs;
	RGB fg, bg;
	int left, right, top, bottom;
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

GenericWidget *widget_create_box_ltrb(Screen *screen, int left, int top, int right, int bottom, RGB *fg, RGB *bg);

GenericWidget *widget_create_box(Screen *screen, int x, int y, int w, int h, RGB *fg, RGB *bg);
void widget_draw_widget(Screen *s, GenericWidget *gw);

void widget_write_text(Screen *screen, int x, int y, const char *format, ...);

void widget_draw_vline(Screen *screen, int x, RGB *fg, RGB *bg);

void widget_draw_hline(Screen *screen, int x, RGB *fg, RGB *bg);

int pos_to_margin(int total_size, int margin);

GenericWidget *widget_create_inputline(GenericWidget *parent, int row);
GenericWidget *widget_create_inputline_ltrb(Screen *s, int x, int y, int w, int h,
	RGB *fg, RGB *bg, bool has_border);
void widget_draw_inputline(Screen *screen, InputLine *input, RGB *fg, RGB *bg);
char *inputline_text_realloc(InputLine *iw);
void widget_buffer_reset(void);

void widget_buffer_pop(GenericWidget *gw);

void widget_add_child(GenericWidget *parent, GenericWidget *child);


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


/* --- File: ./src/ui/terminal.c --- */
#include "ui/terminal.h"
#include "module.h"
#include "text/utf8.h"
#include "ui/display.h"
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ESC "\x1b"
#define CSI ESC "["

#define CLEAR_SCREEN CSI "2J" CSI "3J"

#define CURSOR_MOVE(R, C) CSI #R ";" #C "H"

#define CURSOR_HIDE CSI "?25l"
#define CURSOR_SHOW CSI "?25h"

#define ATTR_RESET CSI "0m"

static volatile sig_atomic_t g_resized = 0;

/* -------------------------------------------------------------------------
 * SIGWINCH handler
 *
 * The kernel sends SIGWINCH to the foreground process group when the
 * terminal window is resized.  We simply set a flag; the actual redraw
 * happens in the main loop so we avoid async-signal-safety issues.
 * ------------------------------------------------------------------------- */
static void handle_sigwinch(int sig)
{
	(void)sig;
	g_resized = 1;
}

static bool raw_mode_active = false;
struct termios oldt, newt;

struct Screen {
	int width, height;
	Cell *cells;
	struct {
		int x;
		int y;
	} cursor;
};

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
	signal(SIGWINCH, handle_sigwinch);
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

bool term_get_size(int *cols, int *rows)
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
	s->cells[idx].fg = (fg != NULL) ? *fg : G_ENV.fg;
	;
	s->cells[idx].bg = (bg != NULL) ? *bg : G_ENV.bg;
	;
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


/* --- File: ./src/ui/display.c --- */
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


/* --- File: ./src/ui/widgets.c --- */
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

bool SYNC = false;
struct GenericWidget *cur_input = NULL;
struct GenericWidget *root_window = NULL;

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

void init_game()
{
	screen_clear();
	screen_set_fg(screen, get_anti_color(*THEME("bg")));
	screen_set_bg(screen, *THEME("bg"));

	int width, height = 0;
	screen_get_size(screen, &width, &height);
	root_window = widget_create_box(screen, 0, 0, width, height, (RGB *)THEME("border"), (RGB *)THEME("border_bg")); // Create Windows border
	screen_add_root(screen, root_window);

	game_refresh_ui();

	game_state.initialized = true;

	screen_flush(screen);
}

void game_refresh_ui()
{
	/* screen_clear(); */
	if (!game_state.initialized) {
		GenericWidget *input = widget_create_inputline_ltrb(screen, root_window->left + 1, root_window->bottom - 3,
			root_window->right - 1, root_window->bottom - 1, (RGB *)THEME("indicator"), NULL, true);
		cur_input = input;
		/* widget_draw_box(screen, input); */
		widget_add_child(root_window, input);
	}

	widget_draw_widget(screen, root_window);
	for (int i = 0; i < root_window->childs.len; i++) {
		/* switch (root_window->childs.data[i]->type) { */
		/* case TYPE_INPUT: */
		/* 	widget_draw_inputline(screen, &(root_window->childs.data[i]->data.input), NULL, NULL); */
		/* 	break; */
		/* case TYPE_BOX: */
		widget_draw_widget(screen, root_window->childs.data[i]);
		/* break; */
		/* } */
	}
}

void update_game(void)
{
	int width, height = 0;
	int o_width, o_height = 0;
	screen_get_size(screen, &o_width, &o_height);

	bool got_size = false;
	while (!got_size)
		got_size = term_get_size(&width, &height);

	if ((o_width != width || o_height != height)) {
		usleep(1000);
		Screen *s = screen_create(width, height);
		if (!s)
			return;

		screen_destroy(screen);
		screen = s;

		root_window->right = width - 1;
		root_window->bottom = height - 1;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				set_cell(s, x, y, 0, NULL, (RGB *)THEME("bg"));
			}
		}

		game_refresh_ui();
	}

	if (cur_input && cur_input->data.input.dirty) {
		widget_draw_widget(screen, cur_input);

		int new_x = cur_input->left;
		int new_y = cur_input->top + cur_input->data.input.row;

		screen_set_cursor(screen, &new_x, &new_y);

		cur_input->data.input.dirty = false;
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
		char prev;

		ssize_t n = read(STDIN_FILENO, &prev, 1);
		if (prev == 4) {
			screen_clear();
			game_state.is_running = false;
			break;
		}
		if (cur_input && prev) {
			InputLine *input_w = &(cur_input->data.input);
			size_t len = strlen(input_w->string.text);
			if (input_w->string.cap - len <= 2) {
				log4engine("log.txt", "REALLOC input_w->string.text\n");
				input_w->string.text = inputline_text_realloc(input_w);
			}
			if (isprint(prev)) {
				*input_w->string.p++ = prev;
				*input_w->string.p = '\0';
			} else {
				switch (prev) {
				case 127:
					if (input_w->string.p > input_w->string.text) {
						input_w->string.p--;
						*input_w->string.p = '\0';
					} else
						*input_w->string.p = '\0';
					break;
				case 13:
					int top = root_window->top + 1;
					for (int i = 3; i < root_window->right - 1; i++)
						term_set_cell(screen, i, top, 0, NULL, NULL);
					widget_write_text(screen, 3, top, "%s", input_w->string.text);
					input_w->string.p = input_w->string.text;
					*input_w->string.p = '\0';
					break;
				case 0x0c:
					/* game_refresh_ui(); */
					/* screen_flush(screen); */
					break;
				}
			}
			input_w->dirty = true;
		}
	}
}

long long get_microtime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void game_loop()
{
	init_game();

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


/* --- File: ./combined.c --- */


