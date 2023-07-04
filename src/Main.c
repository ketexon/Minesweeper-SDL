#include <SDL.h>

#include <stdbool.h>

#ifdef KET_DEBUG
#include <stdio.h>
#endif

#include "State.h"

#include "Win.h"

int main(int argc, char* argv[]){
#ifdef KET_DEBUG
	if(AllocConsole()){
		freopen("CONOUT$", "w", stdout);
	}
#endif

	State state;
	State* statePtr = &state;
	if(!State_Init(statePtr)){
		State_Destroy(statePtr);
		return 1;
	}

	bool shouldQuit = false;
	while(!shouldQuit && !statePtr->shouldQuit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_QUIT) {
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