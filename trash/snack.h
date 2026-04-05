#include <stdio.h>

#define SnackHead 'O'
#define SnackTail 'o'

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
