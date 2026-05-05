#pragma once

typedef struct GameState {
	bool initialized;
	bool is_running;
} GameState;

extern struct GameState game_state;

void game_loop();

int kbhit();

void process_input(void);

void update_game(void);

void init_game();

void game_refresh_ui();
