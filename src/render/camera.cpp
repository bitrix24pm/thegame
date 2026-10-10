// camera.cpp — реализация изометрической камеры + обратное проецирование мыши
#include "camera.h"
#include "../math/math_utils.h"
#include <cmath>

// Константы изометрии
static const float ISO_TILT = radians(35.264f);   // arctan(1/sqrt(2))
static const float ISO_YAW  = radians(45.0f);
static const float VIEW_H = 26.0f;                // мировая высота видимой области

void init_camera(Camera& cam, float aspect_) {
    cam.aspect = aspect_;
}

void Camera::update(vec2 follow, float dt) {
    // Плавное следование
    target += (follow - target) * minf(1.0f, dt * 8.0f);

    float h = VIEW_H / zoom;
    float w = h * aspect;
    projection = mat4::ortho(-w / 2, w / 2, -h / 2, h / 2, -100.0f, 100.0f);

    // Вид: translate(-target_world) * rotX(tilt) * rotY(yaw)
    vec3 tgt(target.x, 0.0f, target.y);
    mat4 rot_x = mat4::rotateX(ISO_TILT);
    mat4 rot_y = mat4::rotateY(ISO_YAW);
    mat4 trans = mat4::translate(tgt * -1.0f);
    view = rot_x * rot_y * trans;
    vp = projection * view;
}

vec2 Camera::screen_to_world(float sx, float sy, int sw, int sh) const {
    // NDC
    float nx = (sx / sw) * 2.0f - 1.0f;
    float ny = 1.0f - (sy / sh) * 2.0f;

    // Орто-камера: world = inverse(view) * (nx*halfW, ny*halfH, z, 1)
    // Проще решить аналитически: в системе камеры точка пола y_w=0.
    // Камера: V = Rx * Ry * T(-t). Точка на экране в камерном пространстве:
    // p_c = (nx*h/2*aspect? ...) используем обратную матрицу через транспонирование поворотов.
    float h = VIEW_H / zoom;
    float w = h * aspect;
    vec3 p_cam(nx * w / 2, ny * h / 2, 0); // z произвольный для орто, берём 0

    // inverse(Rx*Ry) = Ry^-1 * Rx^-1
    float cy = std::cos(-ISO_YAW), sy_ = std::sin(-ISO_YAW);
    float cx = std::cos(-ISO_TILT), sx_ = std::sin(-ISO_TILT);
    // Ry(-yaw): x' = c*x + s*z ; z' = -s*x + c*z
    vec3 a(cy * p_cam.x + sy_ * p_cam.z, p_cam.y, -sy_ * p_cam.x + cy * p_cam.z);
    // Rx(-tilt): y' = c*y - s*z ; z' = s*y + c*z
    vec3 b(a.x, cx * a.y - sx_ * a.z, sx_ * a.y + cx * a.z);
    //平移
    b = b + vec3(target.x, 0.0f, target.y);

    // Теперь луч в мире: начало b (т.к. орто — направление = forward камеры)
    // Направление взгляда камеры в мире: -(Ry^-1 Rx^-1) * (0,0,-1)... проще:
    // forward_cam = (0,0,-1) -> world dir = Ry(yaw)*Rx(tilt) applied to (0,0,-1)? Нет:
    // view = Rx*Ry*T => world->cam. cam->world rotation = Ry^-1*Rx^-1. dir_cam=(0,0,-1).
    vec3 d_cam(0, 0, -1);
    vec3 a2(cy * d_cam.x + sy_ * d_cam.z, d_cam.y, -sy_ * d_cam.x + cy * d_cam.z);
    vec3 d(a2.x, cx * a2.y - sx_ * a2.z, sx_ * a2.y + cx * a2.z);

    // Пересечение с плоскостью y=0: t = -b.y/d.y
    if (std::fabs(d.y) < 1e-5f) return target;
    float t = -b.y / d.y;
    vec3 hit = b + d * t;
    return vec2(hit.x, hit.z);
}
