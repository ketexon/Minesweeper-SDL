#include "State.h"
#include "Constants.h"

float aspectRatio = 1.0f;

YGSize SquareMeasureFunc(
	YGNodeRef node,
	float w, YGMeasureMode widthMode,
	float h, YGMeasureMode heightMode
) {
	if(aspectRatio * h > w) return (YGSize) { w, w / aspectRatio };
	else return (YGSize) { h * aspectRatio, h };
}

void State_InitLayout(State* state) {
	state->yoga.root = YGNodeNew();
	YGNodeRef root = state->yoga.root;
	YGNodeStyleSetPadding(root, YGEdgeAll, 16);

	YGNodeStyleSetDisplay(root, YGDisplayFlex);
	YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
	YGNodeStyleSetJustifyContent(root, YGJustifyCenter);
	YGNodeStyleSetAlignItems(root, YGAlignCenter);

	state->yoga.minesweeper = YGNodeNew();
	YGNodeRef minesweeperNode = state->yoga.minesweeper;
	YGNodeInsertChild(root, minesweeperNode, 0);

	YGNodeSetMeasureFunc(minesweeperNode, SquareMeasureFunc);

	State_InitBoard(state);

	State_RecalculateLayout(state, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void State_RecalculateBoardLayout(State* state) {
	float boardLeft = YGNodeLayoutGetLeft(state->yoga.minesweeper);
	float boardTop = YGNodeLayoutGetTop(state->yoga.minesweeper);
	float boardWidth = YGNodeLayoutGetWidth(state->yoga.minesweeper);
	float boardHeight = YGNodeLayoutGetHeight(state->yoga.minesweeper);

	float tileWidth = boardWidth / state->board.width;
	float tileHeight = boardHeight / state->board.height;

	for(size_t x = 0; x < state->board.width; ++x){
		for(size_t y = 0; y < state->board.height; ++y){
			size_t i = x + state->board.width * y;

			SDL_FRect* rect = &state->board.rects[i];
			rect->w = tileWidth;
			rect->h = tileHeight;

			rect->x = x * tileWidth + boardLeft;
			rect->y = y * tileWidth + boardTop;
		}
	}
}

void State_RecalculateLayout(State* state, int windowWidth, int windowHeight){
	YGNodeStyleSetWidth(state->yoga.root, (float) windowWidth);
	YGNodeStyleSetHeight(state->yoga.root, (float) windowHeight);
	YGNodeCalculateLayout(state->yoga.root, YGUndefined, YGUndefined, YGDirectionLTR);

	State_RecalculateBoardLayout(state);
}