/* --- File: ./src/game.c --- */
#include "game.h"
#include "core/engine.h"
#include "module.h"
#include "ui/widgets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOG_LINES 100

static char *log_lines[MAX_LOG_LINES];
static int log_count = 0;

static void game_init(void)
{
	for (int i = 0; i < MAX_LOG_LINES; i++) {
		log_lines[i] = NULL;
	}
}

static void add_log(const char *text)
{
	if (log_count < MAX_LOG_LINES) {
		log_lines[log_count++] = strdup(text);
	} else {
		free(log_lines[0]);
		for (int i = 1; i < MAX_LOG_LINES; i++) {
			log_lines[i - 1] = log_lines[i];
		}
		log_lines[MAX_LOG_LINES - 1] = strdup(text);
	}
}

static void game_handle_command(const char *cmd)
{
	char buf[512];
	snprintf(buf, sizeof(buf), "> %s", cmd);
	add_log(buf);

	if (strcmp(cmd, "quit") == 0) {
		game_state.is_running = false;
	} else if (strcmp(cmd, "help") == 0) {
		add_log("Available commands: help, quit");
	} else {
		snprintf(buf, sizeof(buf), "Unknown command: %s", cmd);
		add_log(buf);
	}
}

static void game_render_logs(Screen *s, int x, int y, int width, int height)
{
	int display_count = (log_count < height) ? log_count : height;
	int log_offset = (log_count > height) ? (log_count - height) : 0;

	for (int i = 0; i < display_count; i++) {
		for (int dx = 0; dx < width; dx++) {
			set_cell(s, x + dx, y + i, ' ', NULL, NULL);
		}
		widget_write_text(s, x, y + i, "%s", log_lines[log_offset + i]);
	}
}

static const GameInterface game_api = {
	.init = game_init,
	.handle_command = game_handle_command,
	.render_logs = game_render_logs
};

static int __game_module_register(void)
{
	engine_register_game_interface(&game_api);
	return 0;
}
APP_INIT(__game_module_register);


