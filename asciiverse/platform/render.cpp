/*
 * Asciiverse terminal edition of Goblin Hack: what goes on the terminal.
 *
 * Two things make a frame:
 *
 *  - the game's own console grid (gh_term_cells: menus, score, messages),
 *    captured each time the game draws it (gh_term_refresh), and
 *  - the dungeon, one terminal cell per map square, centred on the hero,
 *    drawn from the game's own map with the game's own rules for what the
 *    hero can see (the same tests display_map() applies).
 *
 * In the windowed game each thing is a little picture made of characters
 * (things/<kind>/<name>.map). Here each thing is one character: a glyph by
 * kind, coloured from its own picture.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#include <ghack.h>
#include <gh_term.h>
#include <gh_console.h>
#include <gh_thing.h>
#include <gh_level.h>
#include <gh_room.h>
#include <gh_player.h>
#include <gh_game.h>

#include "terminal.h"
#include "ghack_render.h"

#include <map>
#include <math.h>
#include <vector>
#include <stdio.h>

/* the console grid as last drawn this frame */
static gh_term_cell render_term[GH_TERM_MAX_SIZE][GH_TERM_MAX_SIZE / 2];
static bool         render_term_set[GH_TERM_MAX_SIZE][GH_TERM_MAX_SIZE / 2];

/* the map as drawn this frame */
struct map_cell {
    uint32_t ch;
    term_rgb fg;
    term_rgb bg;
    bool     set;
};
static map_cell render_map[TERM_MAX_H][TERM_MAX_W];
static bool     render_map_valid;
static int      render_map_w;
static int      render_map_h;

static uint32_t render_snapshot_ms;

static bool render_zoom;
static void render_zoom_map(gh_level *level, gh_thing *hero, const float *radii, int rays);

static term_rgb rgb_of(uint8 color)
{
    const gh_pixel *p = console.color_to_pixel(color);
    term_rgb c = { p->r, p->g, p->b };
    return c;
}

static term_rgb dim(term_rgb c, int pct)
{
    term_rgb d = { (uint8) (c.r * pct / 100), (uint8) (c.g * pct / 100), (uint8) (c.b * pct / 100) };
    return d;
}

static bool is_black(term_rgb c) { return c.r < 16 && c.g < 16 && c.b < 16; }

/* console boxes drawn this frame: blank cells in them still cover the map */
static bool render_opaque_cells[GH_TERM_MAX_SIZE][GH_TERM_MAX_SIZE / 2];

void render_opaque(int x0, int y0, int x1, int y1)
{
    for (int x = std::max(0, x0); x <= x1 && x < GH_TERM_MAX_SIZE; x++) {
        for (int y = std::max(0, y0); y <= y1 && y < GH_TERM_MAX_SIZE / 2; y++) {
            render_opaque_cells[x][y] = true;
        }
    }
}

void render_capture_term(void)
{
    for (int x = 0; x < GH_TERM_WIDTH && x < GH_TERM_MAX_SIZE; x++) {
        for (int y = 0; y < GH_TERM_HEIGHT && y < GH_TERM_MAX_SIZE / 2; y++) {
            const gh_term_cell *c = &gh_term_cells[x][y];
            if (!c->c) {
                continue;
            }
            if (c->c == ' ' && c->bg == GH_TERM_COLOR_BLACK && !render_opaque_cells[x][y]) {
                continue;
            }
            render_term[x][y]     = *c;
            render_term_set[x][y] = true;
        }
    }
}

/*
 * One glyph for a thing: by its kind (the folder it comes from, then its
 * flags), coloured by the strongest colour in its own picture.
 */
struct glyph {
    uint32_t ch;
    term_rgb fg;
    term_rgb bg;
};

static const char *monster_letters[][2] = {
    { "angel", "A" }, { "bat", "b" }, { "bee", "a" }, { "beetles", "a" },
    { "beholder", "e" }, { "blob", "j" }, { "caterpillar", "w" },
    { "centipede", "c" }, { "chest", "m" }, { "cleaner", "j" },
    { "creepy", "C" }, { "death", "&" }, { "demon", "&" }, { "dragon", "D" },
    { "dwarve", "h" }, { "elf", "e" }, { "fairy", "f" }, { "firebug", "a" },
    { "firelord", "F" }, { "flayer", "h" }, { "galump", "G" },
    { "ghost", "G" }, { "glork", "g" }, { "gnasher", "n" }, { "goblin", "g" },
    { "golem", "'" }, { "hole", "^" }, { "hulk", "H" }, { "hydra", "H" },
    { "imp", "i" }, { "invisible_man", "@" }, { "knight", "K" },
    { "leech", "w" }, { "lich", "L" }, { "maggots", "w" }, { "medusa", "M" },
    { "minotaur", "T" }, { "mist", "v" }, { "morhock", "M" }, { "mouse", "r" },
    { "moving_wall", "#" }, { "mushroom", "," }, { "orc", "o" },
    { "police", "@" }, { "pumpkin", "P" }, { "rat", "r" }, { "roach", "a" },
    { "rust", "R" }, { "shopkeeper", "@" }, { "skeleton", "Z" },
    { "skull", "z" }, { "slug", "w" }, { "smurlinger", "S" }, { "snail", "w" },
    { "snakes", "S" }, { "spider", "s" }, { "spikes", "^" }, { "spirit", "W" },
    { "sprite", "f" }, { "squarm", "q" }, { "stoat", "q" }, { "stomper", "O" },
    { "teletrap", "^" }, { "troll", "T" }, { "vampire", "V" }, { "witch", "@" },
    { "wizard", "@" }, { "worm", "w" }, { "zombie", "Z" },
};

static const char *item_glyphs[][2] = {
    { "treasure/amulets", "\"" }, { "treasure/armour", "[" },
    { "treasure/axes", ")" }, { "treasure/bag_of_gold", "$" },
    { "treasure/bombs", "o" }, { "treasure/bows", "}" },
    { "treasure/chest", "=" }, { "treasure/coins", "$" },
    { "treasure/crowns", "^" }, { "treasure/fate", "&" },
    { "treasure/gems", "*" }, { "treasure/guns", "{" },
    { "treasure/keys", "-" }, { "treasure/staffs", "_" },
    { "treasure/swords", ")" }, { "treasure/wands", "/" },
    { "treasure/warhammers", ")" }, { "magic/potion", "!" },
    { "magic/spell", "?" }, { "food/", "%" }, { "powerups/", "+" },
    { "hoard/", "$" }, { "missiles/", "*" }, { "explosion/", "*" },
    { "weapons/", ")" },
};

static term_rgb art_color(gh_thing *t, uint32_t *ch_out)
{
    const gh_anim_frame *a = t->get_anim();
    term_rgb best = { 200, 200, 200 };
    if (!a || a->cell.empty()) {
        return best;
    }
    std::map<int, int> count;
    std::map<char, int> chars;
    for (size_t i = 0; i < a->cell.size(); i++) {
        const gh_anim_frame_cell &c = a->cell[i];
        if (c.c == ' ' || !c.c) {
            continue;
        }
        if (c.fg == GH_TERM_COLOR_BLACK || c.fg == GH_TERM_COLOR_RANDOM) {
            continue;
        }
        count[c.fg]++;
        if (isalnum((unsigned char) c.c) || strchr("@#$%&*", c.c)) {
            chars[c.c]++;
        }
    }
    int most = -1, n = 0;
    for (std::map<int, int>::iterator i = count.begin(); i != count.end(); ++i) {
        if (i->second > n) {
            n    = i->second;
            most = i->first;
        }
    }
    if (most >= 0) {
        best = rgb_of((uint8) most);
    }
    if (ch_out) {
        char c = 0;
        int  cn = 0;
        for (std::map<char, int>::iterator i = chars.begin(); i != chars.end(); ++i) {
            if (i->second > cn) {
                cn = i->second;
                c  = i->first;
            }
        }
        *ch_out = c ? (uint32_t) c : '?';
    }
    return best;
}

static glyph thing_glyph(gh_thing *t)
{
    glyph g;
    g.bg = (term_rgb) { 0, 0, 0 };
    uint32_t art_ch = 0;
    g.fg = art_color(t, &art_ch);
    g.ch = 0;

    const string &s = t->short_name;

    if (t->is_hero) {
        g.ch = '@';
        g.fg = (term_rgb) { 255, 255, 255 };
    } else if (t->is_monst) {
        for (size_t i = 0; i < sizeof(monster_letters) / sizeof(monster_letters[0]); i++) {
            string p = string("monst/") + monster_letters[i][0];
            if (s.compare(0, p.size(), p) == 0) {
                g.ch = (uint8) monster_letters[i][1][0];
                break;
            }
        }
        if (!g.ch) {
            g.ch = art_ch ? art_ch : 'm';
        }
    } else if (t->is_wall || t->is_rock) {
        g.ch = '#';
    } else if (t->is_door) {
        g.ch = t->is_open ? '\'' : '+';
    } else if (t->is_exit || t->is_return_exit) {
        g.ch = t->is_return_exit ? '<' : '>';
        g.fg = (term_rgb) { 255, 255, 120 };
    } else if (t->is_water) {
        g.ch = '~';
    } else if (t->is_trap) {
        g.ch = '^';
    } else if (t->is_generator) {
        g.ch = '0';
    } else if (t->is_corridor) {
        g.ch = '.';
        g.fg = dim(g.fg, 70);
    } else if (t->is_floor || t->is_dirt) {
        g.ch = '.';
        g.fg = dim(g.fg, 60);
    } else {
        for (size_t i = 0; i < sizeof(item_glyphs) / sizeof(item_glyphs[0]); i++) {
            string p = item_glyphs[i][0];
            if (s.compare(0, p.size(), p) == 0) {
                g.ch = (uint8) item_glyphs[i][1][0];
                break;
            }
        }
        if (!g.ch) {
            if (s.compare(0, 5, "room/") == 0) {
                if (s.find("lava") != string::npos) {
                    g.ch = '~';
                } else if (s.find("tree") != string::npos) {
                    g.ch = '&';
                } else if (s.find("pit") != string::npos || s.find("hole") != string::npos) {
                    g.ch = '^';
                } else if (s.find("teleport") != string::npos) {
                    g.ch = '^';
                } else if (s.find("void") != string::npos || s.find("border") != string::npos) {
                    g.ch = ' ';
                } else {
                    g.ch = art_ch ? art_ch : '.';
                }
            } else {
                g.ch = art_ch ? art_ch : '?';
            }
        }
    }

    if (is_black(g.fg)) {
        g.fg = (term_rgb) { 128, 128, 128 };
    }
    if (t->is_dead) {
        g.fg = (term_rgb) { 110, 110, 110 };
        if (t->is_monst || t->is_hero) {
            g.ch = '%';
        }
    }
    return g;
}

/*
 * The dungeon around the hero, w by h cells, using display_map()'s rules
 * for what may be shown: things in sight this frame, things in lit rooms,
 * and remembered walls, floors, water, doors and exits (dimmed).
 */
/* floor the hero has seen, per level: drawn dim once out of the light */
static gh_level            *render_mem_level;
static std::vector<uint8_t> render_mem;

/*
 * Is map square p inside the hero's light? The windowed game draws that
 * light as a polygon around the hero, one ray per angle; here a square is
 * lit when its centre is within its ray's reach.
 */
static bool in_light(gh_level *level, const gh_point3d &p, const float *radii, int rays)
{
    if (!radii || rays <= 0) {
        return false;
    }
    const int res = level->map.res;
    float dx = (float) (p.x * res + res / 2) - (float) level->map.hero_at.x;
    float dy = (float) (p.y * res + res / 2) - (float) level->map.hero_at.y;
    float d  = sqrtf(dx * dx + dy * dy);
    if (d < res) {
        return true;
    }
    float a = atan2f(dy, dx);
    if (a < 0) {
        a += (float) (2.0 * M_PI);
    }
    int ray = (int) (a / (float) (2.0 * M_PI / rays) + 0.5f) % rays;
    return d <= radii[ray] + res / 3;
}

/*
 * display_map()'s rules for what may be shown: things in sight this frame,
 * things in lit rooms, the floor under the hero's light, and remembered
 * walls, floor, water, doors and exits (not lit). The rank puts the hero
 * over monsters over items over the floor.
 */
static bool thing_seen(gh_thing *t, bool light, bool mem, bool *lit_out, int *rank_out)
{
    if (!t || !t->anim_frames) {
        return false;
    }
    /* shielded things flicker; the hero only changes colour */
    if (t->shielded() && !t->is_hero && (SDL_GetTicks() / 120) % 2) {
        return false;
    }
    const gh_anim_frame *a = t->get_anim();
    if (a && a->invisible && !t->is_hero) {
        return false;
    }

    bool lit    = false;
    bool ground = t->is_floor || t->is_corridor || t->is_dirt;
    if (t->room && t->room->is_lit) {
        lit = true;
    } else if (t->rendered == gh_rendered_this_frame) {
        lit = true;
    } else if (ground && light) {
        lit = true;
    } else if (ground && mem) {
        lit = false;
    } else if (t->has_been_seen) {
        if (!t->is_water && !t->is_boring && !t->is_door && !t->is_exit) {
            return false;
        }
    } else {
        return false;
    }

    int rank = t->render_depth;
    if (t->is_hero) {
        rank += 1000;
    } else if (t->is_monst && !t->is_dead) {
        rank += 800;
    } else if (ground) {
        rank -= 300;
    }
    *lit_out  = lit;
    *rank_out = rank;
    return true;
}

void render_capture_map(gh_level *level, gh_thing *hero, const float *radii, int rays)
{
    int w = render_map_w;
    int h = render_map_h;
    if (w <= 0 || h <= 0 || !level || !hero) {
        return;
    }

    const int res = level->map.res;
    gh_point3d c(hero->at.x / res, hero->at.y / res, 0);
    int x0 = c.x - w / 2;
    int y0 = c.y - h / 2;

    size_t cells = (size_t) level->map.size.w * (size_t) level->map.size.h;
    if (render_mem_level != level || render_mem.size() != cells) {
        render_mem_level = level;
        render_mem.assign(cells, 0);
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            map_cell &m = render_map[y][x];
            m.set = false;

            gh_point3d p(x0 + x, y0 + y, 0);
            if (p.x < 0 || p.y < 0 || p.x >= level->map.size.w || p.y >= level->map.size.h) {
                continue;
            }
            vector<gh_thing*> *v = level->map.isat(p);
            if (!v || v->empty()) {
                continue;
            }

            uint8_t &mem   = render_mem[(size_t) p.y * level->map.size.w + p.x];
            bool     light = in_light(level, p, radii, rays);
            if (light) {
                mem = 1;
            }

            gh_thing *best      = NULL;
            bool      best_lit  = false;
            int       best_rank = -100000;

            for (size_t i = 0; i < v->size(); i++) {
                gh_thing *t = (*v)[i];
                if (!t || !t->anim_frames) {
                    continue;
                }
                bool lit;
                int  rank;
                if (!thing_seen(t, light, mem, &lit, &rank)) {
                    continue;
                }
                if (rank >= best_rank) {
                    best_rank = rank;
                    best      = t;
                    best_lit  = lit;
                }
            }

            if (!best) {
                continue;
            }
            glyph g = thing_glyph(best);
            if (!best_lit) {
                g.fg = dim(g.fg, 45);
            }
            if (best->is_hero) {
                g.bg = (term_rgb) { 40, 40, 90 };
                if (best->shielded() && (SDL_GetTicks() / 120) % 2) {
                    g.fg = (term_rgb) { 120, 220, 255 };
                }
            }
            m.ch  = g.ch;
            m.fg  = g.fg;
            m.bg  = g.bg;
            m.set = true;
        }
    }
    if (render_zoom) {
        render_zoom_map(level, hero, radii, rays);
    }
    render_map_valid = true;

    /* a snapshot for the session log, once a second */
    uint32_t now = SDL_GetTicks();
    if (now - render_snapshot_ms >= 1000) {
        render_snapshot_ms = now;
        char line[160];
        snprintf(line, sizeof(line), "snapshot level %d at %d,%d map %dx%d view %dx%d",
                 (int) level->level_no, (int) c.x, (int) c.y,
                 (int) level->map.size.w, (int) level->map.size.h, w, h);
        terminal_log(line);
    }
}

/*
 * Tab: the zoomed view. Each thing drawn as its own picture of characters,
 * as the windowed game draws it, one character a terminal cell (a map
 * square is 6 by 6). Shows less of the dungeon, but every orc has its
 * face. Same rules for what is seen; the hero in the middle.
 */
void render_toggle_zoom(void)
{
    render_zoom = !render_zoom;
    terminal_log(render_zoom ? "zoom on" : "zoom off");
}

struct zoom_item {
    gh_thing *t;
    bool      lit;
    int       rank;
};

static bool zoom_order(const zoom_item &a, const zoom_item &b) { return a.rank < b.rank; }

static void render_zoom_map(gh_level *level, gh_thing *hero, const float *radii, int rays)
{
    int w = render_map_w;
    int h = render_map_h;
    const int res = level->map.res;
    const int sq  = res / GH_RES_CHAR;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            render_map[y][x].set = false;
        }
    }

    gh_point3d c(hero->at.x / res, hero->at.y / res, 0);
    int rx = w / sq / 2 + 3;
    int ry = h / sq / 2 + 3;

    std::vector<zoom_item> items;
    for (int my = c.y - ry; my <= c.y + ry; my++) {
        for (int mx = c.x - rx; mx <= c.x + rx; mx++) {
            gh_point3d p(mx, my, 0);
            if (p.x < 0 || p.y < 0 || p.x >= level->map.size.w || p.y >= level->map.size.h) {
                continue;
            }
            vector<gh_thing*> *v = level->map.isat(p);
            if (!v) {
                continue;
            }
            uint8_t mem   = render_mem[(size_t) p.y * level->map.size.w + p.x];
            bool    light = in_light(level, p, radii, rays);
            for (size_t i = 0; i < v->size(); i++) {
                gh_thing *t = (*v)[i];
                bool lit;
                int  rank;
                if (thing_seen(t, light, mem, &lit, &rank)) {
                    zoom_item z = { t, lit, rank };
                    items.push_back(z);
                }
            }
        }
    }
    std::stable_sort(items.begin(), items.end(), zoom_order);

    for (size_t i = 0; i < items.size(); i++) {
        gh_thing *t = items[i].t;
        const gh_anim_frame *a = t->get_anim();
        if (!a || a->cell.empty()) {
            continue;
        }
        int ox = (t->at.x - hero->at.x) / GH_RES_CHAR + w / 2 - a->width / 2;
        int oy = (t->at.y - hero->at.y) / GH_RES_CHAR + h / 2 - a->height / 2;
        for (int ay = 0; ay < a->height; ay++) {
            int y = oy + ay;
            if (y < 0 || y >= h) {
                continue;
            }
            for (int ax = 0; ax < a->width; ax++) {
                int x = ox + ax;
                if (x < 0 || x >= w) {
                    continue;
                }
                const gh_anim_frame_cell &cell = a->cell[(size_t) ay * a->width + ax];
                if (!cell.c || (cell.c == ' ' && cell.bg == GH_TERM_COLOR_BLACK)) {
                    continue;
                }
                map_cell &m = render_map[y][x];
                term_rgb fg = rgb_of(t->is_dead ? (uint8) GH_TERM_COLOR_GRAY_4 : cell.fg);
                term_rgb bg = rgb_of(t->is_dead ? (uint8) GH_TERM_COLOR_BLACK : cell.bg);
                if (!items[i].lit) {
                    fg = dim(fg, 45);
                    bg = dim(bg, 45);
                }
                m.ch  = (uint8) cell.c;
                m.fg  = fg;
                m.bg  = bg;
                m.set = true;
            }
        }
    }
}

/*
 * The game's console grid fits the terminal; the map leaves room for the
 * score column on the right.
 */
void render_set_layout(int cols, int rows)
{
    GH_TERM_WIDTH  = std::min(cols, (int) GH_TERM_MAX_SIZE);
    GH_TERM_HEIGHT = std::min(rows, (int) GH_TERM_MAX_SIZE / 2);
    render_map_w   = std::max(10, GH_TERM_WIDTH - 23);
    render_map_h   = std::max(5, GH_TERM_HEIGHT - 5);
}

void render_compose(int cols, int rows)
{
    if (cols > 0 && rows > 0) {
        int w = std::min(cols, (int) GH_TERM_MAX_SIZE);
        int h = std::min(rows, (int) GH_TERM_MAX_SIZE / 2);

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                term_cell &o = term_frame[y][x];
                o.ch = ' ';
                o.fg = (term_rgb) { 200, 200, 200 };
                o.bg = (term_rgb) { 0, 0, 0 };
            }
        }

        if (render_map_valid) {
            /* the map sits under the health bar (rows 0 and 1) */
            for (int y = 0; y < render_map_h && y + 2 < h; y++) {
                for (int x = 0; x < render_map_w && x < w; x++) {
                    const map_cell &m = render_map[y][x];
                    if (!m.set) {
                        continue;
                    }
                    term_cell &o = term_frame[y + 2][x];
                    o.ch = m.ch;
                    o.fg = m.fg;
                    o.bg = m.bg;
                }
            }
        }

        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                if (!render_term_set[x][y]) {
                    continue;
                }
                const gh_term_cell &c = render_term[x][y];
                term_cell &o = term_frame[y][x];
                o.ch = (uint8) c.c;
                o.fg = rgb_of(c.fg);
                o.bg = rgb_of(c.bg);
            }
        }

        /* follow the terminal's size from the next frame on */
        render_set_layout(cols, rows);
    }

    memset(render_term_set, 0, sizeof(render_term_set));
    memset(render_opaque_cells, 0, sizeof(render_opaque_cells));
    render_map_valid = false;
}
