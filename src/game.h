#ifndef GAME_H
#define GAME_H

#include "board.h"

typedef enum game_mode { GM_EDIT, GM_SOLVE, GM_INIT } game_mode_t;

typedef struct game {
    game_mode_t mode;
    board_t board;
} game_t;

#endif
