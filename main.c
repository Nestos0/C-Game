#include "core/engine.h"
#include "module.h"
#include "ui/display.h"
#include "ui/widgets.h"
#include <string.h>
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
