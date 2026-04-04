#include <termios.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[33m"
#define RESET "\033[0m"


void clear();

void set_unbuffered_mode();

void display_field(int rows, int cols, int field[rows][cols]);

void move_cursor(int x, int y);
