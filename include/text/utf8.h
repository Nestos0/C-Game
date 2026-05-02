#pragma once
#include <stdint.h>

int utf8_decode(const char *s, uint32_t *cp);

int utf8_encode(uint32_t cp, char *out);

int cp_display_width(uint32_t cp);
