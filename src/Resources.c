#include "State.h"
#include "Resources.h"
#include "Constants.h"

#include <SDL.h>
#include <SDL_image.h>

SDL_Rect* LoadRectResource(const wchar_t* name){
	HRSRC src = FindResourceW(NULL, name, RC_TYPE_RECT);
	HGLOBAL data = LoadResource(NULL, src);
	void* mem = LockResource(data);
	return (SDL_Rect*) mem;
}


typedef struct Color {
	WORD r, g, b, a;
} Color;

Color* LoadColorResource(const wchar_t* name){
	HRSRC src = FindResourceW(NULL, name, RC_TYPE_COLOR);
	HGLOBAL data = LoadResource(NULL, src);
	void* mem = LockResource(data);
	return (Color*) mem;
}

SDL_Texture* State_LoadTextureFromResource(State* state, LPCWSTR name, const char* filetye){
	HRSRC tilesheetSrc = FindResourceW(NULL, name, RC_TYPE_IMAGE);
	HGLOBAL tilesheetData = LoadResource(NULL, tilesheetSrc);
	void* tilesheetMem = LockResource(tilesheetData);
	SDL_RWops* tilesheetRW = SDL_RWFromMem(tilesheetMem, SizeofResource(NULL, tilesheetSrc));

	return IMG_LoadTextureTyped_RW(
		state->sdl.renderer,
		tilesheetRW,
		true,
		filetye
	);
}

SDL_Texture* State_LoadTextureFromPath(State* state, const char* path){
	return IMG_LoadTexture(state->sdl.renderer, path);
}

void State_LoadResources(State* state) {
	state->images.tilesheet.sourceTextures.original = State_LoadTextureFromResource(state, RC_TILESHEET_DEFAULT, "PNG");
	state->images.tilesheet.sourceTextures.cute = State_LoadTextureFromResource(state, RC_TILESHEET_CUTE, "PNG");

	state->images.tilesheet.texture = state->images.tilesheet.sourceTextures.original;

	state->images.tilesheet.normal = *LoadRectResource(RC_TILE_NORMAL);
	state->images.tilesheet.pressed = *LoadRectResource(RC_TILE_PRESSED);
	state->images.tilesheet.flaged = *LoadRectResource(RC_TILE_FLAGGED);
	state->images.tilesheet.mine = *LoadRectResource(RC_TILE_BOMB);
	state->images.tilesheet.mineRed = *LoadRectResource(RC_TILE_BOMB_RED);
	state->images.tilesheet.tileDigit[0] = *LoadRectResource(RC_TILE_1);
	state->images.tilesheet.tileDigit[1] = *LoadRectResource(RC_TILE_2);
	state->images.tilesheet.tileDigit[2] = *LoadRectResource(RC_TILE_3);
	state->images.tilesheet.tileDigit[3] = *LoadRectResource(RC_TILE_4);
	state->images.tilesheet.tileDigit[4] = *LoadRectResource(RC_TILE_5);
	state->images.tilesheet.tileDigit[5] = *LoadRectResource(RC_TILE_6);
	state->images.tilesheet.tileDigit[6] = *LoadRectResource(RC_TILE_7);
	state->images.tilesheet.tileDigit[7] = *LoadRectResource(RC_TILE_8);

	state->images.tilesheet.smiley.normal = *LoadRectResource(RC_SMILEY_DEFAULT);
	state->images.tilesheet.smiley.pressed = *LoadRectResource(RC_SMILEY_PRESSED);
	state->images.tilesheet.smiley.win = *LoadRectResource(RC_SMILEY_WIN);
	state->images.tilesheet.smiley.lose = *LoadRectResource(RC_SMILEY_LOSE);
	state->images.tilesheet.smiley.surprise = *LoadRectResource(RC_SMILEY_SURPRISE);

	state->images.tilesheet.digit[0] = *LoadRectResource(RC_DIGIT_0);
	state->images.tilesheet.digit[1] = *LoadRectResource(RC_DIGIT_1);
	state->images.tilesheet.digit[2] = *LoadRectResource(RC_DIGIT_2);
	state->images.tilesheet.digit[3] = *LoadRectResource(RC_DIGIT_3);
	state->images.tilesheet.digit[4] = *LoadRectResource(RC_DIGIT_4);
	state->images.tilesheet.digit[5] = *LoadRectResource(RC_DIGIT_5);
	state->images.tilesheet.digit[6] = *LoadRectResource(RC_DIGIT_6);
	state->images.tilesheet.digit[7] = *LoadRectResource(RC_DIGIT_7);
	state->images.tilesheet.digit[8] = *LoadRectResource(RC_DIGIT_8);
	state->images.tilesheet.digit[9] = *LoadRectResource(RC_DIGIT_9);
	state->images.tilesheet.digitMinus = *LoadRectResource(RC_DIGIT_MINUS);

	state->images.tilesheet.border.ur = *LoadRectResource(RC_BORDER_UR);
	state->images.tilesheet.border.ul = *LoadRectResource(RC_BORDER_UL);
	state->images.tilesheet.border.dr = *LoadRectResource(RC_BORDER_DR);
	state->images.tilesheet.border.dl = *LoadRectResource(RC_BORDER_DL);

	state->images.tilesheet.border.lr = *LoadRectResource(RC_BORDER_LR);
	state->images.tilesheet.border.ud = *LoadRectResource(RC_BORDER_UD);

	state->images.tilesheet.border.urd = *LoadRectResource(RC_BORDER_URD);
	state->images.tilesheet.border.uld = *LoadRectResource(RC_BORDER_ULD);

	state->images.tilesheet.background = *LoadRectResource(RC_BACKGROUND_RECT);

	// Color* bgColor = LoadColorResource(RC_BACKGROUND_COLOR);
	// state->backgroundColor = (SDL_Color) {
	// 	.r = bgColor->r,
	// 	.g = bgColor->g,
	// 	.b = bgColor->b,
	// 	.a = bgColor->a,
	// };
	State_UpdateBackgroundColor(state);
}

void State_UpdateBackgroundColor(State* state){
	const int w = 10;
	const int h = 10;
	SDL_Texture* target = SDL_CreateTexture(state->sdl.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h);
	SDL_SetRenderTarget(state->sdl.renderer, target);

	SDL_RenderCopy(
		state->sdl.renderer,
		state->images.tilesheet.texture,
		&state->images.tilesheet.background,
		NULL
	);

	SDL_Color* colors = malloc(w * h * sizeof(*colors));
	SDL_RenderReadPixels(state->sdl.renderer, NULL, SDL_PIXELFORMAT_RGBA32, (void*) colors, sizeof(*colors));

	SDL_SetRenderTarget(state->sdl.renderer, NULL);
	SDL_DestroyTexture(target);

	state->backgroundColor = colors[0];
	free(colors);
}