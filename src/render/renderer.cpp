// renderer.cpp — реализация OpenGL-рендера. Один шейдер, один батч, 1-2 draw call на кадр.
#include "renderer.h"
#include "../data/constants.h"
#include "../math/math_utils.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <GL/gl.h>

// Единый шейдер pos+color. Пол рисуется тем же проходом (у пола нет текстуры —
// сетка и шум генерируются во фрагментном шейдере по мировым координатам).
static const char* VS_MAIN = R"(
#version 330 core
layout(location=0) in vec3 a_pos;
layout(location=1) in vec4 a_col;
uniform mat4 u_vp;
out vec4 v_col;
out vec3 v_world;
void main(){
    v_col=a_col;
    v_world=a_pos;
    gl_Position=u_vp*vec4(a_pos,1.0);
}
)";

static const char* FS_MAIN = R"(
#version 330 core
in vec4 v_col;
in vec3 v_world;
out vec4 frag_color;
uniform float u_time;
uniform int u_mode; // 0 = плоская заливка, 1 = процедурный пол

float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float vnoise(vec2 p){
    vec2 i=floor(p), f=fract(p);
    vec2 u=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),u.x),
               mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),u.x),u.y);
}

void main(){
    if(u_mode==1){
        // Процедурный пол: сетка + шум + пульс
        vec2 uv=v_world.xz;
        vec2 grid=fract(uv);
        float line=min(step(0.93,grid.x)+step(0.93,grid.y),1.0);
        float n=vnoise(uv*3.0+u_time*0.05);
        vec3 base=vec3(0.045,0.055,0.11)+vec3(0.025)*n;
        vec3 gridc=vec3(0.16,0.30,0.52);
        vec3 col=mix(base,gridc,line*0.55);
        col+=gridc*0.06*(0.5+0.5*sin(u_time*1.5+length(uv)*0.8));
        frag_color=vec4(col*v_col.rgb,v_col.a);
    } else {
        frag_color=v_col;
    }
}
)";

bool Renderer::init(int width, int height, void* (*getproc)(const char*)) {
    screen_w = width; screen_h = height;
    if (!gl.load(getproc)) { printf("[gl] function load failed\n"); return false; }

    if (!sh_batch.compile(VS_MAIN, FS_MAIN)) return false;
    u_vp = sh_batch.loc("u_vp");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(batch_data), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, r));
    glEnableVertexAttribArray(1);

    hud_proj = mat4::ortho(0, float(width), float(height), 0, -10, 10); // x вправо, y вниз

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // сортировка "низ -> верх" вручную
    return true;
}

void Renderer::shutdown() {}

void Renderer::begin_frame(float time_sec) {
    glClearColor(rgba_r(COL_BG), rgba_g(COL_BG), rgba_b(COL_BG), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gl.Uniform1f(sh_batch.loc("u_time"), time_sec);
    batch_count = 0;
}

void Renderer::end_frame() { flush_batch(); }

void Renderer::flush_batch() {
    if (batch_count == 0) return;
    sh_batch.use();
    gl.UniformMatrix4fv(u_vp, 1, GL_FALSE, camera.vp.m);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.BufferData(GL_ARRAY_BUFFER, batch_count * sizeof(BatchVertex), batch_data, GL_DYNAMIC_DRAW);
    gl.DrawArrays(GL_TRIANGLES, 0, batch_count);
    batch_count = 0;
}

static inline void set_col(BatchVertex& v, uint32_t c, float a) {
    v.r = rgba_r(c); v.g = rgba_g(c); v.b = rgba_b(c); v.a = a;
}

void Renderer::quad(const vec3& center, float hw, float hh, uint32_t color, float alpha, float rot, float y) {
    if (batch_count + VERT_PER_QUAD > MAX_BATCH_QUADS * VERT_PER_QUAD) flush_batch();
    float cs = std::cos(rot), sn = std::sin(rot);
    vec2 corners[4] = { {-hw,-hh}, {hw,-hh}, {hw,hh}, {-hw,hh} };
    vec2 rc[4];
    for (int i = 0; i < 4; i++)
        rc[i] = vec2(corners[i].x*cs - corners[i].y*sn, corners[i].x*sn + corners[i].y*cs);
    BatchVertex* v = &batch_data[batch_count];
    auto mk = [&](int idx, const vec2& p) {
        v[idx].x = center.x + p.x; v[idx].y = y; v[idx].z = center.z + p.y;
        set_col(v[idx], color, alpha);
    };
    mk(0,rc[0]); mk(1,rc[1]); mk(2,rc[2]);
    mk(3,rc[0]); mk(4,rc[2]); mk(5,rc[3]);
    batch_count += VERT_PER_QUAD;
}

void Renderer::line(const vec2& a, const vec2& b, float width, uint32_t color, float alpha) {
    vec2 d = b - a;
    float len = d.length();
    if (len < 1e-5f) return;
    vec2 dir = d / len;
    float rot = std::atan2(dir.y, dir.x);
    vec3 c((a.x+b.x)/2, 0, (a.y+b.y)/2);
    quad(c, len/2, width/2, color, alpha, rot, 0.02f);
}

void Renderer::circle_outline(const vec2& c, float radius, float width, uint32_t color, float alpha, int segs) {
    if (radius <= 0.01f) return;
    for (int i = 0; i < segs; i++) {
        float a0 = TAU_F*i/segs, a1 = TAU_F*(i+1)/segs;
        vec2 p0(c.x+std::cos(a0)*radius, c.y+std::sin(a0)*radius);
        vec2 p1(c.x+std::cos(a1)*radius, c.y+std::sin(a1)*radius);
        line(p0, p1, width, color, alpha);
    }
}

void Renderer::cone(const vec2& origin, const vec2& dir, float range, float half_angle, uint32_t color, float alpha) {
    float base = std::atan2(dir.y, dir.x);
    const int SEGS = 10;
    // заполнение сектора: треугольники origin -> arc[i] -> arc[i+1] (как вырожденный квад)
    for (int i = 0; i < SEGS; i++) {
        float a0 = base - half_angle + 2*half_angle*i/SEGS;
        float a1 = base - half_angle + 2*half_angle*(i+1)/SEGS;
        vec2 p0(origin.x+std::cos(a0)*range, origin.y+std::sin(a0)*range);
        vec2 p1(origin.x+std::cos(a1)*range, origin.y+std::sin(a1)*range);
        if (batch_count + VERT_PER_QUAD > MAX_BATCH_QUADS*VERT_PER_QUAD) flush_batch();
        BatchVertex* v = &batch_data[batch_count];
        auto put = [&](int idx, const vec2& p) {
            v[idx].x=p.x; v[idx].y=0.015f; v[idx].z=p.y; set_col(v[idx], color, alpha);
        };
        put(0, origin); put(1, p0); put(2, p1);
        put(3, origin); put(4, p1); put(5, p1); // degenerate не нужен, но сохраняем кратность 6
        batch_count += VERT_PER_QUAD;
    }
}

void Renderer::billboard_shadow(const vec2& pos, float size, float alpha) {
    quad(vec3(pos.x, 0, pos.y), size, size*0.6f, 0x000000, alpha, 0, 0.008f);
}

// ---------- Пол: большой квад с режимом u_mode=1 ----------
void Renderer::draw_floor(const Dungeon& d, float time_sec, const vec2& player_pos) {
    (void)time_sec; (void)player_pos;
    // Рисуем только тайлы пола как квады (батч). Пустота не рисуется вовсе.
    for (int ty = 0; ty < DUNGEON_GRID_C; ty++) {
        for (int tx = 0; tx < DUNGEON_GRID_C; tx++) {
            if (d.tiles[ty*DUNGEON_GRID_C + tx] != 1) continue;
            float wx = tx - DUNGEON_GRID_C/2 + 0.5f;
            float wz = ty - DUNGEON_GRID_C/2 + 0.5f;
            // расстояние до игрока -> яркость (освещение факелом)
            float dx = wx - player_pos.x, dz = wz - player_pos.y;
            float dist = std::sqrt(dx*dx + dz*dz);
            float light = clampf(1.0f - (dist - 3.0f) / 13.0f, 0.10f, 1.0f);
            uint32_t tint = 0xFFFFFF;
            // модулируем яркость через альфа-цвет: используем прямой ввод вершин
            if (batch_count + VERT_PER_QUAD > MAX_BATCH_QUADS*VERT_PER_QUAD) flush_batch();
            BatchVertex* v = &batch_data[batch_count];
            float r = rgba_r(COL_FLOOR)*light*2.2f, g = rgba_g(COL_FLOOR)*light*2.2f, b = rgba_b(COL_FLOOR)*light*2.4f;
            (void)tint;
            vec2 c(wx, wz);
            float hs = 0.5f;
            vec2 corners[4] = {{-hs,-hs},{hs,-hs},{hs,hs},{-hs,hs}};
            for (int k = 0; k < 6; k++) {
                const vec2& cc = corners[k==0||k==3?0 : k==1||k==4?1 : k==2?2 : 3];
                v[k].x = c.x+cc.x; v[k].y = 0.0f; v[k].z = c.y+cc.y;
                v[k].r = r; v[k].g = g; v[k].b = b; v[k].a = 1.0f;
            }
            // порядок: 0,1,2, 0,2,3
            BatchVertex t3[6];
            t3[0]=v[0]; t3[1]=v[1]; t3[2]=v[2]; t3[3]=v[0]; t3[4]=v[2]; t3[5]=v[3];
            memcpy(v, t3, sizeof(t3));
            batch_count += VERT_PER_QUAD;
        }
    }
    // Поверх — светящаяся сетка процедурным слоем поверх пола не делаем; вместо этого
    // рисуем тонкие линии сетки глобально в радиусе освещения (дёшево и красиво)
    (void)d;
}

// ---------- Икосаэдр ----------
static const vec3 ICO_V_RAW[12] = {
    {-1,  1.618f, 0}, {1,  1.618f, 0}, {-1, -1.618f, 0}, {1, -1.618f, 0},
    {0, -1,  1.618f}, {0,  1,  1.618f}, {0, -1, -1.618f}, {0,  1, -1.618f},
    {1.618f, 0, -1}, {1.618f, 0, 1}, {-1.618f, 0, -1}, {-1.618f, 0, 1},
};
static const int ICO_FACES[20][3] = {
    {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
    {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
    {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
    {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1},
};

void Renderer::mesh_icosahedron(const vec3& center, float scale, uint32_t color, float alpha, float spin) {
    if (batch_count + 60 > MAX_BATCH_QUADS * VERT_PER_QUAD) flush_batch();
    static vec3 NV[12];
    static bool init_done = false;
    if (!init_done) { for (int i = 0; i < 12; i++) NV[i] = ICO_V_RAW[i].normalized(); init_done = true; }

    float cs = std::cos(spin), sn = std::sin(spin);
    auto rotY = [&](const vec3& p) { return vec3(p.x*cs + p.z*sn, p.y, -p.x*sn + p.z*cs); };

    for (int f = 0; f < 20; f++) {
        vec3 a = rotY(NV[ICO_FACES[f][0]]), b = rotY(NV[ICO_FACES[f][1]]), c = rotY(NV[ICO_FACES[f][2]]);
        vec3 nrm = (b - a).cross(c - a).normalized();
        float sh = 0.35f + 0.65f * maxf(0.0f, nrm.y*0.6f + nrm.z*0.5f + 0.25f);
        float cr = rgba_r(color)*sh, cg = rgba_g(color)*sh, cb = rgba_b(color)*sh;
        BatchVertex* v = &batch_data[batch_count];
        auto put = [&](int i, const vec3& lp) {
            v[i].x = center.x + lp.x*scale; v[i].y = center.y + lp.y*scale; v[i].z = center.z + lp.z*scale;
            v[i].r = cr; v[i].g = cg; v[i].b = cb; v[i].a = alpha;
        };
        put(0,a); put(1,b); put(2,c);
        put(3,a); put(4,c); put(5,c);
        batch_count += VERT_PER_QUAD;
    }
}

// ---------- HUD ----------
static uint64_t glyph_bits(char c);

void Renderer::text(int x, int y, const char* s, uint32_t color, float scale) {
    mat4 saved = camera.vp;
    camera.vp = hud_proj;
    float px = float(x), py = float(y);
    float cell = 6.0f * scale;
    for (const char* p = s; *p; p++) {
        if (*p == ' ') { px += cell; continue; }
        uint64_t bits = glyph_bits(*p);
        for (int row = 0; row < 7; row++) {
            for (int colm = 0; colm < 5; colm++) {
                if (bits & (1ull << (row*5 + colm))) {
                    if (batch_count + VERT_PER_QUAD > MAX_BATCH_QUADS*VERT_PER_QUAD) flush_batch();
                    float gx = px + colm*scale, gy = py + (6-row)*scale; // инвертируем строки
                    BatchVertex* v = &batch_data[batch_count];
                    auto putpx = [&](int i, float X, float Y) {
                        v[i].x = X; v[i].y = 0; v[i].z = Y; set_col(v[i], color, 1.0f);
                    };
                    putpx(0, gx, gy); putpx(1, gx+scale, gy); putpx(2, gx+scale, gy+scale);
                    putpx(3, gx, gy); putpx(4, gx+scale, gy+scale); putpx(5, gx, gy+scale);
                    batch_count += VERT_PER_QUAD;
                }
            }
        }
        px += cell;
    }
    flush_batch();
    camera.vp = saved;
}

void Renderer::hud_bar(int x, int y, int w, int h, float frac, uint32_t color) {
    mat4 saved = camera.vp;
    camera.vp = hud_proj;
    if (batch_count + 2*VERT_PER_QUAD > MAX_BATCH_QUADS*VERT_PER_QUAD) flush_batch();
    BatchVertex* v = &batch_data[batch_count];
    auto putpx = [&](int i, float X, float Y, uint32_t c, float a) {
        v[i].x = X; v[i].y = 0; v[i].z = Y; set_col(v[i], c, a);
    };
    putpx(0,x,y,0x101528,0.85f); putpx(1,float(x+w),float(y),0x101528,0.85f); putpx(2,float(x+w),float(y+h),0x101528,0.85f);
    putpx(3,x,y,0x101528,0.85f); putpx(4,float(x+w),float(y+h),0x101528,0.85f); putpx(5,x,float(y+h),0x101528,0.85f);
    batch_count += 6;
    v = &batch_data[batch_count];
    float fw = w*clampf(frac,0,1);
    putpx(0,float(x),float(y),color,1.0f); putpx(1,x+fw,float(y),color,1.0f); putpx(2,x+fw,float(y+h),color,1.0f);
    putpx(3,float(x),float(y),color,1.0f); putpx(4,x+fw,float(y+h),color,1.0f); putpx(5,float(x),float(y+h),color,1.0f);
    batch_count += 6;
    flush_batch();
    camera.vp = saved;
}

// Битмап-шрифт 5x7 (строки снизу вверх, бит 0 = нижний левый пиксель)
static uint64_t glyph_bits(char c) {
    static const uint64_t font[44] = {
        // A-Z
        0x0FAA6u,0x1D554u,0x0E492u,0x1D554u,0x1F492u,0x1F486u,0x0FAAEu,0x1FBB6u,0x1F514u,0x0ABBEu,
        0x1F2AAu,0x1F08Au,0x1BFB6u,0x1DBB6u,0x0FAAEu,0x1F544u,0x0FAAEu,0x1F54Cu,0x0C492u,0x1FBB4u,
        0x0ABEEu,0x0AEE0u,0x0FBFEu,0x0AEEAu,0x0BBB6u,0x04DE0u,
        // 0-9
        0x0FAAEu,0x04924u,0x0C492u,0x09554u,0x13D54u,0x19496u,0x0FA96u,0x12492u,0x1F554u,0x19554u,
        // punct: ':' '-' '/' '.' '?' '!' '%' '(' ')' '+' '=' '<' '>' '#' '*' ',' ';' '!'->'|' '_' '[' ']'
        0x04920u,0x02040u,0x12492u,0x00800u,0x1192Au,0x04924u,0x08A28u,0x12400u,0x04924u,0x0AA28u,
        0x12492u,0x08410u,0x09249u,0x11800u,0x0AA28u,0x00800u,0x04920u,0x04924u,0x04924u,0x0F800u,
        0x0F800u,0x00F80u,
    };
    int idx;
    if (c >= 'A' && c <= 'Z') idx = c - 'A';
    else if (c >= 'a' && c <= 'z') idx = c - 'a';
    else if (c >= '0' && c <= '9') idx = 26 + (c - '0');
    else {
        switch (c) {
            case ':': idx = 36; break;
            case '-': idx = 37; break;
            case '/': idx = 38; break;
            case '.': idx = 39; break;
            case '?': idx = 40; break;
            case '!': idx = 41; break;
            default: return 0;
        }
    }
    return font[idx];
}
