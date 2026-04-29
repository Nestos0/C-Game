#include "module.h"
#include "text/rich_text.h"
#include "ui/screen.h"
#include <locale.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

struct termios oldt, newt;

void terminal_init()
{
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;

	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}
init_register(terminal_init);

void utf8_init()
{
	setlocale(LC_ALL, "");
}
init_register(utf8_init);

void terminal_restore()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void game_loop()
{
	while (1)
		;
}

int main()
{
	screen_clear();
	for (initcall_t *p = __start_initcalls;
		p < __stop_initcalls; p++)
		if (*p)
			(*p)();

	game_loop();

	terminal_restore();
	return 0;
}
