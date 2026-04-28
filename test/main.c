#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

enum {
	DEFAULT_S_SIZE = 128,
	DEFAULT_COMMON_SIZE = 16,
	DEFAULT_TAG_SIZE = DEFAULT_COMMON_SIZE,
	DEFAULT_S_LIST_SIZE = DEFAULT_COMMON_SIZE,
	DEFAULT_TAG_BUFFER_CAPACITY = 16,
	BB_DOC_DEFAULT_CAPACITY = 16
};

// clang-format off
static const struct {
	char start;
	char end;
} pair_map[] = {
	{ '[', ']' },
	{ '<', '>' },
	{ '{', '}' },
	{ '[', ']' }
};
// clang-format on

typedef struct bbcode {
	char *name;
	struct bbcode *next;
	struct bbcode *pair;
	size_t seqence;
	bool is_closing;
} bbcode;

typedef struct string_b {
	char **paragraph;
	bbcode **node_buf;
	struct string_b *next;
} string_b;

string_b *tag_parse_rec(const char *format, int pair_seq)
{
	string_b *ret = calloc(1, sizeof(string_b) + BB_DOC_DEFAULT_CAPACITY);

	ret->paragraph = (char **)calloc(DEFAULT_S_LIST_SIZE, sizeof(char *));
	char **current_para = ret->paragraph;
	*current_para = (char *)calloc(DEFAULT_S_SIZE, sizeof(char));
	char *dest = current_para[0];

	bbcode *current_node = (bbcode *)calloc(1, sizeof(bbcode));
	ret->node_buf = nullptr;

	const char *p = format;
	while (*p) {
		if (*p == pair_map[pair_seq].start) {
			if (current_node->name[0] != '\0') {
				current_node->next =
					(bbcode *)calloc(1, sizeof(bbcode));
				current_node = current_node->next;
			}

			if (dest == *current_para) {
				current_node->seqence =
					(current_para - ret->paragraph);
			} else {
				*(++current_para) = (char *)calloc(
					DEFAULT_S_SIZE, sizeof(char));
				current_node->seqence =
					(current_para - ret->paragraph);
				dest = *current_para;
			}

			char *tag_ptr = current_node->name;

			while (*(++p) != pair_map[pair_seq].end &&
			       (current_node->name - tag_ptr) <
				       DEFAULT_TAG_SIZE) {
				*tag_ptr++ = *p;
			}

			++p;
			continue;
		}
		*dest++ = *p++;
	}

	if (dest == *current_para) {
		if (current_para > ret->paragraph) {
			free(*current_para);
			*current_para = nullptr;
			current_para = nullptr;

			if (current_node != nullptr) {
				current_node->seqence = -1;
			}
		} else {
			// do nothing
		}
	}

	return ret;
}

string_b *bbcode_parse(const char *format)
{
	return tag_parse_rec(format, 0);
}

char *interpret_tag(const char *format)
{
	string_b *dest = bbcode_parse(format);
	char *ret = (char *)calloc(strlen(format) * 1.5, sizeof(char));
	char **para_ptr = dest->paragraph;
	while (*para_ptr) {
		int len = strlen(*para_ptr);
		strncpy(ret, *para_ptr, len);
		ret += len;
		++para_ptr;
	}
	return ret;
}

int printf_b(const char *format, ...)
{
	int done;
	va_list arg;
	char *interpreted = interpret_tag(format);

	va_start(arg, format);
	if (interpreted && interpreted[0] != '\0') {
		done = vfprintf(stdout, interpreted, arg);
	} else
		done = vfprintf(stdout, format, arg);
	va_end(arg);

	if (interpreted) {
		free(interpreted);
	}

	return done;
}

int main()
{
	char text[] =
		"[red]Void [while]Linux[/white][/red][blue][white]Admin[/white]Nestos[/blue]";

	printf("================================\n");
	printf("Test Start");
	printf_b("%s", text);

	printf("================================\n");
	return 0;
}
