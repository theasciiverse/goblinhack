/*
 * Asciiverse terminal edition: the part of the SDL 1.2 API Goblin Hack uses,
 * implemented without any window, video driver or OpenGL. Events come from
 * the terminal (asciiverse/platform/terminal.cpp), surfaces are plain memory,
 * and sound goes to SDL2_mixer through audio.cpp when built with sound.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#ifndef GHACK_SDL_SHIM_H
#define GHACK_SDL_SHIM_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int16_t  Sint16;
typedef int32_t  Sint32;

#define SDL_INIT_TIMER       0x00000001
#define SDL_INIT_AUDIO       0x00000010
#define SDL_INIT_VIDEO       0x00000020
#define SDL_INIT_NOPARACHUTE 0x00100000

#define SDL_SWSURFACE  0x00000000
#define SDL_HWSURFACE  0x00000001
#define SDL_SRCALPHA   0x00010000
#define SDL_OPENGL     0x00000002
#define SDL_NOFRAME    0x00000020
#define SDL_FULLSCREEN 0x80000000

#define SDL_MUSTLOCK(s) 0

typedef struct SDL_Color { Uint8 r, g, b, unused; } SDL_Color;
typedef struct SDL_Palette { int ncolors; SDL_Color *colors; } SDL_Palette;

typedef struct SDL_PixelFormat {
    SDL_Palette *palette;
    Uint8  BitsPerPixel;
    Uint8  BytesPerPixel;
    Uint8  Rloss, Gloss, Bloss, Aloss;
    Uint8  Rshift, Gshift, Bshift, Ashift;
    Uint32 Rmask, Gmask, Bmask, Amask;
    Uint32 colorkey;
    Uint8  alpha;
} SDL_PixelFormat;

typedef struct SDL_Rect { Sint16 x, y; Uint16 w, h; } SDL_Rect;

typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    Uint16 pitch;
    void *pixels;
    int offset;
    SDL_Rect clip_rect;
    int refcount;
} SDL_Surface;

typedef enum {
    SDLK_UNKNOWN = 0, SDLK_FIRST = 0, SDLK_BACKSPACE = 8, SDLK_TAB = 9, SDLK_CLEAR = 12,
    SDLK_RETURN = 13, SDLK_PAUSE = 19, SDLK_ESCAPE = 27, SDLK_SPACE = 32,
    SDLK_EXCLAIM = 33, SDLK_QUOTEDBL = 34, SDLK_HASH = 35, SDLK_DOLLAR = 36,
    SDLK_AMPERSAND = 38, SDLK_QUOTE = 39, SDLK_LEFTPAREN = 40,
    SDLK_RIGHTPAREN = 41, SDLK_ASTERISK = 42, SDLK_PLUS = 43, SDLK_COMMA = 44,
    SDLK_MINUS = 45, SDLK_PERIOD = 46, SDLK_SLASH = 47,
    SDLK_0 = 48, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
    SDLK_8, SDLK_9,
    SDLK_COLON = 58, SDLK_SEMICOLON = 59, SDLK_LESS = 60, SDLK_EQUALS = 61,
    SDLK_GREATER = 62, SDLK_QUESTION = 63, SDLK_AT = 64,
    SDLK_LEFTBRACKET = 91, SDLK_BACKSLASH = 92, SDLK_RIGHTBRACKET = 93,
    SDLK_CARET = 94, SDLK_UNDERSCORE = 95, SDLK_BACKQUOTE = 96,
    SDLK_a = 97, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f, SDLK_g, SDLK_h,
    SDLK_i, SDLK_j, SDLK_k, SDLK_l, SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q,
    SDLK_r, SDLK_s, SDLK_t, SDLK_u, SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z,
    SDLK_DELETE = 127,
    SDLK_WORLD_0 = 160,
    SDLK_WORLD_1 = 161,
    SDLK_WORLD_2 = 162,
    SDLK_WORLD_3 = 163,
    SDLK_WORLD_4 = 164,
    SDLK_WORLD_5 = 165,
    SDLK_WORLD_6 = 166,
    SDLK_WORLD_7 = 167,
    SDLK_WORLD_8 = 168,
    SDLK_WORLD_9 = 169,
    SDLK_WORLD_10 = 170,
    SDLK_WORLD_11 = 171,
    SDLK_WORLD_12 = 172,
    SDLK_WORLD_13 = 173,
    SDLK_WORLD_14 = 174,
    SDLK_WORLD_15 = 175,
    SDLK_WORLD_16 = 176,
    SDLK_WORLD_17 = 177,
    SDLK_WORLD_18 = 178,
    SDLK_WORLD_19 = 179,
    SDLK_WORLD_20 = 180,
    SDLK_WORLD_21 = 181,
    SDLK_WORLD_22 = 182,
    SDLK_WORLD_23 = 183,
    SDLK_WORLD_24 = 184,
    SDLK_WORLD_25 = 185,
    SDLK_WORLD_26 = 186,
    SDLK_WORLD_27 = 187,
    SDLK_WORLD_28 = 188,
    SDLK_WORLD_29 = 189,
    SDLK_WORLD_30 = 190,
    SDLK_WORLD_31 = 191,
    SDLK_WORLD_32 = 192,
    SDLK_WORLD_33 = 193,
    SDLK_WORLD_34 = 194,
    SDLK_WORLD_35 = 195,
    SDLK_WORLD_36 = 196,
    SDLK_WORLD_37 = 197,
    SDLK_WORLD_38 = 198,
    SDLK_WORLD_39 = 199,
    SDLK_WORLD_40 = 200,
    SDLK_WORLD_41 = 201,
    SDLK_WORLD_42 = 202,
    SDLK_WORLD_43 = 203,
    SDLK_WORLD_44 = 204,
    SDLK_WORLD_45 = 205,
    SDLK_WORLD_46 = 206,
    SDLK_WORLD_47 = 207,
    SDLK_WORLD_48 = 208,
    SDLK_WORLD_49 = 209,
    SDLK_WORLD_50 = 210,
    SDLK_WORLD_51 = 211,
    SDLK_WORLD_52 = 212,
    SDLK_WORLD_53 = 213,
    SDLK_WORLD_54 = 214,
    SDLK_WORLD_55 = 215,
    SDLK_WORLD_56 = 216,
    SDLK_WORLD_57 = 217,
    SDLK_WORLD_58 = 218,
    SDLK_WORLD_59 = 219,
    SDLK_WORLD_60 = 220,
    SDLK_WORLD_61 = 221,
    SDLK_WORLD_62 = 222,
    SDLK_WORLD_63 = 223,
    SDLK_WORLD_64 = 224,
    SDLK_WORLD_65 = 225,
    SDLK_WORLD_66 = 226,
    SDLK_WORLD_67 = 227,
    SDLK_WORLD_68 = 228,
    SDLK_WORLD_69 = 229,
    SDLK_WORLD_70 = 230,
    SDLK_WORLD_71 = 231,
    SDLK_WORLD_72 = 232,
    SDLK_WORLD_73 = 233,
    SDLK_WORLD_74 = 234,
    SDLK_WORLD_75 = 235,
    SDLK_WORLD_76 = 236,
    SDLK_WORLD_77 = 237,
    SDLK_WORLD_78 = 238,
    SDLK_WORLD_79 = 239,
    SDLK_WORLD_80 = 240,
    SDLK_WORLD_81 = 241,
    SDLK_WORLD_82 = 242,
    SDLK_WORLD_83 = 243,
    SDLK_WORLD_84 = 244,
    SDLK_WORLD_85 = 245,
    SDLK_WORLD_86 = 246,
    SDLK_WORLD_87 = 247,
    SDLK_WORLD_88 = 248,
    SDLK_WORLD_89 = 249,
    SDLK_WORLD_90 = 250,
    SDLK_WORLD_91 = 251,
    SDLK_WORLD_92 = 252,
    SDLK_WORLD_93 = 253,
    SDLK_WORLD_94 = 254,
    SDLK_WORLD_95 = 255,
    SDLK_KP0 = 256, SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5,
    SDLK_KP6, SDLK_KP7, SDLK_KP8, SDLK_KP9, SDLK_KP_PERIOD, SDLK_KP_DIVIDE,
    SDLK_KP_MULTIPLY, SDLK_KP_MINUS, SDLK_KP_PLUS, SDLK_KP_ENTER,
    SDLK_KP_EQUALS,
    SDLK_UP = 273, SDLK_DOWN, SDLK_RIGHT, SDLK_LEFT, SDLK_INSERT, SDLK_HOME,
    SDLK_END, SDLK_PAGEUP, SDLK_PAGEDOWN,
    SDLK_F1 = 282, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7,
    SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14,
    SDLK_F15,
    SDLK_NUMLOCK = 300, SDLK_CAPSLOCK, SDLK_SCROLLOCK, SDLK_RSHIFT,
    SDLK_LSHIFT, SDLK_RCTRL, SDLK_LCTRL, SDLK_RALT, SDLK_LALT, SDLK_RMETA, SDLK_LMETA, SDLK_LSUPER, SDLK_RSUPER,
    SDLK_MODE, SDLK_COMPOSE, SDLK_HELP, SDLK_PRINT, SDLK_SYSREQ, SDLK_BREAK,
    SDLK_MENU, SDLK_POWER, SDLK_EURO, SDLK_UNDO,
    SDLK_LAST = 323
} SDLKey;

typedef enum {
    KMOD_NONE = 0x0000, KMOD_LSHIFT = 0x0001, KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040, KMOD_RCTRL = 0x0080, KMOD_LALT = 0x0100,
    KMOD_RALT = 0x0200, KMOD_LMETA = 0x0400, KMOD_RMETA = 0x0800,
    KMOD_NUM = 0x1000, KMOD_CAPS = 0x2000, KMOD_MODE = 0x4000
} SDLMod;
#define KMOD_CTRL  (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_ALT   (KMOD_LALT | KMOD_RALT)

typedef struct SDL_keysym {
    Uint8  scancode;
    SDLKey sym;
    SDLMod mod;
    Uint16 unicode;
} SDL_keysym;

#define SDL_RELEASED 0
#define SDL_PRESSED  1

enum {
    SDL_NOEVENT = 0, SDL_ACTIVEEVENT, SDL_KEYDOWN, SDL_KEYUP, SDL_MOUSEMOTION,
    SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_JOYAXISMOTION,
    SDL_JOYBALLMOTION, SDL_JOYHATMOTION, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP,
    SDL_QUIT, SDL_SYSWMEVENT, SDL_VIDEORESIZE = 16, SDL_VIDEOEXPOSE = 17,
    SDL_NUMEVENTS = 32
};
#define SDL_EVENTMASK(X)        (1u << (X))
#define SDL_KEYDOWNMASK         SDL_EVENTMASK(SDL_KEYDOWN)
#define SDL_KEYUPMASK           SDL_EVENTMASK(SDL_KEYUP)
#define SDL_MOUSEMOTIONMASK     SDL_EVENTMASK(SDL_MOUSEMOTION)
#define SDL_MOUSEBUTTONDOWNMASK SDL_EVENTMASK(SDL_MOUSEBUTTONDOWN)
#define SDL_MOUSEBUTTONUPMASK   SDL_EVENTMASK(SDL_MOUSEBUTTONUP)
#define SDL_QUITMASK            SDL_EVENTMASK(SDL_QUIT)
#define SDL_ALLEVENTS           0xFFFFFFFFu

#define SDL_BUTTON_LEFT      1
#define SDL_BUTTON_MIDDLE    2
#define SDL_BUTTON_RIGHT     3
#define SDL_BUTTON_WHEELUP   4
#define SDL_BUTTON_WHEELDOWN 5
#define SDL_BUTTON(X)        (1u << ((X) - 1))

typedef struct SDL_KeyboardEvent { Uint8 type, which, state; SDL_keysym keysym; } SDL_KeyboardEvent;
typedef struct SDL_MouseMotionEvent { Uint8 type, which, state; Uint16 x, y; Sint16 xrel, yrel; } SDL_MouseMotionEvent;
typedef struct SDL_MouseButtonEvent { Uint8 type, which, button, state; Uint16 x, y; } SDL_MouseButtonEvent;
typedef struct SDL_QuitEvent { Uint8 type; } SDL_QuitEvent;

typedef union SDL_Event {
    Uint8 type;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_QuitEvent quit;
} SDL_Event;

typedef enum { SDL_ADDEVENT, SDL_PEEKEVENT, SDL_GETEVENT } SDL_eventaction;
typedef int (*SDL_EventFilter)(const SDL_Event *event);

typedef enum { SDL_GRAB_QUERY = -1, SDL_GRAB_OFF = 0, SDL_GRAB_ON = 1 } SDL_GrabMode;

typedef enum {
    SDL_GL_RED_SIZE, SDL_GL_GREEN_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_ALPHA_SIZE,
    SDL_GL_BUFFER_SIZE, SDL_GL_DOUBLEBUFFER, SDL_GL_DEPTH_SIZE
} SDL_GLattr;

typedef struct SDL_version { Uint8 major, minor, patch; } SDL_version;
#define SDL_VERSION(X) do { (X)->major = 1; (X)->minor = 2; (X)->patch = 15; } while (0)

typedef enum { SDL_SYSWM_X11 } SDL_SYSWM_TYPE;
typedef struct SDL_SysWMinfo {
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    struct { struct { void *display; unsigned long wmwindow; void (*lock_func)(void); void (*unlock_func)(void); } x11; } info;
} SDL_SysWMinfo;
/* never reached: SDL_GetWMInfo reports no window system */
#define DefaultScreen(d)       0
#define DisplayWidth(d, s)     1024
#define DisplayHeight(d, s)    800
#define XMoveWindow(d, w, x, y) ((void) 0)

typedef struct SDL_Thread SDL_Thread;

#ifdef __cplusplus
extern "C" {
#endif

int          SDL_Init(Uint32 flags);
void         SDL_Quit(void);
char        *SDL_GetError(void);
void         SDL_ClearError(void);

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
SDL_Surface *SDL_GetVideoSurface(void);
void         SDL_GL_SwapBuffers(void);
int          SDL_GL_SetAttribute(SDL_GLattr attr, int value);
int          SDL_GL_GetAttribute(SDL_GLattr attr, int *value);
void        *SDL_GL_GetProcAddress(const char *proc);
void         SDL_WM_SetCaption(const char *title, const char *icon);
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode);
int          SDL_GetWMInfo(SDL_SysWMinfo *info);
int          SDL_ShowCursor(int toggle);

Uint32       SDL_GetTicks(void);
void         SDL_Delay(Uint32 ms);

void         SDL_PumpEvents(void);
int          SDL_PeepEvents(SDL_Event *events, int numevents, SDL_eventaction action, Uint32 mask);
int          SDL_PushEvent(SDL_Event *event);
void         SDL_SetEventFilter(SDL_EventFilter filter);
char        *SDL_GetKeyName(SDLKey key);

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                  Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
#define SDL_AllocSurface SDL_CreateRGBSurface
SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags);
int          SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
void         SDL_FreeSurface(SDL_Surface *surface);
int          SDL_LockSurface(SDL_Surface *surface);
void         SDL_UnlockSurface(SDL_Surface *surface);

SDL_Thread  *SDL_CreateThread(int (*fn)(void *), void *data);
void         SDL_WaitThread(SDL_Thread *thread, int *status);

/* SDL_image */
SDL_Surface *IMG_Load(const char *file);

#ifdef __cplusplus
}
#endif

#endif
