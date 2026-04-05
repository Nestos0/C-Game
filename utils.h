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

#define ARRAY_FOREACH(i, list) for (int i = 0; i < (list).len; i++)

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

typedef struct {
	char *data;
	int len;
	int capacity;
} String;

String *string_new(int capacity);

void string_free(String *s);
