#include "display.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>

#define ERR_NONE 0
#define ERR_COL (1 << 0)
#define ERR_ROW (1 << 1)

#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RESET "\033[0m"

#define REQUIRED_COLS 100
#define REQUIRED_ROWS 100

#define Init_Vector2d(matrix, rows, cols)        \
	for (int r = 0; r < rows; r++) {         \
		for (int c = 0; c < cols; c++) { \
			matrix[r][c] = '#';      \
		}                                \
	}

struct GameConfig {
	int target_fps;
};

typedef struct {
	int x;
	int y;
	int h;
	int w;
	bool border;
	struct {
		int border_width;
	};

} TextBox;

void setfps(int fps)
{
	usleep(1000000 / fps);
}

void init_textbox(TextBox *box)
{
	struct winsize w;
	char status = ERR_NONE;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != 0) {
		perror("ioctl");
		return;
	}

	if (w.ws_col < REQUIRED_COLS)
		status |= ERR_COL;
	if (w.ws_row < REQUIRED_ROWS)
		status |= ERR_ROW;

	if (status) {
	} else {
		box->x = 0;
		box->y = 0;
		box->w = w.ws_col / 5;
		box->h = w.ws_row / 5;
	}
}

int main()
{
	bool game_running = true;
	struct GameConfig config = { 60 };

	TextBox tellbox;

	clear();
	set_unbuffered_mode();
	init_textbox(&tellbox);

	char c = 0;

	while (game_running) {
		c = getchar();
		if (c == 4) {
			game_running = false;
		}
		setfps(config.target_fps);
	}

	printf("\n");

	return 0;
}
