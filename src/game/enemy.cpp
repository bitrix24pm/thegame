// enemy.cpp — ИИ: шип (rush), глаз (стрелок), разломщик (AoE-с Slam), босс (комбинация)
#include "enemy.h"
#include "../world/dungeon.h"
#include "../data/constants.h"
#include "../math/math_utils.h"
#include <cmath>

void spawn_enemy(CombatWorld& w, vec2 pos, uint8_t type, int floor_num, bool is_boss) {
    if (*w.enemy_count >= MAX_ENEMIES) return;
    Enemy& e = w.enemies[*w.enemy_count];
    (*w.enemy_count)++;
    e.base.pos = pos;
    e.base.vel = vec2();
    e.base.state = E_IDLE;
    e.base.type = type;
    e.is_boss = is_boss;
    e.state_timer = 0;
    e.slow_timer = 0;
    e.speed_mult = 1.0f;
    e.knock_res = 1.0f;
    if (is_boss) {
        e.base.hp = int16_t(400 + floor_num * 150);
        e.radius = 1.3f;
        e.knock_res = 6.0f;
    } else {
        switch (type) {
            case 0: e.base.hp = int16_t(30 + floor_num * 10); e.radius = 0.45f; break; // шип
            case 1: e.base.hp = int16_t(24 + floor_num * 8);  e.radius = 0.45f; break; // глаз
            default: e.base.hp = int16_t(50 + floor_num * 15); e.radius = 0.6f; e.knock_res = 2.0f; break;
        }
    }
    e.base.max_hp = e.base.hp;
}

static float enemy_speed(const Enemy& e) {
    float base_sp;
    switch (e.base.type) {
        case 0: base_sp = 5.5f; break;
        case 1: base_sp = 2.6f; break;
        default: base_sp = 3.6f;
    }
    if (e.is_boss) base_sp = 4.2f;
    return base_sp * (e.slow_timer > 0 ? e.speed_mult : 1.0f);
}

static void fire_projectile(CombatWorld& w, vec2 from, vec2 dir, float dmg, float speed) {
    int slot = -1;
    for (int q = 0; q < MAX_PROJECTILES; q++) if (!w.projectiles[q].alive) { slot = q; break; }
    if (slot < 0) return;
    Projectile& pr = w.projectiles[slot];
    pr.alive = true;
    pr.pos = from;
    pr.vel = dir.normalized() * speed;
    pr.damage = dmg;
    pr.lifetime = 3.0f;
    pr.from_player = false;
    pr.tablets = 0;
}

void EnemyAI::update_all(CombatWorld& w, float dt, float time_sec) {
    Player& p = *w.player;
    for (int i = 0; i < *w.enemy_count; i++) {
        Enemy& e = w.enemies[i];
        if (e.base.state == E_DEAD) continue;

        if (e.slow_timer > 0) e.slow_timer -= dt;

        vec2 to_p = p.base.pos - e.base.pos;
        float dist = to_p.length();
        vec2 dir = dist > 1e-4f ? to_p / dist : vec2(1, 0);

        float sp = enemy_speed(e);
        float prep_time = e.is_boss ? 0.45f : 0.55f;
        float attack_range = e.is_boss ? 9.0f : (e.base.type == 1 ? 7.0f : 1.6f);

        switch (e.base.state) {
            case E_IDLE: {
                e.state_timer += dt;
                if (dist < 12.0f && e.state_timer > 0.3f) { e.base.state = E_CHASE; e.state_timer = 0; }
                break;
            }
            case E_CHASE: {
                // движение к игроку с лёгким обходом стен (скольжение вдоль нормали стены)
                vec2 want = dir * sp;
                // для глаза держим дистанцию
                if (e.base.type == 1 && !e.is_boss && dist < 4.5f) want = -dir * sp * 0.7f;
                e.base.vel += (want - e.base.vel) * minf(1.f, dt * 6.f);

                // атака когда в радиусе и часто моргаем (тайминг)
                e.state_timer += dt;
                float cd = e.is_boss ? 1.1f : (e.base.type == 1 ? 1.6f : 1.2f);
                if (dist < attack_range && e.state_timer > cd) {
                    e.base.state = E_ATTACK_PREP;
                    e.state_timer = 0;
                }
                break;
            }
            case E_ATTACK_PREP: {
                e.state_timer += dt;
                e.base.vel *= 0.9f; // останавливаемся — телеграф
                if (e.state_timer >= prep_time) {
                    e.base.state = E_ATTACK;
                    e.state_timer = 0;
                    // сам удар
                    if (e.base.type == 1 || e.is_boss) {
                        // стрелок: снаряд(ы)
                        fire_projectile(w, e.base.pos, dir, e.is_boss ? 14.f : 8.f, 9.f);
                        if (e.is_boss) {
                            float a = std::atan2(dir.y, dir.x);
                            for (int k = -2; k <= 2; k++)
                                if (k != 0)
                                    fire_projectile(w, e.base.pos,
                                        vec2(std::cos(a + k*0.25f), std::sin(a + k*0.25f)), 10.f, 8.f);
                        }
                        w.audio->sfx_shoot();
                    } else if (e.base.type == 2) {
                        // slam: урон по площади вокруг себя при контакте позже
                        if (dist < e.radius + PLAYER_RADIUS + 0.6f)
                            damage_player(w, 16.f, e.base.pos);
                        w.spawn_particles(e.base.pos, 10, COL_ENEMY2, 3.f);
                        w.audio->sfx_ring();
                    } else {
                        // rush: короткий рывок вперёд
                        e.base.vel = dir * 16.f;
                        w.audio->sfx_swing();
                    }
                }
                break;
            }
            case E_ATTACK: {
                e.state_timer += dt;
                if (e.base.type == 0 && !e.is_boss) {
                    // инерционный рывок; контакт во время атаки
                    if (dist < e.radius + PLAYER_RADIUS + 0.1f)
                        damage_player(w, 10.f, e.base.pos);
                }
                if (e.state_timer > 0.35f) { e.base.state = E_RECOVER; e.state_timer = 0; }
                break;
            }
            case E_RECOVER: {
                e.state_timer += dt;
                e.base.vel *= 0.85f;
                if (e.state_timer > 0.4f) { e.base.state = E_CHASE; e.state_timer = 0; }
                break;
            }
        }

        // интеграция + трение
        e.base.pos += e.base.vel * dt;
        if (e.base.state != E_ATTACK || e.base.type != 0)
            e.base.vel *= (1.0f - minf(1.f, dt * 4.f));

        // коллизия со стенами (по осям — скольжение)
        if (!DungeonGen::circle_free(*w.dungeon, e.base.pos.x, e.base.pos.y, e.radius)) {
            if (DungeonGen::circle_free(*w.dungeon, e.base.pos.x, e.base.pos.y - e.base.vel.y * dt, e.radius)) {
                e.base.pos.y -= e.base.vel.y * dt; e.base.vel.y = 0;
            } else if (DungeonGen::circle_free(*w.dungeon, e.base.pos.x - e.base.vel.x * dt, e.base.pos.y, e.radius)) {
                e.base.pos.x -= e.base.vel.x * dt; e.base.vel.x = 0;
            } else {
                e.base.pos -= e.base.vel * dt; e.base.vel = vec2();
            }
        }

        // расталкивание врагов друг от друга
        for (int j = i + 1; j < *w.enemy_count; j++) {
            Enemy& o = w.enemies[j];
            if (o.base.state == E_DEAD) continue;
            vec2 d = o.base.pos - e.base.pos;
            float dl = d.length();
            float min_d = e.radius + o.radius;
            if (dl < min_d && dl > 1e-4f) {
                vec2 push = d / dl;
                float corr = (min_d - dl) * 0.5f;
                e.base.pos -= push * corr;
                o.base.pos += push * corr;
            }
        }

        // контактный урон ближнего типа даже вне атаки (небольшой)
        if (dist < e.radius + PLAYER_RADIUS && e.base.state != E_ATTACK_PREP) {
            if (time_sec > 0) damage_player(w, 4.f * dt * 10.f, e.base.pos);
        }
    }
}
