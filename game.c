#include "display.h"
#include "snack.h"
#include <stdlib.h>

#define Init_Vector2d(matrix, rows, cols)        \
	for (int r = 0; r < rows; r++) {         \
		for (int c = 0; c < cols; c++) { \
			matrix[r][c] = '#';      \
		}                                \
	}

#define Foreach(item, array) \
	for (int item = 0; item < Get_Array_Size(array); ++item)

#define SFOREACH_2(item, len) for (int item = 0; item < (len); ++item)

#define SFOREACH_3(item, len, start) \
	for (int item = (start); item < (len); ++item)

#define SFOREACH_4(item, len, start, end)                                \
	for (int item = (start); item < ((end) > (len)) ? (end) : (len); \
	     item += (step))

#define GET_MACRO(_1, _2, _3, _4, NAME, ...) NAME

#define SForeach(...) \
	GET_MACRO(__VA_ARGS__, SFOREACH_4, SFOREACH_3, SFOREACH_2)(__VA_ARGS__)

int v_x = 1;
int v_y = 0;

void draw_snack(Snack *s)
{
	move_cursor(s->x, s->y);
	printf("%s%c%s", RED, SnackHead, RESET);
	fflush(stdout);
}

void draw_tail(Snack *s)
{
	SForeach(i, s->len)
	{
		Point cord = s->data[i];
		move_cursor(cord.x, cord.y);
		printf("%s%c%s", BLUE, SnackTail, RESET);
		fflush(stdout);
	}
}

void move_snack(Snack *s, int v_x, int v_y)
{
	Point *p = s->data;
	if (s->len) {
		SForeach(i, s->len, 1)
		{
			(p + 1)->x = p->x;
			(p + 1)->y = p->y;
			p++;
		}
	}
	s->data->x = s->x;
	s->data->y = s->y;

	s->x += v_x;
	s->y += v_y;

	if (s->x >= 20)
		s->x = 0;
	if (s->x < 0)
		s->x = 19;
	if (s->y >= 20)
		s->y = 0;
	if (s->y < 0)
		s->y = 19;
}

int main()
{
	_Bool game_running = true;
	clear();
	set_unbuffered_mode();

	char c = 0;

	Snack snack = { .x = 10, .y = 10 };
	snack.capacity = 10;
	snack.len = 2;
	snack.data = (Point *)malloc(sizeof(Point) * snack.capacity);
	snack.data[0].x = 9;
	snack.data[0].y = 10;
	snack.data[1].x = 8;
	snack.data[1].y = 10;
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
		draw_tail(&snack);
		move_snack(&snack, v_x, v_y);

		move_cursor(0, 21);
		printf("坐标: x:%d, y:%d (按 Ctrl+D 退出)\n", snack.x, snack.y);
		SForeach(i, snack.len)
		{
			printf("坐标: x:%d, y:%d (按 Ctrl+D 退出)\n",
			       snack.data[i].x, snack.data[i].y);
		}
		printf("%d %c", c, c);
		usleep(100000);
	}

	printf("\n");

	free(snack.data);
	return 0;
}
