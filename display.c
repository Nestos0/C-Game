#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "display.h"

#define SnackHead 'O'
#define SnackTail '0'
#define RED "\033[31m"
#define GREEN "\033[32m"
#define RESET "\033[0m"

/* #define Init_Vector2d(matrix, rows, cols)                \ */
/* 	for (int r = 0; r < rows; r++) {                 \ */
/* 		for (int c = 0; c < cols; c++) {         \ */
/* 			matrix[r][c] = rows * r + c + 1; \ */
/* 		}                                        \ */
/* 	} */

#define Init_Vector2d(matrix, rows, cols)        \
	for (int r = 0; r < rows; r++) {         \
		for (int c = 0; c < cols; c++) { \
			matrix[r][c] = ' ';      \
		}                                \
	}

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
}

void display_field(int rows, int cols, int field[rows][cols])
{
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			printf("%c", field[r][c]);
		}
		printf("\n");
	}
}

void move_cursor(int x, int y)
{
	printf("\033[%d;%dH", y + 1, x + 1);
}

int main()
{
	// Init Game
	_Bool game_running = true;
	clear();
	set_unbuffered_mode();

	char c;

	int gamefield[20][20];
	Init_Vector2d(gamefield, 20, 20);

	display_field(20, 20, gamefield);

	/* while (game_running) { */
	/* 	c = getchar(); */
	/* 	if (c == EOF || c == 4) { */
	/* 		game_running = false; */
	/* 	} */

	/* 	switch (c) { */
	/* 	default: */
	/* 		continue; */
	/* 	} */
	/* } */

	return 0;
}
