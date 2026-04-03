#include "display.h"
#include "snack.h"

#define Init_Vector2d(matrix, rows, cols)        \
	for (int r = 0; r < rows; r++) {         \
		for (int c = 0; c < cols; c++) { \
			matrix[r][c] = '#';      \
		}                                \
	}

int v_x = 1;
int v_y = 0;

int main()
{
	_Bool game_running = true;
	clear();
	set_unbuffered_mode();

	char c = 0;

	Snack snack = { .x = 10, .y = 10 };
	int gamefield[20][20];
	Init_Vector2d(gamefield, 20, 20);

	while (game_running) {
		c = getchar();
		if (c == 4) {
			game_running = false;
		}
		switch (c) {
		case 'w':
			v_x = 0;
			v_y = -1;
			break;
		case 'r':
			v_x = 0;
			v_y = 1;
			break;
		case 'a':
			v_x = -1;
			v_y = 0;
			break;
		case 's':
			v_x = 1;
			v_y = 0;
			break;
		case 27:
			game_running = false;
			break;
		}

		move_cursor(0, 0);
		display_field(20, 20, gamefield);
		draw_snack(&snack);
		move_snack(&snack, v_x, v_y);

		move_cursor(0, 21);
		printf("坐标: x:%d, y:%d (按 Ctrl+D 退出)\n", snack.x, snack.y);
		printf("%d %c", c, c);
		usleep(100000);
	}

	printf("\n");
	return 0;
}
