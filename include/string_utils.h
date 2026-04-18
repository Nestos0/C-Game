#pragma once
#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#define DEFAULT_S_BUFFER_SIZE 1024

#define DEFAULT_STRING_CAPACITY 255

typedef struct String {
	size_t width;
	size_t mlen;
	size_t mcapacity;
	bool is_flex;
	union {
		char *data;
		char flex_data[];
	};
} String;

extern void *g_memory_buffer[];
extern int g_buffer_count;

String *string_wrapper(char *str);

void batch_free();

int is_full_width(wchar_t wc);

int get_strwidth(char *str);

int get_strwidth_spec(const char *str);

int set_string(String *obj, const char *str);

char *get_string_data(String *obj_ptr);

#define String_set(obj_ptr, str_val, ...)                                      \
	do {                                                                   \
		bool _is_f = false;                                            \
		char _dummy[] = "" #__VA_ARGS__;                               \
		if (sizeof(_dummy) > 1)                                        \
			_is_f = true;                                          \
                                                                               \
		if (_is_f) {                                                   \
			size_t _total =                                        \
				sizeof(String) + DEFAULT_STRING_CAPACITY;      \
			(obj_ptr) = (String *)safe_malloc(_total);             \
			if (obj_ptr) {                                         \
				memset(obj_ptr, 0, sizeof(String));            \
				(obj_ptr)->is_flex = true;                     \
				(obj_ptr)->mcapacity =                         \
					DEFAULT_STRING_CAPACITY;               \
				strncpy((obj_ptr)->flex_data, str_val,         \
					DEFAULT_STRING_CAPACITY - 1);          \
				(obj_ptr)->flex_data[DEFAULT_STRING_CAPACITY - \
						     1] = '\0';                \
				(obj_ptr)->mlen =                              \
					strlen((obj_ptr)->flex_data);          \
				(obj_ptr)->width = get_strwidth_spec(          \
					(obj_ptr)->flex_data);                 \
			}                                                      \
		} else {                                                       \
			(obj_ptr) = (String *)safe_calloc(1, sizeof(String));  \
			if (obj_ptr)                                           \
				set_string(obj_ptr, str_val);                  \
		}                                                              \
	} while (0)

#define Get_string_data(obj_ptr) \
	((obj_ptr)->is_flex ? (obj_ptr)->flex_data : (obj_ptr)->data)

#endif
