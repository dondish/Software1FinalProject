#include "parser.h"

#include "game.h"
#include <assert.h>
#include <bits/types/FILE.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static FILE* fill_stream(const char* contents) {
    FILE* stream = tmpfile();
    fputs(contents, stream);
    rewind(stream);
    return stream;
}

static void test_parse_line_too_long() {
    const char shrekd[] = "Somebody once told me"
                          "The world is gonna roll me"
                          "I ain't the sharpest tool in the shed"
                          "She was looking kinda dumb"
                          "With her finger and her thumb"
                          "In shape of an \"L\" on her forehead"
                          "Well the years start coming"
                          "And they don't stop coming"
                          "Fed to the rules and I hit the ground running"
                          "Didn't make sense"
                          "Not to live for fun"
                          "Your brain gets smart but your head gets dumb"
                          "So much to do"
                          "So much to see";

    const char exactly_256[] = "06587569129655746311"
                               "31290415728474477519"
                               "71799427945547047796"
                               "12829595051484640916"
                               "51114728430245318916"
                               "12655554069715739134"
                               "03965272076401483045"
                               "93050626794687092461"
                               "90353057657409130794"
                               "09735149730386009315"
                               "13372742274518173040"
                               "487930365151070978698951256512693721\n";

    const char simple[] = "Hello";

    const char line_too_long_then_simple[] =
        "Somebody once told me"
        "The world is gonna roll me"
        "I ain't the sharpest tool in the shed"
        "She was looking kinda dumb"
        "With her finger and her thumb"
        "In shape of an \"L\" on her forehead"
        "Well the years start coming"
        "And they don't stop coming"
        "Fed to the rules and I hit the ground running"
        "Didn't make sense"
        "Not to live for fun"
        "Your brain gets smart but your head gets dumb"
        "So much to do"
        "So much to see\n"
        "solve";
    command_t cmd;
    FILE* stream;

    stream = fill_stream(shrekd);
    assert(parse_line(stream, &cmd, GM_INIT) == P_LINE_TOO_LONG);
    fclose(stream);

    stream = fill_stream(simple);
    assert(parse_line(stream, &cmd, GM_INIT) != P_LINE_TOO_LONG);
    fclose(stream);

    stream = fill_stream(exactly_256);
    assert(parse_line(stream, &cmd, GM_INIT) != P_LINE_TOO_LONG);
    fclose(stream);

    stream = fill_stream(line_too_long_then_simple);
    assert(parse_line(stream, &cmd, GM_INIT) == P_LINE_TOO_LONG);
    assert(parse_line(stream, &cmd, GM_INIT) != P_LINE_TOO_LONG);
    assert(cmd.type == CT_SOLVE);
    fclose(stream);
}

static void test_ignore_blank_line() {
    const char empty[] = "\n";
    const char whitespaced[] = "     ";
    command_t cmd;
    FILE* stream;

    stream = fill_stream(empty);
    assert(parse_line(stream, &cmd, GM_INIT) == P_IGNORE);
    fclose(stream);

    stream = fill_stream(whitespaced);
    assert(parse_line(stream, &cmd, GM_INIT) == P_IGNORE);
    fclose(stream);
}

static void test_eof_exit() {
    command_t command;
    FILE* stream = fill_stream("");
    assert(parse_line(stream, &command, GM_INIT) == P_SUCCESS);
    assert(command.type == CT_EXIT);
}

static void test_too_many_args() {
    const char solve[] = "solve 1 2";
    const char edit[] = "edit 1 2";
    const char mark_errors[] = "mark_errors 1 2";
    const char print_board[] = "print_board 1";
    const char set[] = "set 1 2 3 4";
    const char validate[] = "validate 1";
    const char guess[] = "guess 0.5 4";
    const char generate[] = "generate 2 4 4";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(solve);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(edit);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(mark_errors);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(print_board);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(set);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(validate);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(guess);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);

    stream = fill_stream(generate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    fclose(stream);
}

static void test_parsing_solve() {
    const char only_solve[] = "solve";
    const char solve_correct[] = "solve idk";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(only_solve);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SOLVE);
    fclose(stream);

    stream = fill_stream(solve_correct);
    assert(parse_line(stream, &cmd, GM_INIT) == P_SUCCESS);
    assert(cmd.type == CT_SOLVE);
    assert(cmd.arg.str_val != NULL && !strcmp(cmd.arg.str_val, "idk"));
    fclose(stream);
}

static void test_parsing_edit() {
    const char only_edit[] = "edit";
    const char edit_plus_arg[] = "edit idk";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(only_edit);
    assert(parse_line(stream, &cmd, GM_INIT) == P_SUCCESS);
    assert(cmd.type == CT_EDIT);
    assert(cmd.arg.str_val == NULL);
    fclose(stream);

    stream = fill_stream(edit_plus_arg);
    assert(parse_line(stream, &cmd, GM_INIT) == P_SUCCESS);
    assert(cmd.type == CT_EDIT);
    assert(cmd.arg.str_val != NULL && !strcmp(cmd.arg.str_val, "idk"));
    fclose(stream);
}

static void test_parsing_mark_errors() {
    const char only_mark_errors[] = "mark_errors";
    const char nonbool_mark_errors[] = "mark_errors 2";
    const char nonbool_mark_errors2[] = "mark_errors -1";
    const char nonbool_mark_errors3[] = "mark_errors hi 2";
    const char correct_mark_errors[] = "mark_errors 1";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(only_mark_errors);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_MARK_ERRORS);
    fclose(stream);

    stream = fill_stream(only_mark_errors);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_MODE);
    assert(cmd.type == CT_MARK_ERRORS);
    fclose(stream);

    stream = fill_stream(only_mark_errors);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_MARK_ERRORS);
    fclose(stream);

    stream = fill_stream(nonbool_mark_errors);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_MARK_ERRORS);
    fclose(stream);

    stream = fill_stream(nonbool_mark_errors2);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_MARK_ERRORS);
    fclose(stream);

    stream = fill_stream(nonbool_mark_errors3);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_MARK_ERRORS);
    fclose(stream);

    stream = fill_stream(correct_mark_errors);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_MARK_ERRORS);
    assert(cmd.arg.bool_val == TRUE);
    fclose(stream);
}

static void test_parsing_print_board() {
    const char text[] = "print_board";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(text);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_PRINT_BOARD);
    fclose(stream);

    stream = fill_stream(text);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_PRINT_BOARD);
    fclose(stream);

    stream = fill_stream(text);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_PRINT_BOARD);
    fclose(stream);
}

static void test_parsing_set() {
    const char only_set[] = "set";
    const char set_one_arg[] = "set 1";
    const char set_one_wrong_arg[] = "set x";
    const char set_two_arg[] = "set 1 2";
    const char set_two_wrong_arg[] = "set 1 x";
    const char set_three_arg[] = "set 1 2 3";
    const char set_three_wrong_arg[] = "set 1 2 x";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(only_set);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_SET);
    fclose(stream);

    stream = fill_stream(only_set);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SET);
    fclose(stream);

    stream = fill_stream(set_one_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SET);
    fclose(stream);

    stream = fill_stream(set_one_wrong_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SET);
    fclose(stream);

    stream = fill_stream(set_two_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SET);
    fclose(stream);

    stream = fill_stream(set_two_wrong_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SET);
    fclose(stream);

    stream = fill_stream(set_three_arg);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_SET);
    assert(cmd.arg.three_int_val.i == 1);
    assert(cmd.arg.three_int_val.j == 2);
    assert(cmd.arg.three_int_val.k == 3);
    fclose(stream);

    stream = fill_stream(set_three_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_SET);
    assert(cmd.arg.three_int_val.i == 1);
    assert(cmd.arg.three_int_val.j == 2);
    assert(cmd.arg.three_int_val.k == 3);
    fclose(stream);

    stream = fill_stream(set_three_wrong_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_SET);
    fclose(stream);
}

static void test_parsing_validate() {
    const char validate[] = "validate";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(validate);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_VALIDATE);
    fclose(stream);

    stream = fill_stream(validate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_VALIDATE);
    fclose(stream);

    stream = fill_stream(validate);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_VALIDATE);
    fclose(stream);
}

static void test_parsing_guess() {
    const char only_guess[] = "guess";
    const char one_arg_guess[] = "guess 1";
    const char one_arg_guess_float[] = "guess 0.5";
    const char one_arg_guess_wrong[] = "guess x";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(only_guess);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GUESS);
    fclose(stream);

    stream = fill_stream(one_arg_guess);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_GUESS);
    assert(cmd.arg.float_val == 1);
    fclose(stream);

    stream = fill_stream(one_arg_guess_float);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_GUESS);
    assert(cmd.arg.float_val == 0.5);
    fclose(stream);

    stream = fill_stream(one_arg_guess_wrong);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_GUESS);
    fclose(stream);

    stream = fill_stream(one_arg_guess);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS);
    fclose(stream);

    stream = fill_stream(one_arg_guess);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS);
    fclose(stream);

    stream = fill_stream(one_arg_guess_wrong);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS);
    fclose(stream);
}

static void test_parsing_generate() {
    const char only_generate[] = "generate";
    const char one_arg_generate[] = "generate 1";
    const char one_wrong_arg_generate[] = "generate -1";
    const char two_arg_generate[] = "generate 1 2";
    const char two_wrong_arg_generate[] = "generate 1 x";
    const char two_wrong_arg_generate2[] = "generate x 1";
    FILE* stream;
    command_t cmd;

    stream = fill_stream(only_generate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(one_arg_generate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(one_wrong_arg_generate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(only_generate);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(one_arg_generate);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(one_wrong_arg_generate);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(two_arg_generate);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(two_arg_generate);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(two_wrong_arg_generate);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(two_wrong_arg_generate2);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(two_arg_generate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_GENERATE);
    assert(cmd.arg.two_int_val.i == 1);
    assert(cmd.arg.two_int_val.j == 2);
    fclose(stream);

    stream = fill_stream(two_wrong_arg_generate);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);

    stream = fill_stream(two_wrong_arg_generate2);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_GENERATE);
    fclose(stream);
}

static void test_parsing_undo() {
    const char undo[] = "undo";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(undo);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_UNDO);
    fclose(stream);

    stream = fill_stream(undo);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_UNDO);
    fclose(stream);

    stream = fill_stream(undo);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_UNDO);
    fclose(stream);
}

static void test_parsing_redo() {
    const char redo[] = "redo";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(redo);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_REDO);
    fclose(stream);

    stream = fill_stream(redo);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_REDO);
    fclose(stream);

    stream = fill_stream(redo);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_REDO);
    fclose(stream);
}

static void test_parsing_save() {
    const char save[] = "save";
    const char save_arg[] = "save hi";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(save);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_SAVE);
    fclose(stream);

    stream = fill_stream(save);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SAVE);
    fclose(stream);

    stream = fill_stream(save);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_SAVE);
    fclose(stream);

    stream = fill_stream(save_arg);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_SAVE);
    fclose(stream);

    stream = fill_stream(save_arg);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_SAVE);
    assert(!strcmp(cmd.arg.str_val, "hi"));
    fclose(stream);

    stream = fill_stream(save_arg);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_SAVE);
    assert(!strcmp(cmd.arg.str_val, "hi"));
    fclose(stream);
}

static void test_parsing_hint() {
    const char hint[] = "hint";
    const char hint_arg_wrong[] = "hint x";
    const char hint_arg[] = "hint 1";
    const char hint_two_arg_wrong[] = "hint 1 x";
    const char hint_two_arg[] = "hint 1 2";
    FILE *stream;
    command_t cmd;

    stream = fill_stream(hint);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_arg_wrong);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_arg_wrong);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_arg);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_arg);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_two_arg_wrong);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_two_arg_wrong);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_two_arg);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_HINT);
    fclose(stream);

    stream = fill_stream(hint_two_arg);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_HINT);
    assert(cmd.arg.two_int_val.i == 1);
    assert(cmd.arg.two_int_val.j == 2);
    fclose(stream);
}

static void test_parsing_guess_hint() {
    const char guess_hint[] = "guess_hint";
    const char guess_hint_arg_wrong[] = "guess_hint x";
    const char guess_hint_arg[] = "guess_hint 1";
    const char guess_hint_two_arg_wrong[] = "guess_hint 1 x";
    const char guess_hint_two_arg[] = "guess_hint 1 2";
    FILE *stream;
    command_t cmd;

    stream = fill_stream(guess_hint);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_arg_wrong);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_arg_wrong);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_arg);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_arg);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_NUM_OF_ARGS);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_two_arg_wrong);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_two_arg_wrong);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_INVALID_ARGUMENTS);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_two_arg);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_GUESS_HINT);
    fclose(stream);

    stream = fill_stream(guess_hint_two_arg);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_GUESS_HINT);
    assert(cmd.arg.two_int_val.i == 1);
    assert(cmd.arg.two_int_val.j == 2);
    fclose(stream);
}

static void test_parsing_num_solutions() {
    const char num_solutions[] = "num_solutions";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(num_solutions);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_NUM_SOLUTIONS);
    fclose(stream);

    stream = fill_stream(num_solutions);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_NUM_SOLUTIONS);
    fclose(stream);

    stream = fill_stream(num_solutions);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_NUM_SOLUTIONS);
    fclose(stream);
}

static void test_parsing_autofill() {
    const char autofill[] = "autofill";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(autofill);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_AUTOFILL);
    fclose(stream);

    stream = fill_stream(autofill);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_INVALID_MODE);
    assert(cmd.type == CT_AUTOFILL);
    fclose(stream);

    stream = fill_stream(autofill);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_AUTOFILL);
    fclose(stream);
}

static void test_parsing_reset() {
    const char reset[] = "reset";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(reset);
    assert(parse_line(stream, &cmd, GM_INIT) == P_INVALID_MODE);
    assert(cmd.type == CT_RESET);
    fclose(stream);

    stream = fill_stream(reset);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_RESET);
    fclose(stream);

    stream = fill_stream(reset);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_RESET);
    fclose(stream);
}

static void test_parsing_exit() {
    const char exit[] = "exit";
    FILE *stream; 
    command_t cmd;

    stream = fill_stream(exit);
    assert(parse_line(stream, &cmd, GM_INIT) == P_SUCCESS);
    assert(cmd.type == CT_EXIT);
    fclose(stream);

    stream = fill_stream(exit);
    assert(parse_line(stream, &cmd, GM_EDIT) == P_SUCCESS);
    assert(cmd.type == CT_EXIT);
    fclose(stream);

    stream = fill_stream(exit);
    assert(parse_line(stream, &cmd, GM_SOLVE) == P_SUCCESS);
    assert(cmd.type == CT_EXIT);
    fclose(stream);
}

int main() {
    test_parse_line_too_long();
    test_ignore_blank_line();
    test_eof_exit();
    test_too_many_args();
    test_parsing_solve();
    test_parsing_edit();
    test_parsing_mark_errors();
    test_parsing_print_board();
    test_parsing_set();
    test_parsing_validate();
    test_parsing_guess();
    test_parsing_generate();
    test_parsing_undo();
    test_parsing_redo();
    test_parsing_save();
    test_parsing_hint();
    test_parsing_guess_hint();
    test_parsing_num_solutions();
    test_parsing_autofill();
    test_parsing_reset();
    test_parsing_exit();
    return 0;
}
