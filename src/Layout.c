#include "State.h"
#include "Constants.h"

#include <stdio.h>

void State_InitLayout(State* state) {
	State_RecalculateLayout(state, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void State_RecalculateBoardLayout(State* state) {
	SDL_FRect boardRect = state->layoutv2.board;

	float tileWidth = boardRect.w / state->board.width;
	float tileHeight = boardRect.h / state->board.height;

	for(size_t x = 0; x < state->board.width; ++x){
		for(size_t y = 0; y < state->board.height; ++y){
			size_t i = x + state->board.width * y;

			SDL_FRect* rect = &state->board.rects[i];
			rect->w = tileWidth;
			rect->h = tileHeight;

			rect->x = x * tileWidth + boardRect.x;
			rect->y = y * tileWidth + boardRect.y;
		}
	}
}

void State_RecalculateLayoutV2(State* state, int windowWidthPx, int windowHeightPx){
	const float windowContentPadding = 0.025f;

	const int borderThicknessPx = 10;
	const int uiPaddingPx = 4;
	const int tileLengthPx = 16;
	const int smileyHeightPx = 24;
	const int digitsWidthPx = 13 * 3;

	int boardWidthPx = state->board.width * tileLengthPx;

	int boardHeightPx = state->board.height * tileLengthPx;
	int uiHeightPx = smileyHeightPx + 2 * uiPaddingPx;

	int gameWidthPx = boardWidthPx + 2 * borderThicknessPx;
	int gameHeightPx = boardHeightPx + uiHeightPx + 3 * borderThicknessPx;

	float gameAspectRatio = (float)gameWidthPx / gameHeightPx;

	float windowContentPaddingPx = KET_MIN(windowContentPadding * windowWidthPx, windowContentPadding * windowHeightPx);

	float windowContentWidth = windowWidthPx - 2 * windowContentPaddingPx;
	float windowContentHeight = windowHeightPx - 2 * windowContentPaddingPx;

	state->layoutv2.content = (SDL_FRect) {
		.x = windowContentPaddingPx,
		.y = windowContentPaddingPx,
		.w = windowContentWidth,
		.h = windowContentHeight,
	};

	float windowContentAspectRatio = windowContentWidth / windowContentHeight;

	float gameWidthPxScaled;
	float gameHeightPxScaled;

	// true: game width is too big -> width fills
	if(gameAspectRatio > windowContentAspectRatio){
		gameWidthPxScaled = windowContentWidth;
		gameHeightPxScaled = gameWidthPxScaled / gameAspectRatio;
	}
	// false: game height too big -> height fills
	else{
		gameHeightPxScaled = windowContentHeight;
		gameWidthPxScaled = gameHeightPxScaled * gameAspectRatio;
	}

	printf("%d %d\n", gameWidthPx, gameHeightPx);

	float scale = gameHeightPxScaled / gameHeightPx;
	float borderThicknessPxScaled = borderThicknessPx * scale;

	float boardWidthPxScaled = boardWidthPx * scale;
	float boardHeightPxScaled = boardHeightPx * scale;

	float uiHeightPxScaled = uiHeightPx * scale;
	float uiPaddingPxScaled = uiPaddingPx * scale;
	float smileyLengthPxScaled = smileyHeightPx * scale;
	float digitsWidthPxScaled = digitsWidthPx * scale;

	SDL_FRect gameRect = (SDL_FRect) {
		.x = (windowWidthPx - gameWidthPxScaled)/2.0f,
		.y = (windowHeightPx - gameHeightPxScaled)/2.0f,
		.w = gameWidthPxScaled,
		.h = gameHeightPxScaled,
	};

	state->layoutv2.game = gameRect;

	state->layoutv2.ui = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled,
		.y = gameRect.y + borderThicknessPxScaled,
		.w = boardWidthPxScaled,
		.h = uiHeightPxScaled
	};

	state->layoutv2.board = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled,
		.y = gameRect.y + 2 * borderThicknessPxScaled + uiHeightPxScaled,
		.w = boardWidthPxScaled,
		.h = boardHeightPxScaled
	};

	// UI

	state->layoutv2.minesRemaining = (SDL_FRect){
		.x = state->layoutv2.ui.x + uiPaddingPxScaled,
		.y = state->layoutv2.ui.y + uiPaddingPxScaled,
		.w = digitsWidthPxScaled,
		.h = smileyLengthPxScaled
	};

	state->layoutv2.time = (SDL_FRect){
		.x = state->layoutv2.ui.x + state->layoutv2.ui.w - uiPaddingPxScaled - digitsWidthPxScaled,
		.y = state->layoutv2.ui.y + uiPaddingPxScaled,
		.w = digitsWidthPxScaled,
		.h = smileyLengthPxScaled
	};

	state->layoutv2.smiley = (SDL_FRect){
		.x = state->layoutv2.ui.x + (boardWidthPxScaled - smileyLengthPxScaled)/2.0f,
		.y = state->layoutv2.ui.y + uiPaddingPxScaled,
		.w = smileyLengthPxScaled,
		.h = smileyLengthPxScaled
	};


	// BORDERS

	state->layoutv2.border.uiTopLeft = (SDL_FRect) {
		.x = gameRect.x,
		.y = gameRect.y,
		.w = borderThicknessPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.uiTop = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled,
		.y = gameRect.y,
		.w = boardWidthPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.uiTopRight = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled + boardWidthPxScaled,
		.y = gameRect.y,
		.w = borderThicknessPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.uiLeft = (SDL_FRect) {
		.x = gameRect.x,
		.y = gameRect.y + borderThicknessPxScaled,
		.w = borderThicknessPxScaled,
		.h = uiHeightPxScaled
	};

	state->layoutv2.border.uiRight = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled + boardWidthPxScaled,
		.y = gameRect.y + borderThicknessPxScaled,
		.w = borderThicknessPxScaled,
		.h = uiHeightPxScaled
	};

	state->layoutv2.border.uiBottomLeft = (SDL_FRect) {
		.x = gameRect.x,
		.y = gameRect.y + borderThicknessPxScaled + uiHeightPxScaled,
		.w = borderThicknessPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.uiBottom = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled,
		.y = gameRect.y + borderThicknessPxScaled + uiHeightPxScaled,
		.w = boardWidthPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.uiBottomRight = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled + boardWidthPxScaled,
		.y = gameRect.y + borderThicknessPxScaled + uiHeightPxScaled,
		.w = borderThicknessPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.boardLeft = (SDL_FRect) {
		.x = gameRect.x,
		.y = gameRect.y + 2 * borderThicknessPxScaled + uiHeightPxScaled,
		.w = borderThicknessPxScaled,
		.h = boardHeightPxScaled
	};

	state->layoutv2.border.boardRight = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled + boardWidthPxScaled,
		.y = gameRect.y + 2 * borderThicknessPxScaled + uiHeightPxScaled,
		.w = borderThicknessPxScaled,
		.h = boardHeightPxScaled
	};

	state->layoutv2.border.boardBottomLeft = (SDL_FRect) {
		.x = gameRect.x,
		.y = gameRect.y + 2 * borderThicknessPxScaled + uiHeightPxScaled + boardHeightPxScaled,
		.w = borderThicknessPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.boardBottom = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled,
		.y = gameRect.y + 2 * borderThicknessPxScaled + uiHeightPxScaled + boardHeightPxScaled,
		.w = boardWidthPxScaled,
		.h = borderThicknessPxScaled
	};

	state->layoutv2.border.boardBottomRight = (SDL_FRect) {
		.x = gameRect.x + borderThicknessPxScaled + boardWidthPxScaled,
		.y = gameRect.y + 2 * borderThicknessPxScaled + uiHeightPxScaled + boardHeightPxScaled,
		.w = borderThicknessPxScaled,
		.h = borderThicknessPxScaled
	};
}

void State_RecalculateLayout(State* state, int windowWidth, int windowHeight){
	State_RecalculateLayoutV2(state, windowWidth, windowHeight);
	State_RecalculateBoardLayout(state);
}