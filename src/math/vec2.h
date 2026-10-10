// vec2.h — 2D вектор (игровая логика на плоскости пола)
#pragma once
#include <cmath>

struct vec2 {
    float x = 0, y = 0;
    vec2() {}
    vec2(float x_, float y_) : x(x_), y(y_) {}

    vec2 operator+(const vec2& o) const { return {x + o.x, y + o.y}; }
    vec2 operator-(const vec2& o) const { return {x - o.x, y - o.y}; }
    vec2 operator*(float s) const { return {x * s, y * s}; }
    vec2 operator/(float s) const { return {x / s, y / s}; }
    vec2& operator+=(const vec2& o) { x += o.x; y += o.y; return *this; }
    vec2& operator-=(const vec2& o) { x -= o.x; y -= o.y; return *this; }
    vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float length_sq() const { return x * x + y * y; }
    vec2 normalized() const {
        float l = length();
        return l > 1e-6f ? vec2(x / l, y / l) : vec2(0, 0);
    }
    float dot(const vec2& o) const { return x * o.x + y * o.y; }
    float cross(const vec2& o) const { return x * o.y - y * o.x; }
    vec2 perp() const { return {-y, x}; }
};

inline vec2 operator*(float s, const vec2& v) { return v * s; }
inline float distance2d(const vec2& a, const vec2& b) { return (a - b).length(); }
