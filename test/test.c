#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct BBcode {
	struct BBcode *next;
	struct BBcode *ending;
	char *start;
	char *end;
	char tag[16];
} BBcode;

typedef struct Pair {
	char start;
	char end;
} Pair;

static BBcode *g_bbcode_buffer;

// clang-format off
static const Pair pair_config[] = {
	{ '[', ']' },
	{ '<', '>' },
	{ '{', '}' },
	{ '[', ']' }
};
// clang-format on

BBcode *parse_tag_tail(char *fmt, int pair_select)
{
	BBcode *head = NULL;
	BBcode *current = NULL;

	char *p = fmt;
	while (*p) {
		if (*p == pair_config[pair_select].start) {
			char *start = ++p;
			while (*p && *p != pair_config[pair_select].end)
				p++;
			if (*p == pair_config[pair_select].end) {
				BBcode *new_node =
					(BBcode *)calloc(1, sizeof(BBcode));
				if (!new_node)
					break;

				size_t len = p - start;
				if (len > 15)
					len = 15;
				strncpy(new_node->tag, start, len);
				new_node->tag[len] = '\0';

				if (head == NULL) {
					head = new_node;
					current = head;
				} else {
					current->next = new_node;
					current = new_node;
				}
			}
		}
		if (*p)
			p++;
	}
	return head;
}

BBcode *parse_tag_head(char *fmt, int pair_num)
{
	BBcode *head = NULL;

	char *p = fmt;
	while (*p) {
		if (*p == pair_config[pair_num].start) {
			BBcode *new_node = (BBcode *)calloc(1, sizeof(BBcode));
			new_node->start = ++p;
			while (*p != pair_config[pair_num].end) {
				++p;
			}
			if (*p == pair_config[pair_num].end) {
				new_node->end = p;
				size_t len = ((p - new_node->start) < 15) ?
						     (p - new_node->start) :
						     15;
				strncpy(new_node->tag, new_node->start, len);

				new_node->tag[len] = '\0';

				if (head == NULL)
					head = new_node;
				else {
					new_node->next = head;
					head = new_node;
				}
			} else {
				free(new_node);
			}
		}
		++p;
	}

	return head;
}

BBcode *parse_bbcode(char *fmt)
{
	return parse_tag_head(fmt, 0);
}

BBcode *match_bbcode_rec(BBcode *tail, size_t depth)
{
	if (tail == NULL) {
		return NULL;
	}
	BBcode *head = tail->next;
	BBcode *current = tail;

	while (head != NULL) {
		if (head->tag[0] == '/') {
			depth++;
			head = match_bbcode_rec(head, depth);
			depth--;
		} else {
			if (strcmp(head->tag, current->tag + 1) == 0) {
				head->ending = current;
				if (depth > 0)
					return head->next;
				current = head->next;
				head = current->next;
			}
		}
	}

	return head;
}

BBcode *match_bbcode(BBcode *tail)
{
	return match_bbcode_rec(tail, 0);
}

void test_inspect_bbcode(BBcode *head)
{
	printf("\n=== inspect_bbcode test ===\n");
	printf("Input tags (reversed, head-insert order):\n");
	BBcode *p = head;
	while (p) {
		printf("  tag: %-16s ending: %s\n", p->tag,
		       p->ending ? p->ending->tag : "(null)");
		p = p->next;
	}

	match_bbcode(head);

	printf("After inspect:\n");
	p = head;
	while (p) {
		printf("  tag: %-16s ending: %s\n", p->tag,
		       p->ending ? p->ending->tag : "(null)");
		p = p->next;
	}
	printf("===========================\n\n");
}

void free_bbcode_buffer()
{
	while (g_bbcode_buffer) {
		BBcode *tmp = g_bbcode_buffer;
		g_bbcode_buffer = g_bbcode_buffer->next;
		free(tmp);
	}
}

int main()
{
	g_bbcode_buffer = (BBcode *)malloc(sizeof(BBcode));
	char text[] = "[i]Hello, World[b][/b][i][/i]";

	g_bbcode_buffer = parse_bbcode(text);
	BBcode *p = g_bbcode_buffer;
	while (p) {
		printf("Found tag: %s\n", p->tag);
		p = p->next;
	}

	test_inspect_bbcode(g_bbcode_buffer);

	// free memory
	while (g_bbcode_buffer) {
		BBcode *tmp = g_bbcode_buffer;
		g_bbcode_buffer = g_bbcode_buffer->next;
		free(tmp);
	}

	return 0;
}
