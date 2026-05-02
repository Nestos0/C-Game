#include "core/engine.h"
#include "module.h"
#include "text/rich_text.h"
#include "ui/display.h"
#include "ui/terminal.h"
#include "ui/widgets.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define TARGET_FPS 60
#define TARGET_FRAME_TIME (1000000 / TARGET_FPS)

typedef struct {
	char name[16];
	enum { CFG_RGB,
		CFG_TEXT } type;
	struct {
		int r, g, b;
	} color;
} Config;

RGB def_bg = (struct RGB) { 16, 16, 28 };
RGB outer_border = (struct RGB) { 155, 155, 155 };
RGB outer_border_bg = (struct RGB) { 16, 16, 28 };

Config config[] = { { .name = "bg", .type = CFG_RGB, .color = { 16, 16, 28 } },
	{ .name = "fg", .type = CFG_RGB, .color = { 155, 155, 155 } } };

Config get_name(char *name, Config *cfg)
{
	for (int i = 0; i < 3; i++) {
		if (cfg[i].name == name) {
			return cfg[i];
		}
	}
	return (Config) { 0 };
}

void game_refresh_ui(void *config)
{
	if (screen->width < 90) {
		/* widget_write_text(screen, 0, 0, "Terminal width:%d, height:%d", screen->width, screen->height); */
		widget_draw_box(screen, 0, 0, screen->width, screen->height, &outer_border, &outer_border_bg);
		BoxLTRB *main = widget_draw_box(screen, 2, 1, screen->width - 4, screen->height - 10, NULL, NULL);
		widget_draw_box_ltrb(screen, 2, main->bottom + 1,
			pos_at_margin(screen->width, 2),
			pos_at_margin(screen->height, 1), NULL, NULL);
		widget_draw_box_ltrb(screen, main->left + 1, main->bottom - 5,
			main->right - 1, main->bottom - 1, NULL, NULL);
	} else {
		/* widget_write_text(screen, 0, 0, "Terminal width:%d, height:%d", screen->width, screen->height); */
		widget_draw_box(screen, 0, 0, screen->width, screen->height, &outer_border, &outer_border_bg);
		widget_draw_box_ltrb(screen, 2, 1, pos_at_margin(screen->width / 2, 2), (screen->height) - 2, NULL, NULL);
	}
}

void init_game()
{
	Config config[] = { { .name = "bg", .type = CFG_RGB, .color = { 16, 16, 28 } },
		{ .name = "bg", .type = CFG_RGB, .color = { 155, 155, 155 } } };
	screen_clear();
	screen_set_fg(screen, get_anti_color(def_bg));
	screen_set_bg(screen, def_bg);
	game_refresh_ui(&config);
	screen_flush(screen);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

void process_input(void)
{
	/* char ch = getchar(); */
	/* if (ch == 4) { */
	/* 	screen_clear(); */
	/* 	game_state.is_running = false; */
	/* } */
}

void update_game(void)
{
	int width, height = 0;
	term_get_size(&height, &width);
	if (screen->height != height || screen->width != width) {
		Screen *s = (Screen *)calloc(1, sizeof(Screen));
		if (!s)
			return;

		s->cells = (Cell *)calloc((size_t)(width * height), sizeof(Cell));
		if (!s->cells) {
			free(s);
			return;
		}

		s->width = width;
		s->height = height;
		screen = s;
		for (size_t y = 0; y < s->height; y++) {
			for (size_t x = 0; x < s->width; x++) {
				size_t idx = y * s->width + x;
				s->cells[idx].bg = def_bg;
			}
		}
		game_refresh_ui(config);
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
	/* long long last_frame_time = get_microtime(); */
	/* float delta_time = 0.0f; */
	fflush(stdout);
	do {
		long long frame_start = get_microtime();
		process_input();
		update_game();
		screen_flush(screen);
		long long frame_end = get_microtime();
		long long actual_frame_duration = frame_end - frame_start;

		if (actual_frame_duration < TARGET_FRAME_TIME) {
			usleep(TARGET_FRAME_TIME - actual_frame_duration);
		}
	} while (game_state.is_running);
}
