#include <stdio.h>

#define SnackHead 'O'
#define SnackTail 'o'

typedef struct Point {
	int x;
	int y;
} Point;

typedef struct Snack {
	int x;
	int y;
	struct {
		struct Point *data;
		int capacity;
		int len;
	};
} Snack;
