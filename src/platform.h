// platform.h — абстракция окна/ввода (SDL2). Только контекст + ввод + аудио-колбэк.
#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

struct InputState {
    const uint8_t* keys = nullptr;   // SDL keyboard state
    int mouse_x = 0, mouse_y = 0;
    bool mouse_l = false, mouse_r = false;
    bool lmb_pressed = false;        // edge
    bool rmb_pressed = false;
    bool key_pressed[SDL_SCANCODE_UNKNOWN]; // edge-события этого кадра
    float scroll = 0;
};

class Platform {
public:
    bool init(const char* title, int w, int h);
    void shutdown();
    void swap();
    bool poll_events(InputState& in); // false = закрыть окно
    SDL_Window* window = nullptr;
    SDL_GLContext gl_ctx = nullptr;
    int width = 1280, height = 720;
    static void* get_proc(const char* name) { return SDL_GL_GetProcAddress(name); }
};

// Аудио-колбэк: указатель на микшер
typedef void (*AudioCallbackFn)(int16_t* stream, int samples);
bool audio_init(AudioCallbackFn fn);
void audio_shutdown();
