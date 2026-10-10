// mat4.h — 4x4 матрица (столбцовый порядок, как в OpenGL)
#pragma once
#include "vec3.h"
#include <cmath>
#include <cstring>

struct mat4 {
    float m[16]; // column-major: m[col*4 + row]

    static mat4 identity() {
        mat4 r; std::memset(r.m, 0, sizeof(r.m));
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    static mat4 translate(const vec3& t) {
        mat4 r = identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    static mat4 scale(float s) {
        mat4 r = identity();
        r.m[0] = r.m[5] = r.m[10] = s;
        return r;
    }
    static mat4 scale(const vec3& s) {
        mat4 r = identity();
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
        return r;
    }

    // Угол в радианах
    static mat4 rotateX(float a) {
        float c = std::cos(a), s = std::sin(a);
        mat4 r = identity();
        r.m[5] = c;  r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }
    static mat4 rotateY(float a) {
        float c = std::cos(a), s = std::sin(a);
        mat4 r = identity();
        r.m[0] = c;  r.m[2] = -s;
        r.m[8] = s;  r.m[10] = c;
        return r;
    }
    static mat4 rotateZ(float a) {
        float c = std::cos(a), s = std::sin(a);
        mat4 r = identity();
        r.m[0] = c;  r.m[1] = s;
        r.m[4] = -s; r.m[5] = c;
        return r;
    }

    static mat4 ortho(float l, float r_, float b, float t, float n, float f) {
        mat4 r = identity();
        r.m[0] = 2.0f / (r_ - l);
        r.m[5] = 2.0f / (t - b);
        r.m[10] = -2.0f / (f - n);
        r.m[12] = -(r_ + l) / (r_ - l);
        r.m[13] = -(t + b) / (t - b);
        r.m[14] = -(f + n) / (f - n);
        return r;
    }

    mat4 operator*(const mat4& o) const {
        mat4 r; std::memset(r.m, 0, sizeof(r.m));
        for (int col = 0; col < 4; col++)
            for (int row = 0; row < 4; row++) {
                float sum = 0;
                for (int k = 0; k < 4; k++)
                    sum += m[k * 4 + row] * o.m[col * 4 + k];
                r.m[col * 4 + row] = sum;
            }
        return r;
    }

    vec3 transform_point(const vec3& p) const {
        vec3 out;
        out.x = m[0]*p.x + m[4]*p.y + m[8]*p.z + m[12];
        out.y = m[1]*p.x + m[5]*p.y + m[9]*p.z + m[13];
        out.z = m[2]*p.x + m[6]*p.y + m[10]*p.z + m[14];
        return out;
    }
};
