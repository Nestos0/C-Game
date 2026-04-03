#include <stdio.h>

#define SnackHead 'O'
#define SnackTail 'o'
#define RED "\033[31m"
#define GREEN "\033[32m"
#define RESET "\033[0m"

typedef struct {
	int x;
	int y;
} Snack;

void draw_snack(Snack *s);

void move_snack(Snack *s, int v_x, int v_y);
