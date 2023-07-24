#pragma once

#include <stdbool.h>

#define LUA_CREATE_GAME_FUNCTION "create_game"
#define LUA_GENERATE_MINES_FUNCTION "generate_mines"
#define LUA_COUNT_MINES_FUNCTION "count_mines"

#define LUA_CREATE_GAME_FUNCTIONW L"create_game"
#define LUA_GENERATE_MINES_FUNCTIONW L"generate_mines"
#define LUA_COUNT_MINES_FUNCTIONW L"count_mines"

struct State;

bool State_InitLua(struct State*, char* path);
bool State_Lua_GenerateBoard(struct State*, int tileX, int tileY);
void State_DestroyLua(struct State* state);