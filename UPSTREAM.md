# Goblin Hack in a terminal: notes for Neil

Hi Neil. This is a terminal edition of Goblin Hack, made for Asciiverse
(asciiverse.sh), a catalog and launcher for terminal games, where Zorbash's
terminal edition already is. It plays the whole game in any terminal, no
window and no OpenGL, and it is yours to take in any part, or not.

## How it is built

A second makefile, `asciiverse/Makefile`, next to RUNME, which is
unchanged. It compiles the game against `asciiverse/platform/`:

- `include/`: headers named like SDL 1.2's and OpenGL's. SDL is a small
  stand-in with no video (events, ticks, memory surfaces); every OpenGL call
  is an empty inline function. Nothing links SDL's video or libGL.
- `sdl.cpp`: that stand-in. `SDL_GL_SwapBuffers` is where a frame goes out.
- `terminal.cpp`: raw mode, keys (the kitty keyboard protocol for real
  releases where the terminal has it, else a short hold per press), SGR
  mouse, resize, and diffed 24-bit colour output.
- `render.cpp`: the frame. Your console grid on top, the dungeon under it.
- `audio.cpp`: your SDL_mixer calls, sent to SDL2_mixer.

## The hooks in your code

All under `#ifdef GHACK_TERMINAL`, so the windowed build is exactly as before:

- `term/console.cpp`, `gh_term_refresh`: hands the console grid over before
  it is cleared.
- `map/gfx.cpp`, end of `display_map`: hands over the level, the hero and
  the light radii from `make_light_map`. The terminal map uses your own
  tests for what may be drawn (rendered this frame, lit room, has been seen
  and boring/water/door/exit), plus the light radii to light the floor, as
  your light polygon does.
- `term/term.cpp`, `gh_term_init`: the console is the terminal's size.
  `gh_term_text_box` marks its box as covering the map.
- `include/gh_term.h`: `GH_TERM_MAX_SIZE` 200 in a terminal.
- `main/global.cpp`: the binary's own folder in the search path.
- `main/main.cpp`: no `gh_main_installer` (the package manager or launcher
  does updates).

Three changes are not ifdef'd, because they help any console under 46
rows and change nothing at 46 or more:

- `gh_term_list_rows()` in `term/term.cpp`, used by the six chooser screens
  for their list length, so a list fits between banner and prompt and
  scrolls.
- `ui/intro.cpp`: the keys one to a row and the verse after them.
- `ui/hiscore.cpp`: one row a score under the banner.

## Things I noticed

- `things/ref.cpp` logs "thing milk ref does not exist carried-item" when a
  carried food item is used (`use_simple_item` then `dead`); the game carries
  on. I have not checked the windowed build; not changed here.
- `read_events` in `sdl/screen.cpp`: the `continue` for `SDLK_LALT` and
  `SDLK_RALT` skips the `e++`, so the same event is read again on the next
  pass. Not changed here.

Thank you for Goblin Hack. The little ASCII pictures, every monster with its
own face, are a delight; the Tab view shows them as you drew them.

David (Asciiverse)
