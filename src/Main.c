#include <SDL.h>

#include <stdbool.h>

#include <yoga/Yoga.h>

#include "State.h"

int main(void){
	State state;
	State* statePtr = &state;
	if(!State_Init(statePtr)){
		State_Destroy(statePtr);
		return 1;
	}

	bool shouldQuit = false;
	while(!shouldQuit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_EVENT_QUIT) {
				shouldQuit = 1;
				break;
			}
			else{
				State_HandleEvent(statePtr, &event);
			}
		}

		State_Update(statePtr);
	}

	State_Destroy(statePtr);

	return 0;
}