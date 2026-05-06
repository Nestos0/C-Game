/* --- File: ./src/core/engine.c --- */
/* File src/core/engine.c */
#include "core/engine.h"
#include "game.h"
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
struct GenericWidget *gamelog_window = NULL;
struct GenericWidget *state_window = NULL;

static const GameInterface *g_game_api = NULL;

void engine_register_game_interface(const GameInterface *iface)
{
	g_game_api = iface;
}

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

int kbhit(void)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

static void layout_windows(int width, int height)
{
	root_window->right = width - 1;
	root_window->bottom = height - 1;

	if (width > 100) {
		gamelog_window->left = root_window->left + 1;
		gamelog_window->top = root_window->top + 1;
		gamelog_window->right = (int)(width * 0.7);
		gamelog_window->bottom = root_window->bottom - 1;

		state_window->left = gamelog_window->right + 1;
		state_window->top = root_window->top + 1;
		state_window->right = root_window->right - 1;
		state_window->bottom = root_window->bottom - 1;
	} else {
		gamelog_window->left = root_window->left + 1;
		gamelog_window->top = root_window->top + 1;
		gamelog_window->right = root_window->right - 1;
		gamelog_window->bottom = (int)(height * 0.8);

		state_window->left = root_window->left + 1;
		state_window->top = gamelog_window->bottom + 1;
		state_window->right = root_window->right - 1;
		state_window->bottom = root_window->bottom - 1;
	}

	if (cur_input) {
		cur_input->left = gamelog_window->left + 1;
		cur_input->top = gamelog_window->bottom - 3;
		cur_input->right = gamelog_window->right - 1;
		cur_input->bottom = gamelog_window->bottom - 1;
	}
}

void init_game()
{
	if (g_game_api && g_game_api->init) {
		g_game_api->init();
	}

	screen_clear();
	screen_set_fg(screen, get_anti_color(*THEME("bg")));
	screen_set_bg(screen, *THEME("bg"));

	int width, height = 0;
	screen_get_size(screen, &width, &height);
	root_window = widget_create_box(screen, 0, 0, width, height, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
	screen_add_root(screen, root_window);

	gamelog_window = widget_create_box(screen, 0, 0, 1, 1, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));
	state_window = widget_create_box(screen, 0, 0, 1, 1, (RGB *)THEME("border"), (RGB *)THEME("border_bg"));

	widget_add_child(root_window, gamelog_window);
	widget_add_child(root_window, state_window);

	layout_windows(width, height);
	game_refresh_ui();
	game_state.initialized = true;
	screen_flush(screen);
}

void game_refresh_ui()
{
	if (!game_state.initialized) {
		GenericWidget *input = widget_create_inputline_ltrb(screen, gamelog_window->left + 1, gamelog_window->bottom - 3,
			gamelog_window->right - 1, gamelog_window->bottom - 1, (RGB *)THEME("indicator"), NULL, true);
		cur_input = input;
		widget_add_child(gamelog_window, input);
	}

	widget_draw_widget(screen, root_window);
	widget_draw_widget(screen, gamelog_window);
	widget_draw_widget(screen, state_window);

	if (g_game_api && g_game_api->render_logs) {
		int log_w = gamelog_window->right - gamelog_window->left - 1;
		int log_h = gamelog_window->bottom - gamelog_window->top - 4;
		g_game_api->render_logs(screen, gamelog_window->left + 1, gamelog_window->top + 1, log_w, log_h);
	}

	if (cur_input)
		widget_draw_widget(screen, cur_input);
}

void update_game(void)
{
	int width, height = 0;
	int o_width, o_height = 0;
	screen_get_size(screen, &o_width, &o_height);

	bool got_size = false;
	while (!got_size)
		got_size = term_get_size(&width, &height);

	if (o_width != width || o_height != height) {
		usleep(1000);
		Screen *s = screen_create(width, height);
		if (!s)
			return;

		screen_destroy(screen);
		screen = s;

		RGB bg = *THEME("bg");
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				set_cell(s, x, y, ' ', NULL, &bg);
			}
		}

		layout_windows(width, height);

		game_refresh_ui();

		screen_flush(screen);
	}

	if (cur_input && cur_input->data.input.dirty) {
		widget_draw_widget(screen, cur_input);
		int new_x, new_y;
		screen_set_cursor(screen, &new_x, &new_y);
		cur_input->data.input.dirty = false;
	}
}

void process_input(void)
{
	while (kbhit()) {
		char prev;
		ssize_t n = read(STDIN_FILENO, &prev, 1);
		if (n <= 0)
			break;
		if (prev == 4) {
			game_state.is_running = false;
			break;
		}
		if (cur_input && prev) {
			InputLine *input_w = &(cur_input->data.input);
			if (isprint(prev)) {
				*input_w->string.p++ = prev;
				*input_w->string.p = '\0';
			} else if (prev == 127 && input_w->string.p > input_w->string.text) {
				*(--input_w->string.p) = '\0';
			} else if (prev == 13) {
				if (g_game_api && g_game_api->handle_command) {
					g_game_api->handle_command(input_w->string.text);
				}
				input_w->string.p = input_w->string.text;
				*input_w->string.p = '\0';
				game_refresh_ui();
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
	do {
		long long frame_start = get_microtime();
		process_input();
		update_game();
		screen_flush(screen);
		if (SYNC) {
			long long duration = get_microtime() - frame_start;
			if (duration < TARGET_FRAME_TIME)
				usleep(TARGET_FRAME_TIME - duration);
		}
	} while (game_state.is_running);
}
