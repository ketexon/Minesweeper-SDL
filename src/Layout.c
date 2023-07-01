#include "State.h"
#include "Constants.h"

SDL_FRect Node_GetNodeScreenRect(YGNodeRef node){
	YGNodeRef parent = YGNodeGetParent(node);
	SDL_FRect rect = { 0 };
	if(parent){
		SDL_FRect parentRect = Node_GetNodeScreenRect(parent);
		rect.x = parentRect.x;
		rect.y = parentRect.y;
	}

	rect.x += YGNodeLayoutGetLeft(node);
	rect.y += YGNodeLayoutGetTop(node);
	rect.w += YGNodeLayoutGetWidth(node);
	rect.h += YGNodeLayoutGetHeight(node);

	return rect;
}

float aspectRatio = 1.0f;

YGSize AspectRatioMeasureFunc(
	YGNodeRef node,
	float w, YGMeasureMode widthMode,
	float h, YGMeasureMode heightMode
) {
	if(aspectRatio * h > w) return (YGSize) { w, w / aspectRatio };
	else return (YGSize) { h * aspectRatio, h };
}

void State_InitLayout(State* state) {
	YGNodeRef root = YGNodeNew();
	YGNodeStyleSetPadding(root, YGEdgeAll, 16);

	YGNodeStyleSetDisplay(root, YGDisplayFlex);
	YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
	YGNodeStyleSetJustifyContent(root, YGJustifyCenter);
	YGNodeStyleSetAlignItems(root, YGAlignStretch);

	YGNodeRef ui = YGNodeNew();
	YGNodeInsertChild(root, ui, 0);

	YGNodeStyleSetDisplay(ui, YGDisplayFlex);
	YGNodeStyleSetFlexDirection(root, YGFlexDirectionColumn);
	YGNodeStyleSetJustifyContent(root, YGJustifyCenter);
	YGNodeStyleSetAlignItems(root, YGAlignCenter);

	YGNodeStyleSetFlex(ui, 1);

	YGNodeRef menu = YGNodeNew();
	YGNodeInsertChild(ui, menu, 0);

	YGNodeStyleSetPaddingPercent(menu, YGEdgeAll, 1.0f);

	// YGNodeStyleSetFlex(menu, 1);
	YGNodeStyleSetHeightPercent(menu, 10.0f);

	YGNodeStyleSetDisplay(menu, YGDisplayFlex);
	YGNodeStyleSetFlexDirection(menu, YGFlexDirectionRow);
	YGNodeStyleSetAlignItems(menu, YGAlignStretch);

	YGNodeRef minesLeftContainer = YGNodeNew();
	YGNodeRef smileyContainer = YGNodeNew();
	YGNodeRef timeContainer = YGNodeNew();

	YGNodeInsertChild(menu, minesLeftContainer, 0);
	YGNodeInsertChild(menu, smileyContainer, 1);
	YGNodeInsertChild(menu, timeContainer, 2);

	YGNodeStyleSetFlex(minesLeftContainer, 1);
	YGNodeStyleSetFlex(smileyContainer, 1);
	YGNodeStyleSetFlex(timeContainer, 1);

	YGNodeStyleSetAlignItems(minesLeftContainer, YGAlignStretch);
	YGNodeStyleSetAlignItems(smileyContainer, YGAlignStretch);
	YGNodeStyleSetAlignItems(timeContainer, YGAlignStretch);

	YGNodeStyleSetFlexDirection(minesLeftContainer, YGFlexDirectionColumn);
	YGNodeStyleSetFlexDirection(smileyContainer, YGFlexDirectionColumn);
	YGNodeStyleSetFlexDirection(timeContainer, YGFlexDirectionColumn);

	YGNodeStyleSetAlignItems(minesLeftContainer, YGAlignCenter);
	YGNodeStyleSetAlignItems(smileyContainer, YGAlignCenter);
	YGNodeStyleSetAlignItems(timeContainer, YGAlignCenter);

	YGNodeStyleSetPaddingPercent(smileyContainer, YGEdgeAll, 1.25f);

	// each digit sprite is 13 x 23 pixels
	YGNodeRef minesLeft = YGNodeNew();
	YGNodeInsertChild(minesLeftContainer, minesLeft, 0);

	YGNodeStyleSetFlex(minesLeft, 1.0f);
	YGNodeStyleSetAspectRatio(minesLeft, 3 * 13.0f / 23.0f);

	YGNodeRef smiley = YGNodeNew();
	YGNodeInsertChild(smileyContainer, smiley, 0);

	YGNodeStyleSetFlex(smiley, 1.0f);
	YGNodeStyleSetAspectRatio(smiley, 1.0f);

	YGNodeRef time = YGNodeNew();
	YGNodeInsertChild(timeContainer, time, 0);

	YGNodeStyleSetFlex(time, 1.0f);
	YGNodeStyleSetAspectRatio(time, 3 * 13.0f / 23.0f);


	YGNodeRef boardNode = YGNodeNew();
	YGNodeInsertChild(ui, boardNode, 1);

	YGNodeStyleSetFlex(boardNode, 1);

	YGNodeSetMeasureFunc(boardNode, AspectRatioMeasureFunc);

	state->layout.root = root;
	state->layout.ui = ui;
	state->layout.board = boardNode;

	state->layout.minesLeft = minesLeft;
	state->layout.smiley = smiley;
	state->layout.time = time;

	State_InitBoard(state);

	State_RecalculateLayout(state, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void State_RecalculateBoardLayout(State* state) {
	SDL_FRect boardRect = Node_GetNodeScreenRect(state->layout.board);

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

void State_RecalculateLayout(State* state, int windowWidth, int windowHeight){
	YGNodeStyleSetWidth(state->layout.root, (float) windowWidth);
	YGNodeStyleSetHeight(state->layout.root, (float) windowHeight);
	YGNodeCalculateLayout(state->layout.root, YGUndefined, YGUndefined, YGDirectionLTR);

	State_RecalculateBoardLayout(state);
}