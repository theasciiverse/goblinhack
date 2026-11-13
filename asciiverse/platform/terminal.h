/*
 * Asciiverse terminal edition of Goblin Hack: the terminal itself.
 *
 * terminal.cpp owns the terminal (raw mode, alternate screen, keys, mouse,
 * output); render.cpp fills a frame of cells from the game each time the
 * game finishes drawing one (SDL_GL_SwapBuffers).
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#ifndef GHACK_TERMINAL_H
#define GHACK_TERMINAL_H

#include <stdint.h>
#include <string>

struct term_rgb {
    uint8_t r, g, b;
    bool operator==(const term_rgb &o) const { return r == o.r && g == o.g && b == o.b; }
};

struct term_cell {
    uint32_t ch;
    term_rgb fg;
    term_rgb bg;
};

/* largest frame we draw; bigger terminals get the frame centred */
enum { TERM_MAX_W = 200, TERM_MAX_H = 100 };

/* the frame render.cpp fills, terminal_frame() writes */
extern term_cell term_frame[TERM_MAX_H][TERM_MAX_W];

bool        terminal_init(void);        /* before main: own the tty */
void        terminal_start(void);       /* raw mode, alternate screen */
void        terminal_fini(void);        /* restore, from anywhere */
void        terminal_poll(void);        /* read keys and mouse into SDL events */
void        terminal_wait(int ms);      /* sleep until input or timeout */
void        terminal_frame(void);       /* compose (render.cpp) and write */
int         terminal_cols(void);
int         terminal_rows(void);
std::string terminal_state_dir(void);   /* ~/.local/state/goblinhack */
std::string terminal_exe_dir(void);     /* the folder the binary is in */
void        terminal_log(const std::string &line);

/* render.cpp */
void        render_compose(int cols, int rows);

#endif
