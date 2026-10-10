// math_utils.h — базовые математические функции. Без внешних библиотек.
#pragma once
#include <cmath>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr float PI_F = 3.14159265358979f;
constexpr float TAU_F = 6.28318530717958f;

inline float radians(float deg) { return deg * (PI_F / 180.0f); }
inline float degrees(float rad) { return rad * (180.0f / PI_F); }

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float minf(float a, float b) { return a < b ? a : b; }
inline float maxf(float a, float b) { return a > b ? a : b; }

inline float smoothstep(float e0, float e1, float x) {
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Угол между двумя векторами (-PI..PI), покомпонентно для vec2-подобных типов
template <typename V>
inline float angle_between(const V& a, const V& b) {
    float d = a.x * b.x + a.y * b.y;
    float l = std::sqrt((a.x*a.x+a.y*a.y) * (b.x*b.x+b.y*b.y));
    if (l < 1e-6f) return 0.0f;
    return std::acos(clampf(d / l, -1.0f, 1.0f));
}

// Быстрый целочисленный ГПСЧ (xorshift32) — для процедурной генерации
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed = 0x9E3779B9u) : s(seed ? seed : 1u) {}
    inline uint32_t next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return s;
    }
    inline int range(int lo, int hi) { // [lo, hi]
        return lo + int(next() % uint32_t(hi - lo + 1));
    }
    inline float flt() { return (next() >> 8) * (1.0f / 16777216.0f); } // 0..1
};
