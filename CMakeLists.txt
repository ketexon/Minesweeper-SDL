cmake_minimum_required(VERSION 3.25)

project(Minesweeper LANGUAGES C)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST OFF)
add_subdirectory(extern/SDL)

set(BUILD_SHARED_LIBS OFF)
set(SDL2IMAGE_SAMPLES OFF)
set(SDL2IMAGE_TESTS OFF)
add_subdirectory(extern/SDL_image)

add_subdirectory(extern/Lua/lua-5.4.6)

set(
	MinesweeperSrc
	src/Main.c
	src/Constants.h
	src/State.h src/State.c

	src/Layout.c

	src/Win.h
	src/Resources.h src/Resources.c

	src/Solver.h src/Solver.c

	src/Matrix.h src/Matrix.c

	src/Menu.c

	rc/rc.rc
)

add_executable(
	Minesweeper WIN32
	${MinesweeperSrc}
)


target_link_libraries(
	Minesweeper
	PRIVATE
	SDL2::SDL2
	SDL2::SDL2main
	SDL2_image
	Shlwapi.lib
)

set_target_properties(
	Minesweeper
	PROPERTIES
	C_STANDARD 17
)

target_compile_definitions(
	Minesweeper
	PUBLIC
	$<$<CONFIG:Debug>:KET_DEBUG>
)