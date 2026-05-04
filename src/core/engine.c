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
	int width, height = 0;
	screen_get_size(screen, &width, &height);
	widget_create_box(screen, 0, 0, width, height, (RGB *)THEME("border"), (RGB *)THEME("border_bg")); // Create Windows border

	if (width < 90) {
		if (initialized) {
			size_t count = G_WIDGET_BUFFER->count;
			for (int i = 0; i < count; i++) {
				GenericWidget *gw = &G_WIDGET_BUFFER->widgets[i];
				switch (gw->type) {
				case TYPE_INPUT:
					widget_draw_inputline(screen, &(gw->data.input), (RGB *)THEME("indicator"), NULL);
					break;
				case TYPE_BOX:
					widget_create_box_ltrb(screen, gw->left, gw->top, gw->right, gw->bottom, NULL, NULL);
					break;
				}
			}
		} else {
			main_window = widget_create_box(screen, 2, 1, width - 4, height - 10, NULL, NULL);
			widget_draw_box(screen, main_window);
			GenericWidget *info = widget_create_box_ltrb(screen, 2, main_window->bottom + 1,
				pos_to_margin(width, 2),
				pos_to_margin(height, 1), NULL, NULL);
			widget_draw_box(screen, info);
			GenericWidget *inputbox = widget_create_box_ltrb(screen, main_window->left + 1, main_window->bottom - 3,
				main_window->right - 1, main_window->bottom - 1, NULL, NULL);
			widget_draw_box(screen, inputbox);
			GenericWidget *input = widget_create_inputline(inputbox, 0);
			widget_draw_inputline(screen, &(input->data.input), (RGB *)THEME("indicator"), NULL);
			cur_input = &(input->data.input);
		}
	} else {
		widget_create_box(screen, 0, 0, width, height, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
		widget_create_box_ltrb(screen, 2, 1, pos_to_margin(width / 2, 2), (height)-2, NULL, NULL);
	}
}

void update_game(void)
{
	int width, height = 0;
	term_get_size(&width, &height);

	if (screen_get_width(screen) != width || screen_get_height(screen) != height) {
		Screen *s = screen_create(width, height);
		if (!s)
			return;

		screen_destroy(screen);
		screen = s;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				set_cell(s, x, y, 0, NULL, (RGB *)THEME("bg"));
			}
		}
		game_refresh_ui();
	}

	if (cur_input && cur_input->dirty) {
		widget_draw_inputline(screen, cur_input, (RGB *)THEME("indicator"), NULL);

		int new_x = cur_input->self->left;
		int new_y = cur_input->self->top + cur_input->row;

		screen_set_cursor(screen, &new_x, &new_y);

		cur_input->dirty = false;
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
