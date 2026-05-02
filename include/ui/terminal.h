#pragma once
#include "ui/display.h"
#include <termios.h>

void get_terminal_size(int *width, int *height);

int term_restore();

bool term_enter_raw(void);

void term_leave_raw(void);

bool term_get_size(int *rows, int *cols);

void ansi_cursor_hide(void);

void ansi_cursor_show(void);

void ansi_clear_screen(void);

void ansi_cursor_goto(int row, int col);

void ansi_set_fg_rgb(int r, int g, int b);

void ansi_set_bg_rgb(int r, int g, int b);

void ansi_reset(void);

void term_set_cell(Screen *s, int x, int y, uint32_t cp, RGB *fg, RGB *bg);
