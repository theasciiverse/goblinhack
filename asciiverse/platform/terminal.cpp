/*
 * Asciiverse terminal edition of Goblin Hack: the terminal.
 *
 * Before main() runs, the game's working folder becomes the player's state
 * folder (saves, high scores, bones, config and the game's own logs land
 * there), and the terminal gets a private handle so nothing the game prints
 * reaches the screen.
 *
 * Input: keys come from the kitty keyboard protocol where the terminal has
 * it (press, repeat and release), else from plain bytes and the usual escape
 * sequences, where a key counts as held for a short while after each press
 * or auto repeat. Mouse comes from SGR reporting. Both become SDL 1.2 events.
 *
 * Output: render.cpp fills term_frame; only the cells that changed since the
 * last frame are written, in 24-bit colour.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 * The input parser follows the one written for the terminal edition of
 * Neil McGill's Zorbash (theasciiverse/zorbash, src/terminal.cpp).
 */
#include "terminal.h"
#include "ghack_sdl_shim.h"

#include <algorithm>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

term_cell term_frame[TERM_MAX_H][TERM_MAX_W];

static bool           term_tty;
static bool           term_started;
static int            term_fd = -1;
static bool           term_have_saved_termios;
static struct termios term_saved_termios;
static int            term_cols = 100;
static int            term_rows = 30;
static volatile bool  term_resized;
static bool           term_kitty_keys;
static std::string    term_in;
static uint64_t       term_in_esc_at;
static uint64_t       term_last_frame_ms;
static std::string    term_state;
static std::string    term_exe;
static FILE          *term_logf;

/* what is on the terminal now */
static term_cell term_screen[TERM_MAX_H][TERM_MAX_W];
static bool      term_screen_valid;
static int       term_off_x;
static int       term_off_y;
static int       term_w;
static int       term_h;

/* held keys, by SDL 1.2 key code */
static bool       term_held[SDLK_LAST];
static uint64_t   term_release_at[SDLK_LAST];
static SDL_keysym term_held_sym[SDLK_LAST];

static const uint64_t TERM_KEY_HOLD_MS = 90;

/*
 * Goblin Hack walks smoothly while a direction key is held. Without key
 * releases, a first press of a direction holds long enough to walk about
 * one square, and bridges the terminal's delay before auto repeat starts.
 */
static const uint64_t TERM_MOVE_FIRST_HOLD_MS = 230;

static bool term_is_move_key(int k)
{
    return k == SDLK_UP || k == SDLK_DOWN || k == SDLK_LEFT || k == SDLK_RIGHT ||
           (k >= SDLK_KP1 && k <= SDLK_KP9);
}
static const uint64_t TERM_ESC_WAIT_MS = 30;

static uint64_t term_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void term_write(const std::string &s)
{
    const char *p = s.data();
    size_t      n = s.size();
    while (n && term_fd >= 0) {
        ssize_t w = write(term_fd, p, n);
        if (w < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return;
        }
        p += w;
        n -= (size_t) w;
    }
}

void terminal_log(const std::string &line)
{
    if (!term_logf) {
        return;
    }
    fprintf(term_logf, "%llu %s\n", (unsigned long long) SDL_GetTicks(), line.c_str());
    fflush(term_logf);
}

static void term_read_size(void)
{
    struct winsize ws;
    if (term_fd >= 0 && ioctl(term_fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row) {
        term_cols = ws.ws_col;
        term_rows = ws.ws_row;
    }
}

static void term_on_winch(int) { term_resized = true; }

int terminal_cols(void) { return std::min(term_cols, (int) TERM_MAX_W); }
int terminal_rows(void) { return std::min(term_rows, (int) TERM_MAX_H); }

////////////////////////////////////////////////////////////////////////////
// Folders
////////////////////////////////////////////////////////////////////////////

static void term_mkdirs(const std::string &path)
{
    for (size_t i = 1; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/') {
            mkdir(path.substr(0, i).c_str(), 0700);
        }
    }
}

static bool term_is_dir(const std::string &p)
{
    struct stat st;
    return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

/*
 * Copy a folder tree's directories (not its files) into the state folder:
 * the game writes into the first search folder that has the directory.
 */
static void term_mirror_dirs(const std::string &from, const std::string &to)
{
    term_mkdirs(to);
    DIR *d = opendir(from.c_str());
    if (!d) {
        return;
    }
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') {
            continue;
        }
        std::string f = from + "/" + e->d_name;
        if (term_is_dir(f)) {
            term_mirror_dirs(f, to + "/" + e->d_name);
        }
    }
    closedir(d);
}

std::string terminal_state_dir(void) { return term_state; }
std::string terminal_exe_dir(void) { return term_exe; }

static void term_find_exe(void)
{
    char buf[4096];
    buf[0] = 0;
#ifdef __APPLE__
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) {
        buf[0] = 0;
    }
#else
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    buf[n > 0 ? n : 0] = 0;
#endif
    char real[4096];
    if (buf[0] && realpath(buf, real)) {
        term_exe = real;
    } else {
        term_exe = buf;
    }
    size_t slash = term_exe.rfind('/');
    term_exe = (slash == std::string::npos) ? "." : term_exe.substr(0, slash);
}

/*
 * The game's resources: beside the binary (av), in the source tree (a
 * developer build), or the installed share folder (packages).
 */
static std::string term_resources(void)
{
    const char *cands[] = { "/resources", "/src/resources", "/../src/resources", "/../share/goblinhack/resources" };
    for (size_t i = 0; i < sizeof(cands) / sizeof(cands[0]); i++) {
        std::string d = term_exe + cands[i];
        if (term_is_dir(d + "/things")) {
            return d;
        }
    }
    return "";
}

static void term_setup_state(void)
{
    std::string dir;
    const char *xdg = getenv("XDG_STATE_HOME");
    if (xdg && xdg[0] == '/') {
        dir = xdg;
    } else {
        const char *home = getenv("HOME");
        if (home && home[0]) {
#ifdef __APPLE__
            dir = std::string(home) + "/Library/Application Support";
#else
            dir = std::string(home) + "/.local/state";
#endif
        }
    }
    if (dir.empty()) {
        return;
    }
    term_state = dir + "/goblinhack";
    term_mkdirs(term_state);

    std::string res = term_resources();
    if (!res.empty()) {
        const char *writable[] = { "bones", "config", "levels", "runtime", "saved" };
        for (size_t i = 0; i < sizeof(writable) / sizeof(writable[0]); i++) {
            if (term_is_dir(res + "/" + writable[i])) {
                term_mirror_dirs(res + "/" + writable[i],
                                 term_state + "/resources/" + writable[i]);
            }
        }
    }

    if (chdir(term_state.c_str()) != 0) {
        return;
    }

    const char *lv = getenv("GOBLINHACK_LOG");
    if (lv && *lv) {
        term_logf = fopen("goblinhack-terminal.log", "w");
    }
}

////////////////////////////////////////////////////////////////////////////
// Start and stop
////////////////////////////////////////////////////////////////////////////

bool terminal_init(void)
{
    term_find_exe();
    term_setup_state();

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        return false;
    }
    setlocale(LC_CTYPE, "");

    /* our own handle; the game's prints go to its stdout.txt and stderr.txt */
    term_fd = dup(STDOUT_FILENO);
    fcntl(term_fd, F_SETFD, FD_CLOEXEC);
    fflush(stdout);
    fflush(stderr);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
    }
    term_tty = true;
    term_read_size();
    atexit(terminal_fini);
    return true;
}

static bool term_silent;

bool terminal_silent(void) { return term_silent; }

/*
 * Runs before the game's own static constructors, which read the cwd. The
 * C runtime hands constructors the program's arguments on Linux and macOS.
 */
__attribute__((constructor(101))) static void term_early(int argc, char **argv, char **)
{
    const char *env = getenv("GOBLINHACK_SILENT");
    term_silent = env && *env && strcmp(env, "0");
    for (int i = 1; argv && i < argc; i++) {
        if (!strcmp(argv[i], "--silent") || !strcmp(argv[i], "-s")) {
            term_silent = true;
        }
    }
    terminal_init();
}

static void term_fatal(int sig)
{
    terminal_fini();
    signal(sig, SIG_DFL);
    raise(sig);
}

void terminal_start(void)
{
    if (term_started || !term_tty) {
        return;
    }

    if (tcgetattr(STDIN_FILENO, &term_saved_termios) == 0) {
        term_have_saved_termios = true;
        struct termios raw      = term_saved_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = term_on_winch;
    sigaction(SIGWINCH, &sa, NULL);
    signal(SIGSEGV, term_fatal);
    signal(SIGABRT, term_fatal);
    signal(SIGBUS, term_fatal);
    signal(SIGTERM, term_fatal);

    term_started = true;

    /*
     * Alternate screen, hide cursor, no autowrap, mouse with motion in SGR
     * form, then kitty keyboard flags (disambiguate, event types, all keys
     * as escapes) and a check whether they took.
     */
    term_write("\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[2J"
               "\x1b[?1003h\x1b[?1006h"
               "\x1b[>11u\x1b[?u\x1b[c");
    term_screen_valid = false;
}

void terminal_fini(void)
{
    if (!term_started) {
        return;
    }
    term_started = false;

    static const char reset[] = "\x1b[<u\x1b[?1003l\x1b[?1006l\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l";
    ssize_t unused = write(term_fd, reset, sizeof(reset) - 1);
    (void) unused;

    if (term_have_saved_termios) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_saved_termios);
    }
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

////////////////////////////////////////////////////////////////////////////
// Keys
////////////////////////////////////////////////////////////////////////////

enum {
    TK_NONE = 0,
    KITTY_KP_0 = 57399, KITTY_KP_9 = 57408, KITTY_KP_DECIMAL = 57409,
    KITTY_KP_DIVIDE, KITTY_KP_MULTIPLY, KITTY_KP_SUBTRACT, KITTY_KP_ADD,
    KITTY_KP_ENTER, KITTY_KP_EQUAL, KITTY_KP_SEPARATOR, KITTY_KP_LEFT,
    KITTY_KP_RIGHT, KITTY_KP_UP, KITTY_KP_DOWN, KITTY_KP_PAGE_UP,
    KITTY_KP_PAGE_DOWN, KITTY_KP_HOME, KITTY_KP_END, KITTY_KP_INSERT,
    KITTY_KP_DELETE, KITTY_KP_BEGIN,
    KITTY_MODIFIER_LOW = 57441, KITTY_MODIFIER_HIGH = 57454,
    TK_UP = 0x110000, TK_DOWN, TK_LEFT, TK_RIGHT, TK_HOME, TK_END,
    TK_PAGEUP, TK_PAGEDOWN, TK_INSERT, TK_DELETE, TK_F1,
    TK_F12 = TK_F1 + 11,
};

static SDLMod term_mods(int m)
{
    int mod = KMOD_NONE;
    if (m & 1) mod |= KMOD_LSHIFT;
    if (m & 2) mod |= KMOD_LALT;
    if (m & 4) mod |= KMOD_LCTRL;
    return (SDLMod) mod;
}

static bool term_key_to_sdl(int key, SDLMod mod, SDL_keysym *ks)
{
    memset(ks, 0, sizeof(*ks));
    int sym = SDLK_UNKNOWN;

    if (key >= KITTY_MODIFIER_LOW && key <= KITTY_MODIFIER_HIGH) {
        return false;
    }

    switch (key) {
    case 13: sym = SDLK_RETURN; break;
    case 27: sym = SDLK_ESCAPE; break;
    case 9: sym = SDLK_TAB; break;
    case 127:
    case 8: sym = SDLK_BACKSPACE; break;
    case 32: sym = SDLK_SPACE; break;
    case TK_UP: sym = SDLK_UP; break;
    case TK_DOWN: sym = SDLK_DOWN; break;
    case TK_LEFT: sym = SDLK_LEFT; break;
    case TK_RIGHT: sym = SDLK_RIGHT; break;
    case TK_HOME: sym = SDLK_HOME; break;
    case TK_END: sym = SDLK_END; break;
    case TK_PAGEUP: sym = SDLK_PAGEUP; break;
    case TK_PAGEDOWN: sym = SDLK_PAGEDOWN; break;
    case TK_INSERT: sym = SDLK_INSERT; break;
    case TK_DELETE: sym = SDLK_DELETE; break;
    case KITTY_KP_DECIMAL:
    case KITTY_KP_DELETE: sym = SDLK_KP_PERIOD; break;
    case KITTY_KP_DIVIDE: sym = SDLK_KP_DIVIDE; break;
    case KITTY_KP_MULTIPLY: sym = SDLK_KP_MULTIPLY; break;
    case KITTY_KP_SUBTRACT: sym = SDLK_KP_MINUS; break;
    case KITTY_KP_ADD: sym = SDLK_KP_PLUS; break;
    case KITTY_KP_ENTER: sym = SDLK_KP_ENTER; break;
    case KITTY_KP_EQUAL: sym = SDLK_KP_EQUALS; break;
    /* keypad with num lock off: the game reads the keypad as 7 8 9 / 4 6 / 1 2 3 */
    case KITTY_KP_LEFT: sym = SDLK_KP4; break;
    case KITTY_KP_RIGHT: sym = SDLK_KP6; break;
    case KITTY_KP_UP: sym = SDLK_KP8; break;
    case KITTY_KP_DOWN: sym = SDLK_KP2; break;
    case KITTY_KP_PAGE_UP: sym = SDLK_KP9; break;
    case KITTY_KP_PAGE_DOWN: sym = SDLK_KP3; break;
    case KITTY_KP_HOME: sym = SDLK_KP7; break;
    case KITTY_KP_END: sym = SDLK_KP1; break;
    case KITTY_KP_INSERT: sym = SDLK_KP0; break;
    case KITTY_KP_BEGIN: sym = SDLK_KP5; break;
    default: break;
    }

    if (sym == SDLK_UNKNOWN) {
        if (key >= KITTY_KP_0 && key <= KITTY_KP_9) {
            sym = SDLK_KP0 + (key - KITTY_KP_0);
        } else if (key >= TK_F1 && key <= TK_F12) {
            sym = SDLK_F1 + (key - TK_F1);
        } else if (key >= 'A' && key <= 'Z') {
            sym = key - 'A' + 'a';
            mod = (SDLMod) (mod | KMOD_LSHIFT);
        } else if (key > 32 && key < 127) {
            /* a shifted symbol, as SDL sees it on a US keyboard */
            static const char shifted[]   = "!@#$%^&*()_+{}|:\"<>?~";
            static const char unshifted[] = "1234567890-=[]\\;',./`";
            const char       *p           = strchr(shifted, key);
            if (p) {
                sym = unshifted[p - shifted];
                mod = (SDLMod) (mod | KMOD_LSHIFT);
            } else {
                sym = key;
            }
        } else {
            return false;
        }
    }

    ks->sym     = (SDLKey) sym;
    ks->mod     = mod;
    ks->unicode = (key > 0 && key < 0x10000) ? (Uint16) key : 0;
    ks->scancode = (Uint8) (sym & 0xff);
    return true;
}

static void term_push_key(const SDL_keysym &ks, bool down)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type       = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.type   = e.type;
    e.key.state  = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym = ks;
    SDL_PushEvent(&e);
    if (term_logf) {
        char line[96];
        snprintf(line, sizeof(line), "key %s %s mod %d", down ? "down" : "up",
                 SDL_GetKeyName(ks.sym), (int) ks.mod);
        terminal_log(line);
    }
}

/* event: 1 press, 2 repeat, 3 release */
static void term_key_event(int key, int mods, int event)
{
    /* Ctrl-C quits, as in any terminal program */
    if ((key == 'c' || key == 3) && (mods & 4) && event != 3) {
        terminal_log("ctrl-c: quit");
        terminal_fini();
        exit(0);
    }

    SDL_keysym ks;
    if (!term_key_to_sdl(key, term_mods(mods), &ks)) {
        return;
    }
    int k = ks.sym;
    if (k <= 0 || k >= SDLK_LAST) {
        return;
    }

    if (event == 3) {
        if (term_held[k]) {
            term_held[k] = false;
            term_push_key(term_held_sym[k], false);
        }
        return;
    }

    /* the same key with other modifiers still held: let it go first */
    if (term_held[k] && term_held_sym[k].mod != ks.mod) {
        term_push_key(term_held_sym[k], false);
        term_held[k] = false;
    }

    if (term_held[k]) {
        /* auto repeat of a held key: the game counts held keys itself */
        if (!term_kitty_keys) {
            term_release_at[k] = std::max(term_release_at[k], term_now_ms() + TERM_KEY_HOLD_MS);
        }
        return;
    }

    term_held[k]     = true;
    term_held_sym[k] = ks;
    if (!term_kitty_keys) {
        term_release_at[k] = term_now_ms() +
            (term_is_move_key(k) ? TERM_MOVE_FIRST_HOLD_MS : TERM_KEY_HOLD_MS);
    }
    term_push_key(ks, true);
}

static void term_release_expired_keys(void)
{
    if (term_kitty_keys) {
        return;
    }
    uint64_t now = term_now_ms();
    for (int k = 0; k < SDLK_LAST; k++) {
        if (term_held[k] && now >= term_release_at[k]) {
            term_held[k] = false;
            term_push_key(term_held_sym[k], false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// Mouse: terminal cells to the game's 1024x800 screen
////////////////////////////////////////////////////////////////////////////

static void term_mouse_event(int b, int x, int y, bool release)
{
    int cx = x - 1 - term_off_x;
    int cy = y - 1 - term_off_y;
    if (term_w <= 0 || term_h <= 0) {
        return;
    }
    cx = std::max(0, std::min(cx, term_w - 1));
    cy = std::max(0, std::min(cy, term_h - 1));
    int px = (cx * 1024 + 512) / term_w;
    int py = (cy * 800 + 400) / term_h;

    SDL_Event e;
    memset(&e, 0, sizeof(e));

    if (b & 32) {
        e.type     = SDL_MOUSEMOTION;
        e.motion.x = (Uint16) px;
        e.motion.y = (Uint16) py;
        SDL_PushEvent(&e);
        return;
    }

    int button;
    if (b & 64) {
        if (release) {
            return;
        }
        button = ((b & 3) == 0) ? SDL_BUTTON_WHEELUP : SDL_BUTTON_WHEELDOWN;
    } else {
        switch (b & 3) {
        case 0: button = SDL_BUTTON_LEFT; break;
        case 1: button = SDL_BUTTON_MIDDLE; break;
        case 2: button = SDL_BUTTON_RIGHT; break;
        default: return;
        }
    }

    e.type          = release ? SDL_MOUSEBUTTONUP : SDL_MOUSEBUTTONDOWN;
    e.button.button = (Uint8) button;
    e.button.state  = release ? SDL_RELEASED : SDL_PRESSED;
    e.button.x      = (Uint16) px;
    e.button.y      = (Uint16) py;
    SDL_PushEvent(&e);
}

////////////////////////////////////////////////////////////////////////////
// Parsing
////////////////////////////////////////////////////////////////////////////

static void term_parse_params(const std::string &s, int p[4][3], int *np)
{
    memset(p, 0, sizeof(int) * 12);
    int i = 0, j = 0;
    *np = s.empty() ? 0 : 1;
    for (size_t n = 0; n < s.size(); n++) {
        char c = s[n];
        if (c == ';') {
            if (i < 3) i++;
            j = 0;
            (*np)++;
        } else if (c == ':') {
            if (j < 2) j++;
        } else if (c >= '0' && c <= '9') {
            p[i][j] = p[i][j] * 10 + (c - '0');
        }
    }
}

static void term_csi(char prefix, const std::string &params, char final)
{
    int p[4][3];
    int np;
    term_parse_params(params, p, &np);

    if (prefix == '<' && (final == 'M' || final == 'm')) {
        term_mouse_event(p[0][0], p[1][0], p[2][0], final == 'm');
        return;
    }
    if (prefix == '?' && final == 'u') {
        term_kitty_keys = true;
        terminal_log("kitty keyboard protocol on");
        return;
    }
    if (prefix) {
        return;
    }

    int mods  = (np >= 2 && p[1][0]) ? p[1][0] - 1 : 0;
    int event = (np >= 2 && p[1][1]) ? p[1][1] : 1;
    int key   = TK_NONE;

    switch (final) {
    case 'u': key = p[0][0]; break;
    case 'A': key = TK_UP; break;
    case 'B': key = TK_DOWN; break;
    case 'C': key = TK_RIGHT; break;
    case 'D': key = TK_LEFT; break;
    case 'H': key = TK_HOME; break;
    case 'F': key = TK_END; break;
    case 'E': key = KITTY_KP_BEGIN; break;
    case 'P': key = TK_F1; break;
    case 'Q': key = TK_F1 + 1; break;
    case 'S': key = TK_F1 + 3; break;
    case 'Z': key = 9; mods |= 1; break;
    case '~':
        switch (p[0][0]) {
        case 1: case 7: key = TK_HOME; break;
        case 4: case 8: key = TK_END; break;
        case 2: key = TK_INSERT; break;
        case 3: key = TK_DELETE; break;
        case 5: key = TK_PAGEUP; break;
        case 6: key = TK_PAGEDOWN; break;
        case 11: key = TK_F1; break;
        case 12: key = TK_F1 + 1; break;
        case 13: key = TK_F1 + 2; break;
        case 14: key = TK_F1 + 3; break;
        case 15: key = TK_F1 + 4; break;
        case 17: key = TK_F1 + 5; break;
        case 18: key = TK_F1 + 6; break;
        case 19: key = TK_F1 + 7; break;
        case 20: key = TK_F1 + 8; break;
        case 21: key = TK_F1 + 9; break;
        case 23: key = TK_F1 + 10; break;
        case 24: key = TK_F1 + 11; break;
        default: break;
        }
        break;
    default: break;
    }

    if (key != TK_NONE) {
        term_key_event(key, mods, event);
    }
}

static void term_parse_input(bool flush_lone_esc)
{
    size_t i = 0;
    while (i < term_in.size()) {
        unsigned char c = term_in[i];

        if (c != 0x1b) {
            int    key = c;
            size_t len = 1;
            if (c >= 0xc0) {
                len = (c >= 0xf0) ? 4 : (c >= 0xe0) ? 3 : 2;
                if (i + len > term_in.size()) {
                    break;
                }
                key = 0;
            }
            if (c == 13 || c == 10) {
                term_key_event(13, 0, 1);
            } else if (c == 9) {
                term_key_event(9, 0, 1);
            } else if (c == 127 || c == 8) {
                term_key_event(127, 0, 1);
            } else if (c >= 1 && c <= 26) {
                term_key_event('a' + c - 1, 4, 1);
            } else if (key) {
                term_key_event(key, 0, 1);
            }
            i += len;
            continue;
        }

        if (i + 1 >= term_in.size()) {
            if (flush_lone_esc) {
                term_key_event(27, 0, 1);
                i++;
                continue;
            }
            break;
        }

        unsigned char n = term_in[i + 1];

        if (n == '[') {
            size_t j      = i + 2;
            char   prefix = 0;
            if (j < term_in.size() && strchr("<?>=", term_in[j])) {
                prefix = term_in[j];
                j++;
            }
            size_t start = j;
            while (j < term_in.size() && !(term_in[j] >= 0x40 && term_in[j] <= 0x7e)) {
                j++;
            }
            if (j >= term_in.size()) {
                break;
            }
            term_csi(prefix, term_in.substr(start, j - start), term_in[j]);
            i = j + 1;
            continue;
        }

        if (n == 'O') {
            if (i + 2 >= term_in.size()) {
                break;
            }
            term_csi(0, "", term_in[i + 2]);
            i += 3;
            continue;
        }

        if (n >= 32 && n < 127) {
            term_key_event(n, 2, 1);
            i += 2;
            continue;
        }

        term_key_event(27, 0, 1);
        i++;
    }
    term_in.erase(0, i);
}

void terminal_wait(int ms)
{
    if (!term_started) {
        if (ms > 0) {
            usleep((useconds_t) ms * 1000);
        }
        return;
    }
    struct pollfd pfd;
    pfd.fd      = STDIN_FILENO;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    poll(&pfd, 1, ms);
}

void terminal_poll(void)
{
    if (!term_started) {
        return;
    }

    char buf[4096];
    for (;;) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        term_in.append(buf, (size_t) n);
    }

    if (!term_in.empty()) {
        bool waiting_on_esc = (term_in[term_in.size() - 1] == 0x1b) ||
                              (term_in.size() >= 2 && term_in[term_in.size() - 2] == 0x1b);
        if (waiting_on_esc && !term_in_esc_at) {
            term_in_esc_at = term_now_ms();
        }
        bool flush = term_in_esc_at && (term_now_ms() - term_in_esc_at >= TERM_ESC_WAIT_MS);
        term_parse_input(flush);
        if (term_in.empty()) {
            term_in_esc_at = 0;
        }
    }

    term_release_expired_keys();

    if (term_resized) {
        term_resized = false;
        term_read_size();
        term_screen_valid = false;
        char line[64];
        snprintf(line, sizeof(line), "resize %dx%d", term_cols, term_rows);
        terminal_log(line);
    }
}

////////////////////////////////////////////////////////////////////////////
// Output
////////////////////////////////////////////////////////////////////////////

static void term_put_utf8(std::string &out, uint32_t c)
{
    if (c < 0x80) {
        out += (char) c;
    } else if (c < 0x800) {
        out += (char) (0xc0 | (c >> 6));
        out += (char) (0x80 | (c & 0x3f));
    } else if (c < 0x10000) {
        out += (char) (0xe0 | (c >> 12));
        out += (char) (0x80 | ((c >> 6) & 0x3f));
        out += (char) (0x80 | (c & 0x3f));
    } else {
        out += (char) (0xf0 | (c >> 18));
        out += (char) (0x80 | ((c >> 12) & 0x3f));
        out += (char) (0x80 | ((c >> 6) & 0x3f));
        out += (char) (0x80 | (c & 0x3f));
    }
}

void terminal_frame(void)
{
    /* about 30 frames a second is plenty for a terminal */
    uint64_t now = term_now_ms();
    if (term_screen_valid && now - term_last_frame_ms < 33) {
        render_compose(0, 0); /* drop this frame's captures */
        return;
    }
    term_last_frame_ms = now;

    int w = terminal_cols();
    int h = terminal_rows();
    render_compose(w, h);

    if (!term_started) {
        return;
    }

    int off_x = std::max(0, (term_cols - w) / 2);
    int off_y = std::max(0, (term_rows - h) / 2);

    std::string out;
    out.reserve(64 * 1024);

    if (!term_screen_valid || off_x != term_off_x || off_y != term_off_y || w != term_w || h != term_h) {
        out += "\x1b[0m\x1b[48;2;0;0;0m\x1b[2J";
        for (int y = 0; y < TERM_MAX_H; y++) {
            for (int x = 0; x < TERM_MAX_W; x++) {
                term_screen[y][x].ch = 0;
            }
        }
        term_off_x        = off_x;
        term_off_y        = off_y;
        term_w            = w;
        term_h            = h;
        term_screen_valid = true;
    }

    bool     have_pen = false;
    term_rgb pen_fg   = { 0, 0, 0 };
    term_rgb pen_bg   = { 0, 0, 0 };
    int      cur_x    = -1;
    int      cur_y    = -1;
    char     esc[64];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            term_cell c = term_frame[y][x];
            if (c.ch < 32 || c.ch == 127) {
                c.ch = ' ';
            } else if (c.ch > 127) {
                int cw = wcwidth((wchar_t) c.ch);
                if (cw != 1) {
                    c.ch = '?';
                }
            }

            term_cell &t = term_screen[y][x];
            if (t.ch == c.ch && t.fg == c.fg && t.bg == c.bg) {
                continue;
            }
            t = c;

            if (cur_x != x || cur_y != y) {
                snprintf(esc, sizeof(esc), "\x1b[%d;%dH", y + 1 + off_y, x + 1 + off_x);
                out += esc;
            }
            if (!have_pen || !(pen_fg == c.fg)) {
                snprintf(esc, sizeof(esc), "\x1b[38;2;%u;%u;%um", c.fg.r, c.fg.g, c.fg.b);
                out += esc;
                pen_fg = c.fg;
            }
            if (!have_pen || !(pen_bg == c.bg)) {
                snprintf(esc, sizeof(esc), "\x1b[48;2;%u;%u;%um", c.bg.r, c.bg.g, c.bg.b);
                out += esc;
                pen_bg = c.bg;
            }
            have_pen = true;
            term_put_utf8(out, c.ch);

            /* terminals disagree on some widths: never trust the cursor after non-ASCII */
            if (c.ch < 127) {
                cur_x = x + 1;
                cur_y = y;
            } else {
                cur_x = -1;
            }
        }
    }

    if (!out.empty()) {
        term_write(out);
    }
}
