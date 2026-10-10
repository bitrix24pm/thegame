// renderer.h — OpenGL-рендер: шейдеры, батчинг, изометрия, HUD (битмап-шрифт)
#pragma once
#include "shader.h"
#include "camera.h"
#include "../math/vec3.h"
#include "../data/structs.h"
#include <cstdint>

constexpr int MAX_BATCH_QUADS = 8192;   // статичный буфер, без аллокаций в рантайме
constexpr int VERT_PER_QUAD = 6;        // 2 треугольника

// Вершина: pos(3) + color(4) = 7 float
struct BatchVertex {
    float x, y, z, r, g, b, a;
};

class Renderer {
public:
    bool init(int width, int height, void* (*getproc)(const char*));
    void shutdown();

    Camera camera;

    // --- Кадровый цикл ---
    void begin_frame(float time_sec);
    void end_frame();

    // --- Батчинг плоских квадов на полу (y задан) ---
    void quad(const vec3& center, float hw, float hh, uint32_t color, float alpha, float rot = 0.0f, float y = 0.01f);
    void line(const vec2& a, const vec2& b, float width, uint32_t color, float alpha);
    void circle_outline(const vec2& c, float radius, float width, uint32_t color, float alpha, int segs = 32);
    void cone(const vec2& origin, const vec2& dir, float range, float half_angle, uint32_t color, float alpha);
    void billboard_shadow(const vec2& pos, float size, float alpha); // тень-эллипс под сущностью

    // --- 3D-объекты (икосаэдр, процедурная геометрия) ---
    void mesh_icosahedron(const vec3& center, float scale, uint32_t color, float alpha, float spin);

    // --- Пол подземелья (один draw call, тайлмапа в текстуре 48x48) ---
    void draw_floor(const Dungeon& d, float time_sec, const vec2& player_pos);

    // --- HUD: битмап-шрифт 5x7, генерируется кодом ---
    void text(int x, int y, const char* s, uint32_t color, float scale = 2.0f);
    void hud_bar(int x, int y, int w, int h, float frac, uint32_t color);

    int screen_w = 1280, screen_h = 720;

private:
    Shader sh_batch;                 // один шейдер для всего (пол, квады, меши, HUD)
    GLint u_vp = -1;

    BatchVertex batch_data[MAX_BATCH_QUADS * VERT_PER_QUAD];
    int batch_count = 0;
    unsigned int vao = 0, vbo = 0;   // ID через glGenBuffers (legacy-контекст)

    mat4 hud_proj;                   // орто по пикселям экрана

    void flush_batch();
};
