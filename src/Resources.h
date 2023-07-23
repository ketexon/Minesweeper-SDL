#pragma once

struct State;

SDL_Texture* State_LoadTextureFromPath(State* state, const char* path);

void State_LoadResources(struct State*);
void State_UpdateBackgroundColor(struct State*);