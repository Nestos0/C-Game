#pragma once
#include "module.h"
#include <sys/types.h>

typedef struct ScreenInfo {
	int width;
	int height;
	struct {
		int x;
		int y;
	} curser;
} ScreenInfo;

extern ScreenInfo screen;

int screen_clear(void);

void get_terminal_size(int *width, int *height);

void terminal_restore();
