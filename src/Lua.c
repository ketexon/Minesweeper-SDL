#include "Lua.h"

#include "State.h"
#include "Win.h"

#include <stdbool.h>

#include <Lua.h>
#include <lualib.h>
#include <lauxlib.h>

static const struct luaL_Reg globalFunctions[] = {
	{NULL, NULL}
};

bool State_InitLua(State* state, char* path) {
	State_DestroyLua(state);
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	lua_getglobal(L, "_G");
	luaL_setfuncs(L, globalFunctions, 0);
	lua_pop(L, 1);

	state->game.lua.state = L;
	state->game.lua.createGameRef = LUA_NOREF;
	state->game.lua.generateMinesRef = LUA_NOREF;
	state->game.lua.countMinesRef = LUA_NOREF;

	bool initSuccessfully = true;

	bool error;
	error = luaL_loadfile(L, path);
	if(error){
		printf("Could not load file: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		initSuccessfully = false;
	}
	else{
		error = lua_pcall(L, 0, 1, 0);
		if(error){
			printf("Could not run file: %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		else{
			if(lua_istable(L, 1)){
				int type;
				type = lua_getfield(L, -1, LUA_CREATE_GAME_FUNCTION);
				if(type == LUA_TFUNCTION){
					state->game.lua.createGameRef = luaL_ref(L,  LUA_REGISTRYINDEX);
				}
				else{
					lua_pop(L, 1);
				}

				type = lua_getfield(L, -1, LUA_GENERATE_MINES_FUNCTION);
				if(type == LUA_TFUNCTION){
					state->game.lua.generateMinesRef = luaL_ref(L,  LUA_REGISTRYINDEX);
				}
				else{
					lua_pop(L, 1);
				}

				type = lua_getfield(L, -1, LUA_COUNT_MINES_FUNCTION);
				if(type == LUA_TFUNCTION){
					state->game.lua.countMinesRef = luaL_ref(L,  LUA_REGISTRYINDEX);
				}
				else{
					lua_pop(L, 1);
				}

				if(
					state->game.lua.createGameRef == LUA_NOREF
					&& state->game.lua.generateMinesRef == LUA_NOREF
					&& state->game.lua.countMinesRef == LUA_NOREF
				){
					MessageBoxW(
						NULL,
						L"Lua file does not return a table with one or more of: "
						L"\"" LUA_CREATE_GAME_FUNCTIONW L"\", "
						L"\"" LUA_GENERATE_MINES_FUNCTIONW L"\", "
						L"\"" LUA_COUNT_MINES_FUNCTIONW L"\".",
						L"Lua import error",
						MB_OK | MB_ICONEXCLAMATION
					);
					initSuccessfully = false;
				}
			}
			else{
				MessageBoxW(NULL, L"Lua file does not return a table.", L"Lua import error", MB_OK | MB_ICONEXCLAMATION);
				initSuccessfully = false;
			}
		}
	}

	if(!initSuccessfully) lua_close(L);
	return initSuccessfully;
}

void State_Lua_MessageBoxError(State* state, const wchar_t* functionName){
	lua_State* L = state->game.lua.state;

	const char* msg = lua_tostring(L, -1);
	int wMsgLen = MultiByteToWideChar(
		CP_UTF8, 0,
		msg, -1,
		NULL, 0
	);

	wchar_t* wMsg = malloc(wMsgLen * sizeof(*wMsg));

	MultiByteToWideChar(
		CP_UTF8, 0,
		msg, -1,
		wMsg, wMsgLen
	);

	const wchar_t messageBoxTextPrefix[] = L"Error in function";
	size_t messageBoxTextPrefixLen = sizeof(messageBoxTextPrefix)/sizeof(*messageBoxTextPrefix) - 1;
	// size_t messageBoxTextLen =
	// 	messageBoxTextPrefixLen
	// 	+ 2 // space quote
	// 	+ wcslen(functionName)
	// 	+ 1 // quote
	// 	+ 2 // : space
	// 	+ (wMsgLen - 1);

	size_t messageBoxTextSize =
		messageBoxTextPrefixLen
		+ 1 // space
		+ 1 // quote
		+ wcslen(functionName)
		+ 1 // quote
		+ 2 // : space
		+ wcslen(wMsg)
		+ 1; // null
	wchar_t* messageBoxText = malloc(messageBoxTextSize * sizeof(*messageBoxText));

	// swprintf_s(
	// 	messageBoxText, messageBoxTextLen + 1,
	// 	L"%s \"%s\": %s",
	// 	messageBoxTextPrefix,
	// 	functionName,
	// 	wMsg
	// );

	swprintf_s(
		messageBoxText, messageBoxTextSize,
		L"%s \"%s\": %s", messageBoxTextPrefix, functionName, wMsg
	);

	// MessageBoxW(NULL, messageBoxText, L"Lua Error", MB_OK | MB_ICONEXCLAMATION);
	MessageBoxW(NULL, messageBoxText, L"Lua Error", MB_OK | MB_ICONEXCLAMATION);

	free(messageBoxText);
	free(wMsg);
}

bool State_Lua_GenerateBoard(State* state, int tileX, int tileY) {
	lua_State* L = state->game.lua.state;

	int type = lua_rawgeti(L, LUA_REGISTRYINDEX, state->game.lua.createGameRef);
	if(type == LUA_TFUNCTION){
		lua_pushinteger(L, state->board.width);
		lua_pushinteger(L, state->board.height);
		lua_pushinteger(L, state->board.nMines);
		lua_pushinteger(L, tileX);
		lua_pushinteger(L, tileY);
		int error = lua_pcall(L, 5, 1, 0);
		if(error){
			State_Lua_MessageBoxError(state, LUA_CREATE_GAME_FUNCTIONW);
			lua_pop(L, 1);
			return false;
		}
		else{
			lua_pop(L, 1);
		}
	}
	else{
		State_CreateGameDefault(state, tileX, tileY);
	}

	return true;
}

void State_DestroyLua(State* state){
	lua_State* L = state->game.lua.state;

	if(state->game.lua.state != NULL) {
		// free refs
		luaL_unref(L, LUA_REGISTRYINDEX, state->game.lua.createGameRef);
		luaL_unref(L, LUA_REGISTRYINDEX, state->game.lua.generateMinesRef);
		luaL_unref(L, LUA_REGISTRYINDEX, state->game.lua.countMinesRef);

		lua_close(L);
	}
}