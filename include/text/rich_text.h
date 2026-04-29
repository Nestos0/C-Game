#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	BB_PARSE_STACK = 16,
	BB_INIT_CAP = 16,
	BB_DOC_DEFAULT_CAPACITY = 16,
	BB_TAG_NAME_MIN_CAP = 8,
	BB_TAG_LENGTH = 16,
	STRING_DYNAMIC_MIN_CAP = 128
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

const char *get_ansi_code(const char *name);

const char *get_ansi_code_safe(const char *name, size_t len);

int get_ansi_code_seq(const char *name);

int get_ansi_code_seq_safe(const char *name, size_t len);

char *bbcode_parse_stream(const char *format);

char *bbcode_interpret_spec(const char *format);

int printf_bbcode(const char *format, ...);
