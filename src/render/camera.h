// camera.h/cpp — изометрическая ортографическая камера
#pragma once
#include "../math/mat4.h"
#include "../math/vec2.h"

struct Camera {
    mat4 projection;
    mat4 view;
    mat4 vp;            // projection * view (кэш)
    vec2 target;        // мировая цель (x,z на полу)
    float zoom = 1.0f;
    float aspect = 16.0f / 9.0f;

    void update(vec2 follow, float dt);
    // экран -> мир: пересечение луча с плоскостью пола y=0 (в изометрии)
    vec2 screen_to_world(float sx, float sy, int sw, int sh) const;
};

void init_camera(Camera& cam, float aspect_);
