#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include "utils.h"

#define DEFAULT_S_BUFFER_SIZE 1024

#define DEFAULT_S_DATA_SIZE 255

typedef struct String {
	size_t width;
	size_t mcapacity;
	struct String *next;
	char *data;
} String;

extern String *g_string_head;

String *string_wrapper(char *str);

void batch_free();

int is_full_width(wchar_t wc);

int get_strwidth(char *str);

int get_strwidth_spec(char *str);

int set_string(String *obj, char *str);

#endif
