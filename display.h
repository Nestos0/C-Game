#include <termios.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

void clear();

void set_unbuffered_mode();

void display_field(int rows, int cols, int field[rows][cols]);

void move_cursor(int x, int y);
