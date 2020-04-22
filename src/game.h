#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "bool.h"
#include "history.h"
#include "lp.h"

typedef enum game_mode { GM_EDIT, GM_SOLVE, GM_INIT } game_mode_t;

typedef struct game {
    game_mode_t mode;
    bool_t mark_errors;
    board_t board;
    history_t history;
    lp_env_t lp_env;
} game_t;

#endif
