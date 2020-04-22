#include "board.h"
#include "bool.h"
#include "game.h"
#include "history.h"
#include "lp.h"
#include "mainaux.h"
#include "parser.h"

int main() {
    game_t state;
    bool_t should_continue = TRUE;
    command_t cmd;
    parser_error_codes_t parse_status;

    if (!init_game(&state)) {
        return 1;
    }

    while (should_continue) {
        do {
            print_prompt(&state);
            parse_status = parse_line(stdin, &cmd, state.mode);
        } while (parse_status == P_IGNORE);

        if (parse_status != P_SUCCESS) {
            print_parser_error(&cmd, parse_status);
            continue;
        }

        should_continue = command_execute(&state, &cmd);
    }

    destroy_game(&state);

    return 0;
}
