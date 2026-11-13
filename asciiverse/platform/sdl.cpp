/*
 * Asciiverse terminal edition: the SDL 1.2 calls Goblin Hack makes, with no
 * window and no video driver. Time is the monotonic clock, events come from
 * the terminal, surfaces are plain memory, and each SDL_GL_SwapBuffers (the
 * end of every frame the game draws) puts the frame on the terminal.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#include "ghack_sdl_shim.h"
#include "terminal.h"

#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SDL_EventFilter     sdl_filter;
static std::deque<SDL_Event> sdl_queue;
static SDL_Surface        *sdl_video;
static char                sdl_error[256];
static uint64_t            sdl_start_ms;

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

extern "C" {

int SDL_Init(Uint32 flags)
{
    (void) flags;
    if (!sdl_start_ms) {
        sdl_start_ms = now_ms();
    }
    terminal_start();
    return 0;
}

void SDL_Quit(void)
{
    terminal_fini();
}

char *SDL_GetError(void) { return sdl_error; }
void  SDL_ClearError(void) { sdl_error[0] = 0; }

/*
 * Surfaces: 32 bits a pixel, RGBA in memory order, as the game asks for.
 */
static SDL_PixelFormat *make_format(int bpp, Uint32 r, Uint32 g, Uint32 b, Uint32 a)
{
    SDL_PixelFormat *f = (SDL_PixelFormat *) calloc(1, sizeof(*f));
    f->BitsPerPixel  = (Uint8) (bpp ? bpp : 32);
    f->BytesPerPixel = (Uint8) ((f->BitsPerPixel + 7) / 8);
    f->Rmask = r; f->Gmask = g; f->Bmask = b; f->Amask = a;
    Uint32 m[4] = { r, g, b, a };
    Uint8 *shift[4] = { &f->Rshift, &f->Gshift, &f->Bshift, &f->Ashift };
    for (int i = 0; i < 4; i++) {
        Uint8 s = 0;
        if (m[i]) {
            while (!((m[i] >> s) & 1)) {
                s++;
            }
        }
        *shift[i] = s;
    }
    f->alpha = 255;
    return f;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
                                  Uint32 r, Uint32 g, Uint32 b, Uint32 a)
{
    if (w < 0 || h < 0) {
        snprintf(sdl_error, sizeof(sdl_error), "bad surface size %dx%d", w, h);
        return NULL;
    }
    SDL_Surface *s = (SDL_Surface *) calloc(1, sizeof(*s));
    if (!depth) {
        depth = 32;
    }
    if (!r && !g && !b && !a && depth == 32) {
        r = 0x000000ff; g = 0x0000ff00; b = 0x00ff0000; a = 0xff000000;
    }
    s->flags  = flags;
    s->format = make_format(depth, r, g, b, a);
    s->w      = w;
    s->h      = h;
    s->pitch  = (Uint16) (w * s->format->BytesPerPixel);
    s->pixels = calloc((size_t) (s->pitch ? s->pitch : 1), (size_t) (h ? h : 1));
    s->clip_rect.w = (Uint16) w;
    s->clip_rect.h = (Uint16) h;
    s->refcount = 1;
    return s;
}

void SDL_FreeSurface(SDL_Surface *s)
{
    if (!s || s == sdl_video) {
        return;
    }
    free(s->pixels);
    free(s->format);
    free(s);
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags)
{
    if (!src) {
        return NULL;
    }
    SDL_Surface *d = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
                                          fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
    SDL_BlitSurface(src, NULL, d, NULL);
    return d;
}

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr)
{
    if (!src || !dst || !src->pixels || !dst->pixels) {
        return -1;
    }
    int sx = sr ? sr->x : 0, sy = sr ? sr->y : 0;
    int w  = sr ? sr->w : src->w, h = sr ? sr->h : src->h;
    int dx = dr ? dr->x : 0, dy = dr ? dr->y : 0;
    int bpp = src->format->BytesPerPixel;
    if (bpp != dst->format->BytesPerPixel) {
        return 0;
    }
    for (int y = 0; y < h; y++) {
        if (sy + y < 0 || sy + y >= src->h || dy + y < 0 || dy + y >= dst->h) {
            continue;
        }
        int x0 = 0, x1 = w;
        if (sx + x0 < 0) x0 = -sx;
        if (dx + x0 < 0) x0 = -dx;
        if (sx + x1 > src->w) x1 = src->w - sx;
        if (dx + x1 > dst->w) x1 = dst->w - dx;
        if (x1 <= x0) {
            continue;
        }
        memcpy((Uint8 *) dst->pixels + (dy + y) * dst->pitch + (dx + x0) * bpp,
               (Uint8 *) src->pixels + (sy + y) * src->pitch + (sx + x0) * bpp,
               (size_t) (x1 - x0) * bpp);
    }
    return 0;
}

int  SDL_LockSurface(SDL_Surface *) { return 0; }
void SDL_UnlockSurface(SDL_Surface *) {}

/*
 * The "window": a fixed 1024x800 surface the game lays its screens out on.
 * Nothing is ever drawn into it; the terminal shows the game instead.
 */
SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags)
{
    if (!sdl_video) {
        sdl_video = SDL_CreateRGBSurface(flags, 1, 1, 32, 0, 0, 0, 0);
    }
    sdl_video->w     = w ? w : 1024;
    sdl_video->h     = h ? h : 800;
    sdl_video->flags = flags;
    (void) bpp;
    return sdl_video;
}

SDL_Surface *SDL_GetVideoSurface(void)
{
    if (!sdl_video) {
        SDL_SetVideoMode(1024, 800, 32, SDL_OPENGL);
    }
    return sdl_video;
}

void SDL_GL_SwapBuffers(void) { terminal_frame(); }
int  SDL_GL_SetAttribute(SDL_GLattr, int) { return 0; }
int  SDL_GL_GetAttribute(SDL_GLattr, int *v) { if (v) *v = 0; return 0; }
void *SDL_GL_GetProcAddress(const char *) { return NULL; }
void SDL_WM_SetCaption(const char *, const char *) {}
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode m) { return m == SDL_GRAB_QUERY ? SDL_GRAB_OFF : m; }
int  SDL_GetWMInfo(SDL_SysWMinfo *) { return 0; }
int  SDL_ShowCursor(int) { return 0; }

Uint32 SDL_GetTicks(void)
{
    if (!sdl_start_ms) {
        sdl_start_ms = now_ms();
    }
    return (Uint32) (now_ms() - sdl_start_ms);
}

/*
 * The game sleeps a few ms a frame; spend it waiting on the terminal, so a
 * key is read as soon as it is typed.
 */
void SDL_Delay(Uint32 ms)
{
    terminal_wait((int) ms);
    terminal_poll();
}

void SDL_PumpEvents(void) { terminal_poll(); }

int SDL_PushEvent(SDL_Event *e)
{
    if (sdl_filter && !sdl_filter(e)) {
        return 0;
    }
    if (sdl_queue.size() < 1024) {
        sdl_queue.push_back(*e);
    }
    return 1;
}

int SDL_PeepEvents(SDL_Event *events, int n, SDL_eventaction action, Uint32 mask)
{
    if (action == SDL_ADDEVENT) {
        for (int i = 0; i < n; i++) {
            SDL_PushEvent(&events[i]);
        }
        return n;
    }
    terminal_poll();
    int found = 0;
    for (std::deque<SDL_Event>::iterator it = sdl_queue.begin();
         it != sdl_queue.end() && found < n;) {
        if (mask & SDL_EVENTMASK(it->type)) {
            events[found++] = *it;
            if (action == SDL_GETEVENT) {
                it = sdl_queue.erase(it);
                continue;
            }
        }
        ++it;
    }
    return found;
}

void SDL_SetEventFilter(SDL_EventFilter f) { sdl_filter = f; }

char *SDL_GetKeyName(SDLKey key)
{
    static char name[32];
    switch (key) {
    case SDLK_RETURN:    return (char *) "return";
    case SDLK_ESCAPE:    return (char *) "escape";
    case SDLK_TAB:       return (char *) "tab";
    case SDLK_BACKSPACE: return (char *) "backspace";
    case SDLK_SPACE:     return (char *) "space";
    case SDLK_UP:        return (char *) "up";
    case SDLK_DOWN:      return (char *) "down";
    case SDLK_LEFT:      return (char *) "left";
    case SDLK_RIGHT:     return (char *) "right";
    case SDLK_HOME:      return (char *) "home";
    case SDLK_END:       return (char *) "end";
    case SDLK_PAGEUP:    return (char *) "page up";
    case SDLK_PAGEDOWN:  return (char *) "page down";
    case SDLK_DELETE:    return (char *) "delete";
    default: break;
    }
    if (key >= SDLK_F1 && key <= SDLK_F15) {
        snprintf(name, sizeof(name), "f%d", key - SDLK_F1 + 1);
    } else if (key >= SDLK_KP0 && key <= SDLK_KP9) {
        snprintf(name, sizeof(name), "[%d]", key - SDLK_KP0);
    } else if (key > 32 && key < 127) {
        snprintf(name, sizeof(name), "%c", key);
    } else {
        snprintf(name, sizeof(name), "unknown key");
    }
    return name;
}

SDL_Thread *SDL_CreateThread(int (*)(void *), void *) { return NULL; }
void SDL_WaitThread(SDL_Thread *, int *) {}

/*
 * The game only reads its two console font images, for their size: return
 * a blank surface of the PNG's size.
 */
SDL_Surface *IMG_Load(const char *file)
{
    FILE *f = fopen(file, "rb");
    if (!f) {
        snprintf(sdl_error, sizeof(sdl_error), "can't open %s", file);
        return NULL;
    }
    unsigned char hdr[24];
    size_t n = fread(hdr, 1, sizeof(hdr), f);
    fclose(f);
    int w = 16, h = 16;
    if (n == sizeof(hdr) && hdr[1] == 'P' && hdr[2] == 'N' && hdr[3] == 'G') {
        w = (hdr[16] << 24) | (hdr[17] << 16) | (hdr[18] << 8) | hdr[19];
        h = (hdr[20] << 24) | (hdr[21] << 16) | (hdr[22] << 8) | hdr[23];
    }
    return SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
}

} /* extern "C" */
