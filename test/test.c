#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

enum {
	DEFAULT_S_SIZE = 128,
	DEFAULT_COMMON_SIZE = 16,
	DEFAULT_TAG_SIZE = DEFAULT_COMMON_SIZE,
	DEFAULT_S_LIST_SIZE = DEFAULT_COMMON_SIZE
};

typedef struct bbcode {
	bool is_suffix;
	char tag[DEFAULT_TAG_SIZE];
	struct bbcode *next;
	struct bbcode *pair;
	size_t depth;
	int8_t startof;
} bbcode;

typedef struct string_bbcode {
	char *original;
	char **filtered;
	bbcode *node_head;
	struct string_bbcode *next;
} string_bbcode;

typedef struct color_stack {
	char **data;
	int top;
	int capacity;
} color_stack;

struct Pair {
	char start;
	char end;
};


// clang-format off
static const struct Pair pair_map[] = {
	{ '[', ']' },
	{ '<', '>' },
	{ '{', '}' },
	{ '[', ']' }
};
// clang-format on

string_bbcode *parse_tag_rec(const char *format, int pair_seq)
{
	string_bbcode *ret =
		calloc(1, sizeof(string_bbcode) + DEFAULT_TAG_SIZE);
	ret->original = (char *)calloc(strlen(format) + 1, sizeof(char));
	strcpy(ret->original, format);

	ret->filtered = (char **)calloc(DEFAULT_S_LIST_SIZE, sizeof(char *));
	char **current_para = ret->filtered;
	*current_para = (char *)calloc(DEFAULT_S_SIZE, sizeof(char));
	char *dest = current_para[0];

	bbcode *current_node = (bbcode *)calloc(1, sizeof(bbcode));
	ret->node_head = current_node;

	char *p = ret->original;
	while (*p) {
		if (*p == pair_map[pair_seq].start) {
			if (current_node->tag[0] != '\0') {
				current_node->next =
					(bbcode *)calloc(1, sizeof(bbcode));
				current_node = current_node->next;
			}

			if (dest == *current_para) {
				current_node->startof =
					(current_para - ret->filtered);
			} else {
				*(++current_para) = (char *)calloc(
					DEFAULT_S_SIZE, sizeof(char));
				current_node->startof =
					(current_para - ret->filtered);
				dest = *current_para;
			}

			char *tag_ptr = current_node->tag;

			while (*(++p) != pair_map[pair_seq].end &&
			       (current_node->tag - tag_ptr) <
				       DEFAULT_TAG_SIZE) {
				*tag_ptr++ = *p;
			}

			++p;
			continue;
		}
		*dest++ = *p++;
	}
	if (dest == *current_para) {
		if (current_para > ret->filtered) {
			free(*current_para);
			*current_para = nullptr;
			current_para = nullptr;

			if (current_node != nullptr) {
				current_node->startof = -1;
			}
		} else {
			// do nothing
		}
	}

	ret->next = nullptr;
	return ret;
}

string_bbcode *parse_bbcode(const char *format)
{
	return parse_tag_rec(format, 0);
}

bbcode *match_bbcode_rec(bbcode *h_node, size_t depth)
{
	if (h_node == nullptr) {
		return nullptr;
	}

	size_t cur_depth = depth;
	bbcode *current = h_node;
	bbcode *ptr = h_node->next;

	current->depth = depth;
	while (current != nullptr && ptr != nullptr) {
		if (ptr->tag[0] != '/') {
			ptr = match_bbcode_rec(ptr, cur_depth + 1);
		} else {
			if (strcmp(current->tag, ptr->tag + 1) == 0) {
				ptr->depth = cur_depth;
				if (depth > 0)
					return ptr->next;
				current = ptr->next;
				if (current != nullptr) {
					current->depth = cur_depth;
					ptr = current->next;
				}
			} else {
				ptr->depth = depth;
				ptr = ptr->next;
			}
		}
	}

	return ptr;
}

static inline bbcode *match_bbcode(bbcode *buf)
{
	return match_bbcode_rec(buf, 0);
}

void free_bbcode_buffer(string_bbcode *buf)
{
	while (buf) {
		string_bbcode *tmp = buf;
		buf = buf->next;
		free(tmp);
	}
}

// clang-format off
const char* get_ansi_color(const char* tag) {
	if (!tag || tag[0] == '\0') return "";
	if (tag[0] == '/') return "\x1b[0m";
	
	if (strcmp(tag, "red") == 0) return "\x1b[31m";
	if (strcmp(tag, "green") == 0) return "\x1b[32m";
	if (strcmp(tag, "yellow") == 0) return "\x1b[33m";
	if (strcmp(tag, "blue") == 0) return "\x1b[34m";
	if (strcmp(tag, "magenta") == 0) return "\x1b[35m";
	if (strcmp(tag, "cyan") == 0) return "\x1b[36m";
	if (strcmp(tag, "white") == 0) return "\x1b[37m";
	
	return "";
}
// clang-format on

color_stack *create_stack(int cap)
{
	color_stack *s = (color_stack *)malloc(sizeof(color_stack));
	s->data = (char **)calloc(cap, sizeof(char *));
	for (int i = 0; i < cap; i++) {
		s->data[i] = (char *)calloc(DEFAULT_TAG_SIZE, sizeof(char));
	}
	s->top = -1;
	s->capacity = cap;
	return s;
}

void free_stack(color_stack *s)
{
	if (!s)
		return;
	for (int i = 0; i < s->capacity; i++) {
		free(s->data[i]);
	}
	free(s->data);
	free(s);
}

void push_color(color_stack *s, char *tag)
{
	if (tag)
		strncpy(s->data[++(s->top)], tag, s->capacity);
}

char *pop_color(color_stack *s)
{
	return (s->top == -1) ? NULL : s->data[(s->top)--];
}

char *bbcode_interpret(string_bbcode *obj)
{
	if (obj == nullptr)
		return nullptr;

	color_stack *stack = create_stack(DEFAULT_S_LIST_SIZE);

	size_t total_len = 0;
	int max_chunks = 0;
	while (obj->filtered && obj->filtered[max_chunks]) {
		total_len += strlen(obj->filtered[max_chunks]);
		max_chunks++;
	}

	bbcode *node = obj->node_head;
	while (node) {
		if (node->tag[0] != '\0') {
			push_color(stack, node->tag);
			total_len += strlen(get_ansi_color(node->tag));
		}
		node = node->next;
	}

	char *ret = (char *)calloc((total_len + 1), sizeof(char));
	if (!ret)
		return nullptr;
	char *dest = ret;

	for (int i = 0; i < max_chunks; i++) {
		node = obj->node_head;
		while (node) {
			if (node->tag[0] != '\0' && node->startof == i) {
				const char *color_code =
					get_ansi_color(node->tag);
				strcpy(dest, color_code);
				dest += strlen(color_code);
			}
			node = node->next;
		}

		if (obj->filtered[i]) {
			strcpy(dest, obj->filtered[i]);
			dest += strlen(obj->filtered[i]);
		}
	}

	while (node) {
		if (node->tag[0] != '\0' && node->startof == -1) {
			const char *color_code = get_ansi_color(node->tag);
			strcpy(dest, color_code);
			dest += strlen(color_code);
		}
		node = node->next;
	}

	free_stack(stack);
	return ret;
}

int print_color(const char *format, ...)
{
	va_list arg;
	int done;

	string_bbcode *string = parse_bbcode(format);
	match_bbcode(string->node_head);

	char *interpreted_fmt = bbcode_interpret(string);

	va_start(arg, format);
	if (interpreted_fmt) {
		done = vfprintf(stdout, interpreted_fmt, arg);
		free(interpreted_fmt);
	} else
		done = vfprintf(stdout, format, arg);
	va_end(arg);

	return done;
}

void inspect_bbcode(bbcode *head)
{
	bbcode *p = head;
	while (p) {
		printf("Found tag: %s\n", p->tag);
		p = p->next;
	}
	printf("\n=== inspect_bbcode test ===\n");

	// 执行匹配并计算深度
	match_bbcode(head);

	printf("After match (Depth info):\n");
	p = head;
	while (p) {
		// 打印标签内容以及它被计算出的深度
		printf("  tag: %-16s depth: %zu\n", p->tag, p->depth);
		p = p->next;
	}
	printf("===========================\n\n");
}

void inspect_string_b(string_bbcode *buf)
{
	printf("===========================\n\n");

	bbcode *b_ptr = buf->node_head;
	inspect_bbcode(b_ptr);

	b_ptr = buf->node_head;
	while (b_ptr) {
		int startof = b_ptr->startof;
		printf("Tag: %s; ", b_ptr->tag);
		if (startof >= 0)
			printf("Startof: \"%s\"; ", buf->filtered[startof]);
		putchar('\n');

		b_ptr = b_ptr->next;
	}

	printf("===========================\n\n");

	printf("%s\n", buf->original);
	char **p = buf->filtered;
	while (*p) {
		printf("%s\n", *p);
		p++;
	}

	printf("=== print_color Function Test Samples ===\n");

	print_color(
		"[green]>> Status:[/green] Process [yellow]%s[/yellow] completed successfully!\n",
		"parser_module");

	print_color(
		"[cyan]=> Detected [magenta]%d[/magenta] tags in total.[/cyan]\n",
		4);

	print_color("[blue]>> Table Formatter:[/blue] ID: %04d | Name: %-10s\n",
		    42, "C_Parser");

	print_color(
		">> Original Re-Print: [red]Red Text[/red]r[blue]Blue Text[/blue]\n");
	printf("===========================\n\n");
}

int main()
{
	char text[] =
		"[red][white]Text[/white][/red]r[blue]Blue Text[/blue]";

	string_bbcode *string = parse_bbcode(text);

	inspect_string_b(string);

	return 0;
}
