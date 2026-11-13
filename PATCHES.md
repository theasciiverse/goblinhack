# Asciiverse changes to Goblin Hack

Goblin Hack is Neil McGill's (goblinhack) "2d smooth scrolling ascii
dungeon romp", first written in 1999, here from upstream
github.com/goblinhack/goblinhack-game at `c3c1567` (2021-09-26), version
1.20. Upstream is the `upstream` remote; our changes are on the
`asciiverse` branch. The upstream autoconf build (`RUNME`) is untouched and
still builds the windowed game.

## The terminal edition

`asciiverse/Makefile` builds the game for a terminal: no window, no OpenGL,
no video driver. Everything new lives in `asciiverse/platform/`; the game's
own sources change only in small hooks, all under `GHACK_TERMINAL`.

- **No window, no OpenGL** (`asciiverse/platform/include`): the part of the
  SDL 1.2 API the game uses, with no video (`sdl.cpp`), and its OpenGL calls
  as empty inline functions (`ghack_gl_shim.h`). Surfaces are plain memory.
  The end of each frame the game draws (`SDL_GL_SwapBuffers`) puts the frame
  on the terminal.
- **The picture** (`render.cpp`): the game's console grid (menus, score,
  messages) as the game draws it, captured in `gh_term_refresh`
  (`term/console.cpp`), over the dungeon, one terminal cell a map square,
  centred on the hero (`map/gfx.cpp` hands over the map and the hero's light
  at the end of `display_map`). What shows follows `display_map`'s own rules:
  things in sight this frame, lit rooms, remembered walls, doors, water and
  exits. The floor is lit by the hero's own light rays, as the windowed game's
  light polygon does, and remembered floor stays, dim.
- **One character a thing**: by kind (the folder it comes from: `monst/orc`
  is `o`, `treasure/gems` is `*`, `magic/potion` is `!`, walls `#`, doors
  `+`, exits `>`), coloured by the strongest colour in its own picture.
- **Tab: the pictures.** The zoomed view draws every thing as its own
  picture of characters, as the windowed game does, one character a cell.
- **Keys and mouse from the terminal** (`terminal.cpp`, after the Zorbash
  terminal edition's): the kitty keyboard protocol where the terminal has
  it, with real key releases, else plain bytes with a short hold, so a tap of
  a direction walks about one square and a held key walks on. SGR mouse.
  Ctrl-C quits.
- **The console is the terminal**: as wide and tall as it is, up to 200 by
  100 (`term/term.cpp`, `gh_term.h`), following a resize.
- **Short consoles**: the game was laid out for 46 rows. Below that, chooser
  lists show as many entries as fit and scroll (`gh_term_list_rows`), the
  intro's keys go one to a row with the verse after them, and the high
  scores one to a row (`ui/intro.cpp`, `ui/hiscore.cpp`). On 46 rows or more
  nothing changes.
- **Boxes cover the map**: the help and quit boxes hide what is under them
  (`term/term.cpp`, `gh_term_text_box`).
- **Files**: the game's working folder is the player's state folder
  (`$XDG_STATE_HOME/goblinhack`, else `~/.local/state/goblinhack`, on macOS
  `~/Library/Application Support/goblinhack`), so saves, high scores, bones,
  config and the game's own `stdout.txt` and `stderr.txt` land there. The
  game's files are found beside the binary (`resources/`), in the source
  tree, or in `/usr/share/goblinhack` (`main/global.cpp`).
- **No self-updater**: the terminal build never runs `gh_main_installer`,
  which downloads the next version from SourceForge with curl
  (`main/main.cpp`).
- **Sound** through SDL2_mixer (`audio.cpp`, the only file that sees the
  real SDL2 headers); `--silent` or `GOBLINHACK_SILENT=1` opens no audio
  device. `make -f asciiverse/Makefile SOUND=0` builds with no SDL at all.
- **Session log**: `GOBLINHACK_LOG=1` writes `goblinhack-terminal.log` in
  the state folder: every key, resizes, and a snapshot (level, hero square)
  once a second.

## Packaging

- `asciiverse/build.sh`: SDL2 2.32.10 (no video) and SDL2_mixer 2.8.1 (WAV
  only) built from source and linked statically, then the terminal build;
  glibc Linux in ubuntu:22.04, macOS on the runner.
- `asciiverse/stage.sh`: the av archive, the binary with `resources/`
  beside it.
- `asciiverse/nfpm.yaml`: .deb, .rpm and Arch packages, the binary in
  `/usr/bin`, the game's files in `/usr/share/goblinhack/resources`.
- `.github/workflows/release.yml`: Linux x86-64 and ARM64, macOS x86-64 and
  ARM64. No Windows: the terminal code reads the terminal through POSIX
  (termios, poll).
- `asciiverse/record-preview.py`: the catalog preview.
