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

struct InputLine *cur_input = NULL;
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
					InputLine *input = &(gw->data.input);
					widget_draw_inputline(screen, input, (RGB *)THEME("indicator"), NULL);
					break;
				case TYPE_BOX:
					BoxLTRB *box = &(gw->data.box);
					widget_draw_box_ltrb(screen, box->left, box->top, box->right, box->bottom, NULL, NULL);
					break;
				}
			}
		} else {
			BoxLTRB *main = widget_draw_box(screen, 2, 1, screen->width - 4, screen->height - 10, NULL, NULL);
			widget_draw_box_ltrb(screen, 2, main->bottom + 1,
				pos_at_margin(screen->width, 2),
				pos_at_margin(screen->height, 1), NULL, NULL);

			BoxLTRB *inputbox = widget_draw_box_ltrb(screen, main->left + 1, main->bottom - 3,
				main->right - 1, main->bottom - 1, NULL, NULL);
			InputLine *input = widget_create_inputline(inputbox);
			widget_draw_inputline(screen, input, (RGB *)THEME("indicator"), NULL);
			cur_input = input;
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
	if (kbhit()) {
		char prev = getchar();
		if (prev == 4) {
			screen_clear();
			game_state.is_running = false;
		} else {
			if (cur_input && prev) {
				size_t len = strlen(cur_input->string.text);
				if (cur_input->string.cap - len <= 2) {
					log4engine("log.txt", "REALLOC cur_input->string.text\n");
					cur_input->string.text = inputline_text_realloc(cur_input);
				}
				char *ptr = cur_input->string.p;
				*ptr++ = prev;
				*ptr = '\0';
				cur_input->string.p = ptr;
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
		term_set_cell(screen, 10, 10, 'A', (RGB *)THEME("indicator"), NULL);
		widget_draw_inputline(screen, cur_input, (RGB *)THEME("indicator"), NULL);
		screen->cursor.x = cur_input->parent->left;
		screen->cursor.y = cur_input->parent->top + cur_input->row;
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

		long long frame_end = get_microtime();
		long long actual_frame_duration = frame_end - frame_start;

		if (actual_frame_duration < TARGET_FRAME_TIME) {
			usleep(TARGET_FRAME_TIME - actual_frame_duration);
		}
	} while (game_state.is_running);
}
