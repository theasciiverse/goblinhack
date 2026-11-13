/*
 * Asciiverse terminal edition: the hooks the game calls (only when built
 * with GHACK_TERMINAL), in asciiverse/platform/render.cpp and terminal.cpp.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#ifndef GHACK_RENDER_H
#define GHACK_RENDER_H

#include <string>

class gh_level;
class gh_thing;

void        render_capture_term(void);                    /* gh_term_refresh */
void        render_capture_map(gh_level *, gh_thing *,    /* display_map */
                               const float *light_radii, int light_rays);
void        render_set_layout(int cols, int rows);        /* console size */
void        render_opaque(int x0, int y0, int x1, int y1); /* gh_term_text_box */
void        render_toggle_zoom(void);                     /* Tab */
int         terminal_cols(void);
int         terminal_rows(void);
std::string terminal_exe_dir(void);
std::string terminal_state_dir(void);
bool        terminal_silent(void);

#endif
