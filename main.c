#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_ARRAY_SIZE 8
#define DEFAULT_ARRAY_SCALE 2
#define Get_Array_Size(array) (sizeof(array) / sizeof(array[0]))
#define InitArray(array_out, source_list, source_len)                          \
	do {                                                                   \
		(array_out).len = (source_len);                                \
		(array_out).capacity =                                         \
			((source_len) / DEFAULT_ARRAY_SIZE + 1) *              \
			DEFAULT_ARRAY_SIZE;                                    \
		(array_out).data = malloc(sizeof(int) * (array_out).capacity); \
		for (int i = 0; i < (source_len); i++) {                       \
			(array_out).data[i] = (source_list)[i];                \
		}                                                              \
	} while (0)

#define ArrayPush(array, item)                                             \
	do {                                                               \
		if ((array).len == (array).capacity) {                     \
			int new_capacity = (array).capacity == 0 ?         \
						   DEFAULT_ARRAY_SIZE :    \
						   ((array).capacity *     \
						    DEFAULT_ARRAY_SCALE);  \
			void *new_data = realloc((array).data,             \
						 sizeof((array).data[0]) * \
							 new_capacity);    \
			if (new_data != NULL) {                            \
				(array).data = new_data;                   \
				(array).capacity = new_capacity;           \
			}                                                  \
		}                                                          \
		if ((array).len < (array).capacity) {                      \
			array.data[array.len] = (item);                    \
			(array).len++;                                     \
		}                                                          \
	} while (0)

#define Foreach(item, array) \
	for (int item = 0; item < Get_Array_Size(array); ++item)

#define ARRAY_FOREACH(i, list) for (int i = 0; i < (list).len; i++)

typedef struct Character {
	size_t id;
	char *name;
} Character;

typedef struct GameData {
	int *id_pool;
	Character *characters[];
} GameData;

typedef struct Array {
	int *data;
	int len;
	int capacity;
} Array;



int main()
{
	return 0;
}
