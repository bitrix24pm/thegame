// vec3.h — 3D вектор (рендер, изометрия)
#pragma once
#include <cmath>

struct vec3 {
    float x = 0, y = 0, z = 0;
    vec3() {}
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    vec3 operator-() const { return {-x, -y, -z}; }
    vec3& operator+=(const vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

    float length() const { return std::sqrt(x*x + y*y + z*z); }
    vec3 normalized() const {
        float l = length();
        return l > 1e-6f ? vec3{x / l, y / l, z / l} : vec3(0, 0, 0);
    }
    float dot(const vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    vec3 cross(const vec3& o) const {
        return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
    }
};

inline vec3 operator*(float s, const vec3& v) { return v * s; }
