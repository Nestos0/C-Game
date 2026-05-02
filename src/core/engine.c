#include "core/engine.h"
#include "log.h"
#include "module.h"
#include "text/rich_text.h"
#include "ui/display.h"
#include "ui/terminal.h"
#include "ui/widgets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define TARGET_FPS 20
#define TARGET_FRAME_TIME (1000000 / TARGET_FPS)

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

struct InputLine *cur_input;

void game_refresh_ui()
{
	if (screen->width < 90) {
		/* widget_write_text(screen, 0, 0, "Terminal width:%d, height:%d", screen->width, screen->height); */
		widget_draw_box(screen, 0, 0, screen->width, screen->height, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
		BoxLTRB *main = widget_draw_box(screen, 2, 1, screen->width - 4, screen->height - 10, NULL, NULL);
		widget_draw_box_ltrb(screen, 2, main->bottom + 1,
			pos_at_margin(screen->width, 2),
			pos_at_margin(screen->height, 1), NULL, NULL);
		BoxLTRB *inputbox = widget_draw_box_ltrb(screen, main->left + 1, main->bottom - 3,
			main->right - 1, main->bottom - 1, NULL, NULL);
		InputLine *input = widget_create_inputline(inputbox);
		widget_draw_inputline(input, (RGB *)THEME("indicator"), NULL);
		cur_input = input;
		/* term_set_cell(screen, input->left + 1, input->top + 1, '$', (RGB *)THEME("indicator"), NULL); */
	} else {
		widget_draw_box(screen, 0, 0, screen->width, screen->height, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
		widget_draw_box_ltrb(screen, 2, 1, pos_at_margin(screen->width / 2, 2), (screen->height) - 2, NULL, NULL);
	}
}

void init_game()
{
	screen_clear();
	screen_set_fg(screen, get_anti_color(*THEME("bg")));
	screen_set_bg(screen, *THEME("bg"));
	game_refresh_ui();
	screen_flush(screen);
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
	if (!kbhit()) {
		char prev = getchar();
		if (prev == 4) {
			screen_clear();
			game_state.is_running = false;
		} else {
			if (cur_input && prev > 0) {
				cur_input->start[0] = prev;
				cur_input->start++;
				cur_input->dirty = true;
			}
		}
	}
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
				s->cells[idx].bg = *THEME("bg");
				s->cells[idx].dirty = true;
			}
		}
		game_refresh_ui();
	}
	if (cur_input->dirty) {
		widget_draw_inputline(cur_input, (RGB *)THEME("indicator"), NULL);
		log4engine("log.txt", "%s", cur_input->text);
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
