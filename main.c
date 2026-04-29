#include "text/rich_text.h"
#include <locale.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

typedef void (*initcall_t)(void);

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

#define init_register(fn)              \
	static initcall_t __init##fn \
		__attribute__((section("initcalls"), used)) = fn;

struct termios oldt, newt;

void terminal_init()
{
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;

	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}
init_register(terminal_init);

void terminal_restore()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int main()
{
	for (initcall_t *p = __start_initcalls;
		p < __stop_initcalls; ++p)
		(*p)();

	terminal_restore();
	return 0;
}
