# Minesweeper SDL

![Minesweeper_p5ShM2R5kk](https://github.com/ketexon/Minesweeper-SDL/assets/29184562/2a79d476-d679-438e-b70f-c9d9a31625e0)

Minesweeper written in SDL.

## Setup

Supports Windows only (uses RC files and WinAPI for menus and dialogs).

Tested with Visual Studio 2022 AMD64 generator on Windows 11.

Make sure to initialize git submodules:

```
git submodule update --init --recursive
```

Then, you need to run CMake. I recommend using VSCode's CMake extension to do all the heavy lifting.

Release mode does not include a command prompt, but debug mode does.

## Custom Game Modes

Custom game modes are defined via Lua scripts (note: be careful what scripts you run!).

The script is directy parsed and must return a table of the following schema:

```lua
{
	---@type fun(width: integer, height: integer, n_mines: integer, tile_clicked_x: integer, tile_clicked_y: integer): integer[][]
	create_game = function(
		width: integer,
		height: integer,
		n_mines: integer,
		tile_clicked_x: integer,
		tile_clicked_y: integer
	): [[is_mine: boolean, ]] | nil,
	create_game: function(width, height, n_mines, tile_clicked_x, tile_clicked_y) | nil,
	create_game: function(width, height, n_mines, tile_clicked_x, tile_clicked_y) | nil,
}
```

If `create_game` is specified, it

## Todo

 - [ ] Add solver to prevent 50/50s
 - [ ] Add support for theming
 - [ ] Add custom rules?

## Credits

Using default spritesheet from [minesweeperonline](https://minesweeperonline.com/#)

Kitty cat cute twink boyfriend by @JunaxyArt