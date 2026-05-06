/* --- File: ./include/game.h --- */
#pragma once
#include "ui/display.h"

typedef void (*Game_InitFn)(void);
typedef void (*Game_HandleCmdFn)(const char *cmd);
typedef void (*Game_RenderLogsFn)(struct Screen *s, int x, int y, int width, int height);

typedef struct GameInterface {
    Game_InitFn init;
    Game_HandleCmdFn handle_command;
    Game_RenderLogsFn render_logs;
} GameInterface;

void engine_register_game_interface(const GameInterface *iface);
