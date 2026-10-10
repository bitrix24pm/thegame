// player.cpp — движение WASD, рывок Space, прицел мышью, атака ЛКМ, смена оружия 1/2/Q
#include "player.h"
#include "../world/dungeon.h"
#include "../math/math_utils.h"
#include <cmath>

void player_init(Player& p) {
    p.base = Entity{};
    p.base.hp = int16_t(PLAYER_MAX_HP);
    p.base.max_hp = int16_t(PLAYER_MAX_HP);
    p.hp_f = PLAYER_MAX_HP;
    p.aim = vec2(1, 0);
    p.dash_timer = 0;
    p.dash_cd = 0;
    p.iframes = 0;
    p.attack_cd = 0;
    p.weapon_idx = 0;
    p.tablet_mask = 0;
    // стартовое оружие — Клинок Пустоты
    p.weapons[0] = Weapon{0, WEAPON_DEFS[0].dmg, WEAPON_DEFS[0].rng, WEAPON_DEFS[0].spd, 0};
    // второй слот пуст (type 255 = нет оружия)
    p.weapons[1] = Weapon{255, 0, 0, 0, 0};
}

bool player_update(Player& p, const InputState& in, vec2 aim_world, float dt, CombatWorld& w) {
    bool attacked = false;

    // --- таймеры ---
    if (p.dash_cd > 0) p.dash_cd -= dt;
    if (p.iframes > 0) p.iframes -= dt;
    if (p.attack_cd > 0) p.attack_cd -= dt;
    if (p.dash_timer > 0) p.dash_timer -= dt;

    // --- прицел: мышь -> мировая точка на полу ---
    vec2 to_aim = aim_world - p.base.pos;
    if (to_aim.length_sq() > 1e-4f) p.aim = to_aim.normalized();

    // --- движение WASD (стрелки тоже) ---
    vec2 mv;
    if (in.keys[SDL_SCANCODE_W] || in.keys[SDL_SCANCODE_UP])    mv.y -= 1;
    if (in.keys[SDL_SCANCODE_S] || in.keys[SDL_SCANCODE_DOWN])  mv.y += 1;
    if (in.keys[SDL_SCANCODE_A] || in.keys[SDL_SCANCODE_LEFT])  mv.x -= 1;
    if (in.keys[SDL_SCANCODE_D] || in.keys[SDL_SCANCODE_RIGHT]) mv.x += 1;
    if (mv.length_sq() > 0) mv = mv.normalized();

    // Поворот изометрии на 45°: экранное "вверх" != мировая -Z. Вращаем ввод на -yaw.
    static const float CY = 0.70710678f, SY = 0.70710678f; // cos/sin 45°
    vec2 wm(mv.x * CY - mv.y * SY, mv.x * SY + mv.y * CY);

    if (p.dash_timer > 0) {
        // во время рывка скорость фиксированная
    } else {
        vec2 want = wm * PLAYER_SPEED;
        p.base.vel += (want - p.base.vel) * minf(1.0f, dt * 14.0f);
    }

    // --- рывок: Space по направлению движения или прицела ---
    if (in.key_pressed[SDL_SCANCODE_SPACE] && p.dash_cd <= 0 && p.dash_timer <= 0) {
        vec2 ddir = (wm.length_sq() > 0.01f) ? wm.normalized() : p.aim;
        p.base.vel = ddir * DASH_SPEED;
        p.dash_timer = DASH_TIME;
        p.dash_cd = DASH_CD;
        w.audio->sfx_dash();
        w.spawn_particles(p.base.pos, 6, COL_PLAYER, 2.0f);
    }

    // --- интеграция + трение во время рывка ---
    if (p.dash_timer > 0) p.base.vel *= (1.0f - minf(1.0f, dt * 3.0f));
    p.base.pos += p.base.vel * dt;

    // --- коллизия со стенами (осевое скольжение) ---
    if (!DungeonGen::circle_free(*w.dungeon, p.base.pos.x, p.base.pos.y, PLAYER_RADIUS)) {
        if (DungeonGen::circle_free(*w.dungeon, p.base.pos.x, p.base.pos.y - p.base.vel.y * dt, PLAYER_RADIUS)) {
            p.base.pos.y -= p.base.vel.y * dt; p.base.vel.y = 0;
        } else if (DungeonGen::circle_free(*w.dungeon, p.base.pos.x - p.base.vel.x * dt, p.base.pos.y, PLAYER_RADIUS)) {
            p.base.pos.x -= p.base.vel.x * dt; p.base.vel.x = 0;
        } else {
            p.base.pos -= p.base.vel * dt; p.base.vel = vec2();
        }
    }

    // --- смена оружия: 1 / 2 / Q (таб) ---
    auto try_switch = [&](uint8_t idx) {
        if (idx < 2 && p.weapons[idx].type != 255 && idx != p.weapon_idx) {
            p.weapon_idx = idx;
            w.audio->sfx_pickup();
        }
    };
    if (in.key_pressed[SDL_SCANCODE_1]) try_switch(0);
    if (in.key_pressed[SDL_SCANCODE_2]) try_switch(1);
    if (in.key_pressed[SDL_SCANCODE_TAB] || in.key_pressed[SDL_SCANCODE_Q])
        try_switch(uint8_t((p.weapon_idx + 1) % 2));

    // --- атака: ЛКМ (удержание = автоатака по кулдауну) ---
    Weapon& wpn = p.weapons[p.weapon_idx];
    if (wpn.type != 255 && in.mouse_l && p.attack_cd <= 0) {
        perform_attack(w);
        attacked = true;
        p.attack_cd = 10.0f / maxf(float(wpn.speed), 1.0f); // speed = атак/сек*10
    }

    // синхронизируем hp
    p.base.hp = int16_t(maxf(0.0f, p.hp_f));
    return attacked;
}
