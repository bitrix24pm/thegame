// platform.cpp — реализация окна/ввода/звука через SDL2
#include "platform.h"

bool Platform::init(const char* title, int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return false;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) return false;
    gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) return false;
    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1); // vsync
    width = w; height = h;
    return true;
}

void Platform::shutdown() {
    if (gl_ctx) SDL_GL_DeleteContext(gl_ctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Platform::swap() { SDL_GL_SwapWindow(window); }

bool Platform::poll_events(InputState& in) {
    // сброс edge-состояний
    for (int i = 0; i < SDL_SCANCODE_UNKNOWN; i++) in.key_pressed[i] = false;
    in.lmb_pressed = false;
    in.rmb_pressed = false;
    in.scroll = 0;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT: return false;
            case SDL_KEYDOWN:
                if (!e.key.repeat && e.key.keysym.scancode < SDL_SCANCODE_UNKNOWN)
                    in.key_pressed[e.key.keysym.scancode] = true;
                break;
            case SDL_MOUSEMOTION:
                in.mouse_x = e.motion.x; in.mouse_y = e.motion.y;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) { in.mouse_l = true; in.lmb_pressed = true; }
                if (e.button.button == SDL_BUTTON_RIGHT) { in.mouse_r = true; in.rmb_pressed = true; }
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) in.mouse_l = false;
                if (e.button.button == SDL_BUTTON_RIGHT) in.mouse_r = false;
                break;
            case SDL_MOUSEWHEEL: in.scroll = e.wheel.y; break;
        }
    }
    in.keys = SDL_GetKeyboardState(nullptr);
    return true;
}

// --- Аудио ---
static AudioCallbackFn g_audio_cb = nullptr;
static SDL_AudioDeviceID g_dev = 0;

static void sdl_audio_cb(void*, Uint8* stream, int len) {
    if (g_audio_cb) g_audio_cb((int16_t*)stream, len / sizeof(int16_t));
}

bool audio_init(AudioCallbackFn fn) {
    g_audio_cb = fn;
    SDL_AudioSpec want{}, have{};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = sdl_audio_cb;
    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!g_dev) return false;
    SDL_PauseAudioDevice(g_dev, 0);
    return true;
}

void audio_shutdown() {
    if (g_dev) SDL_CloseAudioDevice(g_dev);
    g_dev = 0;
}
