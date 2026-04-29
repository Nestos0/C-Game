#include "text/rich_text.h"

const char *get_ansi_code(const char *name)
{
	if (name == NULL)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

const char *get_ansi_code_safe(const char *name, size_t len)
{
	if (name == NULL)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strncmp(name, ANSI_MAP[i].name, len) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

int get_ansi_code_seq(const char *name)
{
	if (name == NULL)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return i;
	}
	return -1;
}

int get_ansi_code_seq_safe(const char *name, size_t len)
{
	if (name == NULL)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strncmp(name, ANSI_MAP[i].name, len) == 0)
			return i;
	}
	return -1;
}

char *bbcode_parse_stream(const char *format)
{
	size_t in_len = strlen(format);
	size_t out_cap = in_len * 2 + 64;
	char *ret = calloc(1, out_cap);
	if (!ret)
		return NULL;

	char stack[BB_PARSE_STACK][BB_TAG_LENGTH] = { 0 };
	int top = -1;

	const char *p = format;
	char *dest = ret;
	while (*p) {
		if (*p == '[') {
			const char *start = ++p;
			while (*p && *p != ']')
				++p;

			if (*p == ']') {
				size_t tag_len = p - start;
				if (tag_len < BB_TAG_LENGTH) {
					char tag[BB_TAG_LENGTH] = { 0 };
					strncpy(tag, start, tag_len);

					if (tag[0] != '/') {
						const char *code = get_ansi_code_safe(tag, tag_len);
						if (code[0] == '\0') {
							strncpy(dest, --start, tag_len + 2);
							dest += tag_len + 2;
						}

						if (code[0] != '\0') {
							strcpy(dest, code);
							dest += strlen(code);
							strcpy(stack[++top], tag);
						}
					} else {

						if (strcmp(stack[top], tag + 1) == 0) {
							--top;
							strcpy(dest, "\x1b[0m");
							dest += strlen("\x1b[0m");
						}
					}
				} else {
					--start;
					size_t len = p - start + 1;
					strncpy(dest, start, len);
					dest += len;
				}
			}

			++p;
			continue;
		}
		*dest++ = *p++;
	}

	strcpy(dest, "\x1b[0m");
	dest += strlen("\x1b[0m");

	return ret;
}

char *bbcode_interpret_spec(const char *format)
{
	char *ret = bbcode_parse_stream(format);
	return ret;
}

int printf_bbcode(const char *format, ...)
{
	int done;
	char *interpreted = bbcode_interpret_spec(format);

	va_list arg;
	va_start(arg, format);
	if (interpreted == NULL) {
		done = vfprintf(stdout, format, arg);
	} else
		done = vfprintf(stdout, interpreted, arg);
	va_end(arg);

	if (interpreted != NULL) {
		free(interpreted);
	}

	return done;
}
