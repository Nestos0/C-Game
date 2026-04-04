#include "display.h"

void clear()
{
	printf("\033[H\033[J");
}

void set_unbuffered_mode()
{
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void display_field(int rows, int cols, int field[rows][cols])
{
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			printf("%s%c%s", GREEN, field[r][c], RESET);
		}
		putchar('\n');
	}
}

void move_cursor(int x, int y)
{
	printf("\033[%d;%dH", y + 1, x + 1);
}
