#ifndef MAINAUX_H
#define MAINAUX_H

#include "bool.h"
#include "game.h"
#include "parser.h"

/**
 * Display a parser error to the user.
 */
void print_parser_error(command_t* cmd, parser_error_codes_t error);

/**
 * Initialize a new game, printing any error messages and returning whether
 * initialization succeeded.
 */
bool_t init_game(game_t* game);

/**
 * Clean up any resources owned by `game`.
 */
void destroy_game(game_t* game);

/**
 * Execute a command, prints to stdout all output.
 */
bool_t command_execute(game_t* game, command_t* command);

#endif
