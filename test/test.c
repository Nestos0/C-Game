#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/* --- 0. 基础类型定义 --- */

typedef struct {
	uint8_t r, g, b;
} RGB;

typedef struct {
	uint32_t cp; // Unicode Code Point
	RGB fg, bg; // 前景色和背景色
	bool dirty; // 是否需要重绘
	bool wide; // 宽字符标记 (如中文)
	bool wide_cont; // 宽字符的后半部分
} Cell;

typedef struct {
	int width, height;
	Cell *cells;
} Screen;

/* --- 1. 模块系统设计模式 --- */

typedef int (*initcall_t)(void);

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

#define APP_INIT(fn)                  \
	static initcall_t __init_##fn \
		__attribute__((used, section("initcalls"), aligned(sizeof(void *)))) = fn;

/* --- 2. 全局状态 --- */

static struct {
	Screen *screen;
	bool is_running;
	RGB default_fg;
	RGB default_bg;
	volatile sig_atomic_t win_resized;
} G = {
	.default_fg = { 200, 200, 200 },
	.default_bg = { 20, 20, 30 },
	.is_running = true,
	.win_resized = 0
};

/* --- 3. 辅助工具：UTF-8 编码 --- */

static int utf8_encode(uint32_t cp, char *out)
{
	if (cp < 0x80) {
		out[0] = (char)cp;
		return 1;
	} else if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	} else if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	} else {
		out[0] = (char)(0xF0 | (cp >> 18));
		out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[3] = (char)(0x80 | (cp & 0x3F));
		return 4;
	}
}

/* --- 4. 终端控制模块 --- */

static struct termios orig_termios;

static void handle_sigwinch(int sig)
{
	(void)sig;
	G.win_resized = 1;
}

static int terminal_restore(void)
{
	write(STDOUT_FILENO, "\x1b[?1049l", 8); // 恢复备用屏幕
	write(STDOUT_FILENO, "\x1b[?25h", 6); // 显示光标
	if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) < 0)
		return -1;
	return 0;
}

static int terminal_init(void)
{
	if (!isatty(STDIN_FILENO))
		return -1;
	if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
		return -1;

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
		return -1;

	write(STDOUT_FILENO, "\x1b[?1049h", 8); // 启用备用屏幕
	write(STDOUT_FILENO, "\x1b[?25l", 6); // 隐藏光标

	signal(SIGWINCH, handle_sigwinch);
	return 0;
}
APP_INIT(terminal_init);

/* --- 5. 屏幕缓冲模块 --- */

static void screen_resize(int w, int h)
{
	if (G.screen) {
		free(G.screen->cells);
		free(G.screen);
	}
	G.screen = (Screen *)calloc(1, sizeof(Screen));
	G.screen->width = w;
	G.screen->height = h;
	G.screen->cells = (Cell *)calloc(w * h, sizeof(Cell));

	// 关键：显式初始化 dirty 标记
	// screen_flush 只输出 dirty 的单元格。calloc 虽然清零了内存(cp=0)，
	// 但 dirty 也是 false，如果不手动设为 true，flush 函数什么都不会输出。
	for (int i = 0; i < w * h; i++) {
		G.screen->cells[i].cp = ' '; // 初始化字符为空格
		G.screen->cells[i].fg = G.default_fg;
		G.screen->cells[i].bg = G.default_bg;
		G.screen->cells[i].dirty = true; // 强制全屏刷新
		G.screen->cells[i].wide = false;
		G.screen->cells[i].wide_cont = false;
	}

	// 注意：这里不再发送 \x1b[H。
	// 因为提供的 screen_flush 逻辑依赖于光标在左上角。
	// 如果屏幕大小改变，通常终端备用缓冲区的光标还在(1,1)。
	// 如果出现错位，通常意味着需要全量刷新（我们已经通过 dirty=true 做到了）。
	// 清屏指令是可选的，因为我们马上要用空格重绘每一处。
	// write(STDOUT_FILENO, "\x1b[2J", 4);
}

static int screen_init(void)
{
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	screen_resize(ws.ws_col, ws.ws_row);
	return 0;
}
APP_INIT(screen_init);

/* --- 6. 渲染引擎 (你提供的版本) --- */

static void buf_write(int fd, const char *data, int len, char *buf, int *buf_pos, int buf_size)
{
	if (len <= 0)
		return;

	if (*buf_pos + len > buf_size) {
		write(fd, buf, *buf_pos);
		*buf_pos = 0;

		if (len > buf_size) {
			write(fd, data, len);
			return;
		}
	}

	memcpy(buf + *buf_pos, data, len);
	*buf_pos += len;
}

static int fmt_ansi_color(char *out, int fg_r, int fg_g, int fg_b, int bg_r, int bg_g, int bg_b)
{
	return sprintf(out, "\x1b[38;2;%d;%d;%d;48;2;%d;%d;%dm", fg_r, fg_g, fg_b, bg_r, bg_g, bg_b);
}

// 这是替换后的 screen_flush
static void screen_flush(Screen *s)
{
	int last_fg[3] = { -1, -1, -1 };
	int last_bg[3] = { -1, -1, -1 };
	int cur_x = 0, cur_y = 0;

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

/* --- 7. 游戏逻辑与交互 --- */

// U+2500 Box Drawing Characters
#define H_LINE 0x2500
#define V_LINE 0x2502
#define TL_CORNER 0x250C
#define TR_CORNER 0x2510
#define BL_CORNER 0x2514
#define BR_CORNER 0x2518

static void set_cell(int x, int y, uint32_t cp, RGB *fg, RGB *bg)
{
	// 边界检查
	if (x < 0 || x >= G.screen->width || y < 0 || y >= G.screen->height)
		return;

	int idx = y * G.screen->width + x;
	Cell *c = &G.screen->cells[idx];

	// 状态改变才标记 dirty
	if (c->cp != cp || c->fg.r != fg->r || c->fg.g != fg->g || c->fg.b != fg->b || c->bg.r != bg->r || c->bg.g != bg->g || c->bg.b != bg->b) {

		c->cp = cp;
		c->fg = *fg;
		c->bg = *bg;
		c->dirty = true;
		c->wide = false;
		c->wide_cont = false;
	}
}

static void game_update(void)
{
	if (G.win_resized) {
		struct winsize ws;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
		screen_resize(ws.ws_col, ws.ws_row);
		G.win_resized = 0;
	}

	int w = G.screen->width;
	int h = G.screen->height;

	RGB border_color = { 100, 200, 255 };
	RGB bg_color = G.default_bg;

	// 仅在第一次或 Resize 时绘制边框
	// 为了演示边框检测，这里我们每次都检查并重绘边框(如果被破坏)
	// 但为了性能优化，我们假设边框不需要每帧刷新，除非改变尺寸。
	// 不过由于 set_cell 内部有 dirty 检查，重复调用开销不大。

	// 四角
	set_cell(0, 0, TL_CORNER, &border_color, &bg_color);
	set_cell(w - 1, 0, TR_CORNER, &border_color, &bg_color);
	set_cell(0, h - 1, BL_CORNER, &border_color, &bg_color);
	set_cell(w - 1, h - 1, BR_CORNER, &border_color, &bg_color);

	// 顶底边
	for (int x = 1; x < w - 1; x++) {
		set_cell(x, 0, H_LINE, &border_color, &bg_color);
		set_cell(x, h - 1, H_LINE, &border_color, &bg_color);
	}

	// 左右边
	for (int y = 1; y < h - 1; y++) {
		set_cell(0, y, V_LINE, &border_color, &bg_color);
		set_cell(w - 1, y, V_LINE, &border_color, &bg_color);
	}

	// 3. 动态文字
	static int counter = 0;
	char text[64];
	int len = snprintf(text, sizeof(text), " Running: %d ", counter++);

	int text_start_x = (w - len) / 2;
	int text_start_y = h / 2;

	RGB text_color = G.default_fg;

	for (int i = 0; i < len; i++) {
		set_cell(text_start_x + i, text_start_y, (uint32_t)text[i], &text_color, &bg_color);
	}
}

/* --- 8. 主程序 --- */

int main(void)
{
	for (initcall_t *p = __start_initcalls; p < __stop_initcalls; ++p) {
		if ((*p)() != 0) {
			fprintf(stderr, "Initialization failed!\n");
			return 1;
		}
	}

	char ch;
	while (G.is_running) {
		fd_set fds;
		struct timeval tv = { 0, 10000 };

		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
			if (read(STDIN_FILENO, &ch, 1) > 0) {
				if (ch == 'q')
					G.is_running = false;
			}
		}

		game_update();
		screen_flush(G.screen);
		getchar();
		usleep(30000);
	}

	terminal_restore();
	return 0;
}
