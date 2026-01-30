/*
 *  This file is part of 'Johnny Reborn'
 *  Platform implementation using SDL 1.2 for Windows rendering/audio
 */

#ifdef PLATFORM_SDL12

#include "platform.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* lastError = "";
static uint32 startTicks = 0;

struct PlatformSurface {
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
    uint8* pixels;
    uint8 hasColorKey;
    uint8 colorKeyR, colorKeyG, colorKeyB;
    PlatformRect clipRect;
    int ownPixels;
};

struct PlatformWindow {
    SDL_Surface* screen;
    PlatformSurface* surface;
    int width;
    int height;
    int isFullscreen;
};

static PlatformWindow* mainWindow = NULL;

static int ensureVideoInitialized(void) {
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            lastError = SDL_GetError();
            return -1;
        }
    }
    if (!(SDL_WasInit(SDL_INIT_TIMER) & SDL_INIT_TIMER)) {
        if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0) {
            lastError = SDL_GetError();
            return -1;
        }
    }
    return 0;
}

int platformInit(void) {
    if (SDL_WasInit(0) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
            lastError = SDL_GetError();
            return -1;
        }
    } else if (ensureVideoInitialized() < 0) {
        return -1;
    }

    startTicks = SDL_GetTicks();
    return 0;
}

void platformShutdown(void) {
    SDL_Quit();
    mainWindow = NULL;
}

PlatformWindow* platformCreateWindow(const char* title, int width, int height, int fullscreen) {
    if (ensureVideoInitialized() < 0) {
        return NULL;
    }

    PlatformWindow* window = (PlatformWindow*)malloc(sizeof(PlatformWindow));
    if (!window) {
        lastError = "Out of memory";
        return NULL;
    }

    int flags = SDL_SWSURFACE;
    if (fullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    SDL_WM_SetCaption(title, NULL);
    SDL_Surface* screen = SDL_SetVideoMode(width, height, 32, flags);
    if (!screen) {
        lastError = SDL_GetError();
        free(window);
        return NULL;
    }

    window->screen = screen;
    window->surface = platformCreateSurface(width, height);
    if (!window->surface) {
        SDL_FreeSurface(screen);
        free(window);
        lastError = "Failed to create window surface";
        return NULL;
    }

    window->width = width;
    window->height = height;
    window->isFullscreen = fullscreen ? 1 : 0;

    SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);

    mainWindow = window;
    return window;
}

void platformDestroyWindow(PlatformWindow* window) {
    if (!window) {
        return;
    }

    if (window->surface) {
        platformFreeSurface(window->surface);
    }

    if (mainWindow == window) {
        mainWindow = NULL;
    }

    free(window);
}

void platformShowCursor(int show) {
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void platformToggleFullscreen(PlatformWindow* window) {
    if (!window) {
        return;
    }

    int flags = SDL_SWSURFACE;
    if (!window->isFullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    SDL_Surface* newScreen = SDL_SetVideoMode(window->width, window->height, 32, flags);
    if (!newScreen) {
        lastError = SDL_GetError();
        return;
    }

    window->screen = newScreen;
    window->isFullscreen = !window->isFullscreen;
    SDL_ShowCursor(window->isFullscreen ? SDL_DISABLE : SDL_ENABLE);
}

void platformUpdateWindow(PlatformWindow* window) {
    if (!window || !window->surface || !window->screen) {
        return;
    }

    SDL_Surface* screen = window->screen;
    PlatformSurface* surface = window->surface;

    if (SDL_MUSTLOCK(screen)) {
        if (SDL_LockSurface(screen) != 0) {
            lastError = SDL_GetError();
            return;
        }
    }

    int rowBytes = surface->width * surface->bytesPerPixel;
    uint8* dst = (uint8*)screen->pixels;
    uint8* src = surface->pixels;

    for (int y = 0; y < surface->height; y++) {
        memcpy(dst + y * screen->pitch, src + y * surface->pitch, rowBytes);
    }

    if (SDL_MUSTLOCK(screen)) {
        SDL_UnlockSurface(screen);
    }

    SDL_Flip(screen);
}

PlatformSurface* platformGetWindowSurface(PlatformWindow* window) {
    return window ? window->surface : NULL;
}

PlatformSurface* platformCreateSurface(int width, int height) {
    PlatformSurface* surface = (PlatformSurface*)malloc(sizeof(PlatformSurface));
    if (!surface) {
        return NULL;
    }

    surface->width = width;
    surface->height = height;
    surface->bytesPerPixel = 4;
    surface->pitch = width * 4;
    surface->pixels = (uint8*)calloc(width * height, 4);
    surface->hasColorKey = 0;
    surface->clipRect.x = 0;
    surface->clipRect.y = 0;
    surface->clipRect.w = width;
    surface->clipRect.h = height;
    surface->ownPixels = 1;
    return surface;
}

PlatformSurface* platformCreateSurfaceFrom(void* pixels, int width, int height, int pitch) {
    PlatformSurface* surface = (PlatformSurface*)malloc(sizeof(PlatformSurface));
    if (!surface) {
        return NULL;
    }

    surface->width = width;
    surface->height = height;
    surface->bytesPerPixel = 4;
    surface->pitch = pitch;
    surface->pixels = (uint8*)pixels;
    surface->hasColorKey = 0;
    surface->clipRect.x = 0;
    surface->clipRect.y = 0;
    surface->clipRect.w = width;
    surface->clipRect.h = height;
    surface->ownPixels = 0;
    return surface;
}

void platformFreeSurface(PlatformSurface* surface) {
    if (!surface) {
        return;
    }

    if (surface->ownPixels && surface->pixels) {
        free(surface->pixels);
    }
    free(surface);
}

void platformLockSurface(PlatformSurface* surface) {
    (void)surface;
}

void platformUnlockSurface(PlatformSurface* surface) {
    (void)surface;
}

void platformBlitSurface(PlatformSurface* src, PlatformRect* srcRect,
                        PlatformSurface* dst, PlatformRect* dstRect) {
    if (!src || !dst || !src->pixels || !dst->pixels) {
        return;
    }

    int srcX = srcRect ? srcRect->x : 0;
    int srcY = srcRect ? srcRect->y : 0;
    int srcW = srcRect ? srcRect->w : src->width;
    int srcH = srcRect ? srcRect->h : src->height;

    int dstX = dstRect ? dstRect->x : 0;
    int dstY = dstRect ? dstRect->y : 0;

    if (dstX < dst->clipRect.x) {
        srcX += dst->clipRect.x - dstX;
        srcW -= dst->clipRect.x - dstX;
        dstX = dst->clipRect.x;
    }
    if (dstY < dst->clipRect.y) {
        srcY += dst->clipRect.y - dstY;
        srcH -= dst->clipRect.y - dstY;
        dstY = dst->clipRect.y;
    }
    if (dstX + srcW > dst->clipRect.x + dst->clipRect.w) {
        srcW = dst->clipRect.x + dst->clipRect.w - dstX;
    }
    if (dstY + srcH > dst->clipRect.y + dst->clipRect.h) {
        srcH = dst->clipRect.y + dst->clipRect.h - dstY;
    }

    if (srcW <= 0 || srcH <= 0) {
        return;
    }

    for (int y = 0; y < srcH; y++) {
        for (int x = 0; x < srcW; x++) {
            int sx = srcX + x;
            int sy = srcY + y;
            int dx = dstX + x;
            int dy = dstY + y;

            if (sx < 0 || sy < 0 || sx >= src->width || sy >= src->height) {
                continue;
            }
            if (dx < 0 || dy < 0 || dx >= dst->width || dy >= dst->height) {
                continue;
            }

            uint8* srcPixel = src->pixels + sy * src->pitch + sx * src->bytesPerPixel;
            uint8* dstPixel = dst->pixels + dy * dst->pitch + dx * dst->bytesPerPixel;

            if (src->hasColorKey) {
                if (srcPixel[0] == src->colorKeyB &&
                    srcPixel[1] == src->colorKeyG &&
                    srcPixel[2] == src->colorKeyR) {
                    continue;
                }
            }

            memcpy(dstPixel, srcPixel, 4);
        }
    }
}

void platformFillRect(PlatformSurface* surface, PlatformRect* rect,
                     uint8 r, uint8 g, uint8 b, uint8 a) {
    if (!surface || !surface->pixels) {
        return;
    }

    int x = rect ? rect->x : 0;
    int y = rect ? rect->y : 0;
    int w = rect ? rect->w : surface->width;
    int h = rect ? rect->h : surface->height;

    for (int py = y; py < y + h && py < surface->height; py++) {
        for (int px = x; px < x + w && px < surface->width; px++) {
            uint8* pixel = surface->pixels + py * surface->pitch + px * surface->bytesPerPixel;
            pixel[0] = b;
            pixel[1] = g;
            pixel[2] = r;
            pixel[3] = a;
        }
    }
}

void platformSetColorKey(PlatformSurface* surface, uint8 r, uint8 g, uint8 b) {
    if (!surface) {
        return;
    }

    surface->hasColorKey = 1;
    surface->colorKeyR = r;
    surface->colorKeyG = g;
    surface->colorKeyB = b;
}

void platformSetClipRect(PlatformSurface* surface, PlatformRect* rect) {
    if (!surface) {
        return;
    }

    if (rect) {
        surface->clipRect = *rect;
    } else {
        surface->clipRect.x = 0;
        surface->clipRect.y = 0;
        surface->clipRect.w = surface->width;
        surface->clipRect.h = surface->height;
    }
}

void platformGetClipRect(PlatformSurface* surface, PlatformRect* rect) {
    if (surface && rect) {
        *rect = surface->clipRect;
    }
}

uint32 platformMapRGB(PlatformSurface* surface, uint8 r, uint8 g, uint8 b) {
    (void)surface;
    return (r << 16) | (g << 8) | b;
}

uint8* platformGetSurfacePixels(PlatformSurface* surface) {
    return surface ? surface->pixels : NULL;
}

int platformGetSurfacePitch(PlatformSurface* surface) {
    return surface ? surface->pitch : 0;
}

int platformGetSurfaceWidth(PlatformSurface* surface) {
    return surface ? surface->width : 0;
}

int platformGetSurfaceHeight(PlatformSurface* surface) {
    return surface ? surface->height : 0;
}

int platformGetSurfaceBytesPerPixel(PlatformSurface* surface) {
    return surface ? surface->bytesPerPixel : 0;
}

int platformPollEvent(PlatformEvent* event) {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        switch (sdlEvent.type) {
            case SDL_QUIT:
                event->type = EVENT_QUIT;
                return 1;

            case SDL_KEYDOWN:
                event->type = EVENT_KEY_DOWN;
                event->data.key.modifiers = 0;
                if (sdlEvent.key.keysym.mod & KMOD_ALT) {
                    event->data.key.modifiers |= KEYMOD_LALT;
                }
                switch (sdlEvent.key.keysym.sym) {
                    case SDLK_SPACE: event->data.key.keycode = KEY_SPACE; break;
                    case SDLK_RETURN: event->data.key.keycode = KEY_RETURN; break;
                    case SDLK_ESCAPE: event->data.key.keycode = KEY_ESCAPE; break;
                    case SDLK_m: event->data.key.keycode = KEY_M; break;
                    default: event->data.key.keycode = KEY_UNKNOWN; break;
                }
                return 1;

            case SDL_KEYUP:
                event->type = EVENT_KEY_UP;
                return 1;

            case SDL_VIDEOEXPOSE:
                event->type = EVENT_WINDOW_REFRESH;
                return 1;

            default:
                break;
        }
    }

    return 0;
}

uint32 platformGetTicks(void) {
    return SDL_GetTicks() - startTicks;
}

void platformDelay(uint32 ms) {
    SDL_Delay(ms);
}

static int ensureAudioInitialized(void) {
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            lastError = SDL_GetError();
            return -1;
        }
    }
    return 0;
}

static struct {
    PlatformAudioCallback callback;
    void* userdata;
} audioContext = { NULL, NULL };

static int audioOpened = 0;

static void sdlAudioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    if (audioContext.callback) {
        audioContext.callback(audioContext.userdata, stream, len);
    } else {
        memset(stream, 0x80, len);
    }
}

int platformInitAudio(void) {
    return ensureAudioInitialized();
}

void platformCloseAudio(void) {
    if (audioOpened) {
        SDL_CloseAudio();
        audioOpened = 0;
        audioContext.callback = NULL;
        audioContext.userdata = NULL;
    }
}

int platformOpenAudio(PlatformAudioSpec* spec) {
    if (ensureAudioInitialized() < 0) {
        return -1;
    }

    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    memset(&desired, 0, sizeof(SDL_AudioSpec));

    desired.freq = spec->freq > 0 ? spec->freq : 11025;
    desired.channels = spec->channels > 0 ? spec->channels : 1;
    desired.samples = spec->samples > 0 ? spec->samples : 1024;

    Uint16 format = AUDIO_U8;
    if (spec->format == 16 || spec->format == AUDIO_S16LSB || spec->format == AUDIO_S16SYS) {
        format = AUDIO_S16LSB;
    }
    desired.format = format;

    audioContext.callback = spec->callback;
    audioContext.userdata = spec->userdata;
    desired.callback = sdlAudioCallback;
    desired.userdata = &audioContext;

    if (SDL_OpenAudio(&desired, &obtained) != 0) {
        lastError = SDL_GetError();
        audioContext.callback = NULL;
        audioContext.userdata = NULL;
        return -1;
    }

    spec->freq = obtained.freq;
    spec->channels = obtained.channels;
    spec->samples = obtained.samples;
    spec->format = obtained.format;

    audioOpened = 1;
    return 0;
}

void platformPauseAudio(int pause) {
    if (audioOpened) {
        SDL_PauseAudio(pause ? 1 : 0);
    }
}

void platformLockAudio(void) {
    if (audioOpened) {
        SDL_LockAudio();
    }
}

void platformUnlockAudio(void) {
    if (audioOpened) {
        SDL_UnlockAudio();
    }
}

int platformLoadWAV(const char* filename, PlatformAudioSpec* spec,
                    uint8** audio_buf, uint32* audio_len) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        lastError = "Failed to open WAV file";
        return -1;
    }

    uint8 header[44];
    if (fread(header, 1, 44, file) != 44) {
        fclose(file);
        lastError = "Invalid WAV file";
        return -1;
    }

    uint32 dataSize = *(uint32*)(header + 40);
    *audio_len = dataSize;
    *audio_buf = (uint8*)malloc(dataSize);
    if (!*audio_buf) {
        fclose(file);
        lastError = "Out of memory";
        return -1;
    }

    if (fread(*audio_buf, 1, dataSize, file) != dataSize) {
        free(*audio_buf);
        fclose(file);
        lastError = "Failed to read WAV data";
        return -1;
    }

    fclose(file);

    spec->freq = *(uint32*)(header + 24);
    spec->channels = *(uint16*)(header + 22);
    spec->format = *(uint16*)(header + 34);

    return 0;
}

void platformFreeWAV(uint8* audio_buf) {
    free(audio_buf);
}

const char* platformGetError(void) {
    return lastError;
}

#endif /* PLATFORM_SDL12 */
