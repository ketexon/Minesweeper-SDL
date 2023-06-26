cmake_minimum_required(VERSION 3.25)

project(Minesweeper LANGUAGES C)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST OFF)
add_subdirectory(extern/SDL)

add_subdirectory(extern/SDL_image)

add_subdirectory(extern/yoga)

add_executable(
	Minesweeper
	src/Main.c
	src/Constants.h
	src/State.h src/State.c

	rc/rc.rc
)

target_link_libraries(
	Minesweeper
	PRIVATE
	SDL2::SDL2
	SDL2::SDL2main
	yogacore
)

set_target_properties(
	Minesweeper
	PROPERTIES
	C_STANDARD 11
)