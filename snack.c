#include "snack.h"
#include "display.h"

void draw_snack(Snack *s)
{
	move_cursor(s->x, s->y);
	printf("%c", SnackHead);
	fflush(stdout);
}

void move_snack(Snack *s, int v_x, int v_y)
{
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
