#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	BB_PARSE_STACK = 16,
	BB_INIT_CAP = 16,
	BB_DOC_DEFAULT_CAPACITY = 16,
	BB_TAG_NAME_MIN_CAP = 8
};

typedef struct BBcode {
	char *name;
	struct bbcode *next;
	struct bbcode *pair;
	bool is_closing;
	size_t seqence;
} BBcode;

typedef struct {
	const char *ptr;
	size_t len;
} Segment;

typedef struct BBcodeContext {
	char *content;
	Segment **segment;
	BBcode **bbcode_buffer;
	size_t seg_count;
	size_t bb_count;
} BBcodeContext;

struct {
	char start;
	char end;
} Pairs[] = {
	{ '[', ']' },
	{ '<', '>' }
};

typedef struct {
	const char *name;
	const char *code;
} AnsiMap;

// clang-format off
static const AnsiMap ANSI_MAP[] = {
    { "black",   "\x1b[30m" }, { "red",     "\x1b[31m" }, { "green",  "\x1b[32m" },
    { "yellow",  "\x1b[33m" }, { "blue",    "\x1b[34m" }, { "magenta", "\x1b[35m" },
    { "cyan",    "\x1b[36m" }, { "white",   "\x1b[37m" }, { "default", "\x1b[39m" },
    { "reset",   "\x1b[0m"  }, { "bold",    "\x1b[1m"  }, { "dim",     "\x1b[2m"  },
    { "italic",  "\x1b[3m"  }, { "underline","\x1b[4m" }, { "blink",   "\x1b[5m"  },
    { "reverse", "\x1b[7m"  }, { "strike",  "\x1b[9m"  }, { "bg_black","\x1b[40m" },
    { "bg_red",  "\x1b[41m" }, { "bg_green","\x1b[42m" }, { "bg_yellow","\x1b[43m"}
};
// clang-format on

const char *get_ansi_code(const char *name)
{
	if (name == nullptr)
		return "";

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return ANSI_MAP[i].code;
	}

	return "";
}

int get_ansi_code_seq(const char *name)
{
	if (name == nullptr)
		return -1;

	size_t map_size = sizeof(ANSI_MAP) / sizeof(ANSI_MAP[0]);

	for (size_t i = 0; i < map_size; i++) {
		if (strcmp(name, ANSI_MAP[i].name) == 0)
			return i;
	}
	return -1;
}

int bbcode_init(BBcode *ctx)
{
	if (ctx == nullptr)
		return -1;

	ctx->name = calloc(BB_TAG_NAME_MIN_CAP, sizeof(char));
	ctx->is_closing = false;

	ctx->next = nullptr;
	ctx->pair = nullptr;
	ctx->seqence = -1;

	return 0;
}

void bbcode_free_single(BBcode *ctx)
{
	if (ctx == nullptr)
		return;
	if (ctx->name != nullptr)
		free(ctx->name);

	ctx->pair = nullptr;
}

__attribute__((malloc)) BBcode *bbcode_new()
{
	BBcode *new_node = malloc(sizeof(BBcode));
	bbcode_init(new_node);

	return new_node;
}

void bbcode_context_free(BBcodeContext *ctx)
{
	Segment **sp = ctx->segment;
	while (*sp) {
		free(*sp);
		++sp;
	}

	if (ctx->bbcode_buffer != nullptr) {
		for (size_t i = 0; i < ctx->bb_count; i++) {
			if (ctx->bbcode_buffer[i] != nullptr) {
				free(ctx->bbcode_buffer[i]);
			}
		}
	}

	if (ctx->bbcode_buffer != nullptr) {
		for (size_t i = 0; i < ctx->seg_count; i++) {
			if (ctx->segment[i] != nullptr) {
				free(ctx->segment[i]);
			}
		}
	}

	if (ctx->content)
		free(ctx->content);

	free(ctx);
}

int bbcode_context_init(BBcodeContext *ctx)
{
	if (ctx == nullptr)
		return -1;

	ctx->content = nullptr;
	ctx->seg_count = 0;
	ctx->bb_count = 0;

	ctx->bbcode_buffer = (BBcode **)malloc(BB_INIT_CAP * sizeof(BBcode *));
	ctx->segment = (Segment **)malloc(BB_INIT_CAP * sizeof(Segment *));

	if (ctx->bbcode_buffer == nullptr) {
		free(ctx->segment);
		free(ctx->bbcode_buffer);
		return -1;
	}

	memset(ctx->bbcode_buffer, 0, BB_INIT_CAP * sizeof(BBcode *));
	memset(ctx->segment, 0, BB_INIT_CAP * sizeof(Segment *));
	return 0;
}

BBcodeContext *tag_parse(const char *format, int seq)
{
	BBcodeContext *ret = malloc(sizeof(BBcodeContext));
	bbcode_context_init(ret);
	strcpy(ret->content, format);

	BBcode *new_node = bbcode_new();

	Segment **cur_seg = ret->segment;

	const char *p = format;
	const char *start = format;
	char *dest = ret->content;
	while (*p) {
		if (*p == Pairs[seq].start) {
			// Intercept segment
			(*cur_seg)->ptr = start;
			(*cur_seg)->len = p - start;
			++(ret->seg_count);

			// Skip start of tag parentheses; Set `start` pointer to start of tag name
			start = ++p;
			while (*p && *p != Pairs[seq].end)
				++p;

			int len = p - start;
			char tag[len + 1];
			memcpy(tag, start, len);

			int seq = get_ansi_code_seq(tag);
			if (seq != -1) {
				strcpy(new_node->name, tag);
				strcpy(dest, ANSI_MAP[seq].code);
				++p;
				start = p;
				continue;
			} else {
				strncpy(dest, start - 1, (p - start + 1));
				continue;
			}
		}
		*dest++ = *p++;
	}

	return ret;
}

static inline BBcodeContext *bbcode_parse(const char *format)
{
	return tag_parse(format, 0);
}

char *bbcode_interpret(const char *format)
{
	BBcodeContext *ctx = bbcode_parse(format);
	if (ctx->content) {
		if (ctx->bbcode_buffer != nullptr) {
			for (size_t i = 0; i < ctx->bb_count; i++) {
				if (ctx->bbcode_buffer[i] != nullptr) {
					free(ctx->bbcode_buffer[i]);
				}
			}
		}

		if (ctx->bbcode_buffer != nullptr) {
			for (size_t i = 0; i < ctx->seg_count; i++) {
				if (ctx->segment[i] != nullptr) {
					free(ctx->segment[i]);
				}
			}
		}

		free(ctx);

		return ctx->content;
	}

	return "";
}

int printf_bbcode(const char *format, ...)
{
	int done;
	char *interpreted = bbcode_interpret(format);

	va_list arg;
	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	va_end(arg);
	return done;
}

int main()
{
	printf("================\n");
	printf_bbcode("[red]Red Text[/red]Admin [blue]print some Paratheses {}[/blue]\n");
	printf("================\n");
	return 0;
}
