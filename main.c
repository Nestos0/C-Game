#include "core/engine.h"
#include "module.h"
#include "ui/screen.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

struct GameState game_state;

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

	terminal_restore();
	return 0;
}
