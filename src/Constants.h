#pragma once

#define WINDOW_TITLE "Minesweeper"

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 500

#define BOARD_WIDTH_EASY 10
#define BOARD_HEIGHT_EASY 10

#define BOARD_WIDTH_MEDIUM 25
#define BOARD_HEIGHT_MEDIUM 25

#define BOARD_WIDTH_HARD 50
#define BOARD_HEIGHT_HARD 50

#define BOARD_N_MINES_EASY 10
#define BOARD_N_MINES_MEDIUM 100
#define BOARD_N_MINES_HARD 250

#define BOARD_CLICK_SAFE_AREA 3

#define RC_TYPE_IMAGE L"KET_IMAGE"
#define RC_TYPE_RECT L"KET_RECT"

#define RC_TILESHEET L"TILESHEET"

#define RC_TILE_NORMAL L"TILE_NORMAL"
#define RC_TILE_PRESSED L"TILE_PRESSED"
#define RC_TILE_FLAGGED L"TILE_FLAGGED"
#define RC_TILE_BOMB L"TILE_BOMB"
#define RC_TILE_BOMB_RED L"TILE_BOMB_RED"
#define RC_TILE_BOMB_RED L"TILE_BOMB_RED"

#define RC_TILE_1 L"TILE_1"
#define RC_TILE_2 L"TILE_2"
#define RC_TILE_3 L"TILE_3"
#define RC_TILE_4 L"TILE_4"
#define RC_TILE_5 L"TILE_5"
#define RC_TILE_6 L"TILE_6"
#define RC_TILE_7 L"TILE_7"
#define RC_TILE_8 L"TILE_8"

#define RC_SMILEY_DEFAULT L"SMILEY_DEFAULT"
#define RC_SMILEY_PRESSED L"SMILEY_PRESSED"
#define RC_SMILEY_WIN L"SMILEY_WIN"
#define RC_SMILEY_LOSE L"SMILEY_LOSE"

#define RC_DIGIT_0 L"DIGIT_0"
#define RC_DIGIT_1 L"DIGIT_1"
#define RC_DIGIT_2 L"DIGIT_2"
#define RC_DIGIT_3 L"DIGIT_3"
#define RC_DIGIT_4 L"DIGIT_4"
#define RC_DIGIT_5 L"DIGIT_5"
#define RC_DIGIT_6 L"DIGIT_6"
#define RC_DIGIT_7 L"DIGIT_7"
#define RC_DIGIT_8 L"DIGIT_8"
#define RC_DIGIT_9 L"DIGIT_9"
#define RC_DIGIT_MINUS L"DIGIT_MINUS"

#define RC_BORDER_DR L"BORDER_DR"
#define RC_BORDER_DL L"BORDER_DL"
#define RC_BORDER_UR L"BORDER_UR"
#define RC_BORDER_UL L"BORDER_UL"

#define RC_BORDER_LR L"BORDER_LR"
#define RC_BORDER_UD L"BORDER_UD"

#define RC_BORDER_ULD L"BORDER_ULD"
#define RC_BORDER_URD L"BORDER_URD"

#define RC_MENU L"KET_MENU"

#define IDM_EXIT 200

#define IDM_EASY 210
#define IDM_MEDIUM 211
#define IDM_HARD 212

#define KET_MIN(a, b) ((a < b) ? (a) : (b))
#define KET_MAX(a, b) ((a > b) ? (a) : (b))