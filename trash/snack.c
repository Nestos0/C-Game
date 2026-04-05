#include "snack.h"
#include "display.h"

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
