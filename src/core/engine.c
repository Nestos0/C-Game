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
	if (!game_state.initialized) {
		GenericWidget *input = widget_create_inputline_ltrb(screen, root_window->left + 1, root_window->bottom - 3,
			root_window->right - 1, root_window->bottom - 1, NULL, NULL, true);
		cur_input = &(input->data.input);
		/* widget_draw_box(screen, input); */
		widget_add_child(root_window, input);
	}

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
	widget_draw_widget(screen, root_window);
}

void update_game(void)
{
	int width, height = 0;

	bool got_size = false;
	while (!got_size)
		got_size = term_get_size(&width, &height);

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
					int top = root_window->top + 1;
					for (int i = 3; i < root_window->right - 1; i++)
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
