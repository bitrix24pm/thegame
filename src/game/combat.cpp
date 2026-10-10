// combat.cpp — реализация боёвки: конус/луч/кольцо/точка + скрижали
#include "combat.h"
#include "../world/dungeon.h"
#include "../math/math_utils.h"
#include <cmath>

void CombatWorld::spawn_particles(vec2 pos, int n, uint32_t color, float speed) {
    for (int k = 0; k < n; k++) {
        // ищем мёртвую частицу        int slot = -1;
        for (int i = 0; i < MAX_PARTICLES; i++) if (!particles[i].alive) { slot = i; break; }
        if (slot < 0) slot = k % MAX_PARTICLES;
        Particle& p = particles[slot];
        float a = Rng(uint32_t(pos.x * 1000 + k * 77 + p.pos.z)).flt() * TAU_F;
        float sp = speed * (0.4f + 0.6f * std::sin(float(k) * 2.4f));
        p.alive = true;
        p.pos = vec3(pos.x, 0.35f, pos.y);
        p.vel = vec3(std::cos(a) * sp, 2.0f + sp * 0.3f, std::sin(a) * sp);
        p.max_life = 0.4f + 0.4f * std::fabs(std::sin(float(k)));
        p.life = p.max_life;
        p.size = 0.10f + 0.08f * std::fabs(std::cos(float(k) * 1.7f));
        p.color = color;
    }
}

void CombatWorld::spawn_explosion(vec2 pos, float radius, uint32_t color) {
    spawn_particles(pos, 24, color, radius * 2.5f);
    audio->sfx_explode();
}

void CombatWorld::apply_tablets(uint8_t flags, int ei, vec2 from_pos) {
    Enemy& e = enemies[ei];
    if (flags & TAB_IMPULSE) {
        vec2 push = (e.base.pos - from_pos).normalized();
        e.base.vel += push * 9.0f / maxf(e.knock_res, 0.5f);
    }
    if (flags & TAB_STASIS) {
        e.slow_timer = 2.0f;
        e.speed_mult = 0.3f;
    }
}

void CombatWorld::kill_enemy(int idx, uint8_t tablets) {
    Enemy& e = enemies[idx];
    if (e.base.state == E_DEAD) return;
    uint32_t col = e.is_boss ? COL_BOSS : (e.type == 0 ? COL_ENEMY0 : e.type == 1 ? COL_ENEMY1 : COL_ENEMY2);
    spawn_particles(e.base.pos, e.is_boss ? 48 : 14, col, e.is_boss ? 8.f : 4.f);
    e.base.state = E_DEAD;
    audio->sfx_death();

    if (tablets & TAB_RESONANCE) {
        float radius = e.is_boss ? 6.0f : 3.0f;
        float dmg = e.base.max_hp * 0.5f;
        spawn_explosion(e.base.pos, radius, col);
        for (int j = 0; j < *enemy_count; j++) {
            if (j == idx || enemies[j].base.state == E_DEAD) continue;
            if (distance2d(enemies[j].base.pos, e.base.pos) <= radius)
                damage_enemy(j, dmg, 0, e.base.pos);
        }
    }
}

void CombatWorld::damage_enemy(int idx, float dmg, uint8_t tablets, vec2 from_pos) {
    Enemy& e = enemies[idx];
    if (e.base.state == E_DEAD) return;
    e.base.hp -= int16_t(dmg);
    apply_tablets(tablets, idx, from_pos);
    audio->sfx_hit();
    spawn_particles(e.base.pos, 4, 0xffffff, 2.5f);
    if (e.base.hp <= 0) kill_enemy(idx, tablets);
}

void damage_player(CombatWorld& w, float dmg, vec2 from_pos) {
    Player& p = *w.player;
    if (p.iframes > 0 || p.dash_timer > 0) return;
    p.hp_f -= dmg;
    p.base.hp = int16_t(maxf(0, p.hp_f));
    p.iframes = IFRAME_TIME + 0.35f;
    w.audio->sfx_hurt();
    vec2 knock = (p.base.pos - from_pos).normalized();
    p.base.vel += knock * 6.0f;
    w.spawn_particles(p.base.pos, 8, COL_PLAYER, 3.0f);
}

// ---- Атака игрока ----
static uint32_t weapon_color(const Weapon& w) {
    static const uint32_t cols[4] = {COL_PLAYER, COL_GOLD, 0x6bfff0, 0xff8f4f};
    return cols[w.type & 3];
}

void perform_attack(CombatWorld& w) {
    Player& p = *w.player;
    Weapon& wpn = p.weapons[p.weapon_idx];
    uint8_t tabs = wpn.tablets | p.tablet_mask;
    float range = wpn.range / 10.0f;
    vec2 dir = p.aim.normalized();

    switch (wpn.type) {
        case 0: { // BLADE — конус
            float half_angle = radians(50.0f);
            for (int i = 0; i < *w.enemy_count; i++) {
                Enemy& e = w.enemies[i];
                if (e.base.state == E_DEAD) continue;
                vec2 to_e = e.base.pos - p.base.pos;
                float dist = to_e.length();
                if (dist > range + e.radius) continue;
                float ang = angle_between(dir, to_e);
                if (ang <= half_angle) {
                    w.damage_enemy(i, wpn.damage, tabs, p.base.pos);
                    if (tabs & TAB_FRAGMENT) {
                        // осколки: 2 снаряда в стороны
                        for (int s = -1; s <= 1; s += 2) {
                            int slot = -1;
                            for (int q = 0; q < MAX_PROJECTILES; q++) if (!w.projectiles[q].alive) { slot = q; break; }
                            if (slot < 0) continue;
                            Projectile& pr = w.projectiles[slot];
                            pr.alive = true;
                            pr.pos = p.base.pos;
                            float rot = s * 0.6f;
                            pr.vel = vec2(dir.x * std::cos(rot) - dir.y * std::sin(rot),
                                          dir.x * std::sin(rot) + dir.y * std::cos(rot)) * 14.0f;
                            pr.damage = wpn.damage * 0.4f;
                            pr.lifetime = 0.5f;
                            pr.from_player = true;
                            pr.tablets = 0;
                        }
                    }
                }
            }
            w.audio->sfx_swing();
            break;
        }
        case 1: { // CHISEL — снаряд-луч
            int slot = -1;
            for (int q = 0; q < MAX_PROJECTILES; q++) if (!w.projectiles[q].alive) { slot = q; break; }
            if (slot >= 0) {
                Projectile& pr = w.projectiles[slot];
                pr.alive = true; pr.pos = p.base.pos; pr.vel = dir * 18.0f;
                pr.damage = wpn.damage; pr.lifetime = range / 18.0f + 0.2f;
                pr.from_player = true; pr.tablets = tabs;
            }
            w.audio->sfx_shoot();
            break;
        }
        case 2: { // COMPASS — кольцо
            int slot = -1;
            for (int q = 0; q < MAX_RINGS; q++) if (!w.rings[q].alive) { slot = q; break; }
            if (slot >= 0) {
                Ring& r = w.rings[slot];
                r.alive = true; r.center = p.base.pos; r.radius = 0.3f;
                r.max_radius = range; r.speed = range * 2.2f;
                r.damage = wpn.damage; r.tablets = tabs; r.hit_mask = 0;
            }
            w.audio->sfx_ring();
            break;
        }
        case 3: { // ANCHOR — ловушка
            int slot = -1;
            for (int q = 0; q < MAX_TRAPS; q++) if (!w.traps[q].alive) { slot = q; break; }
            if (slot >= 0) {
                Trap& t = w.traps[slot];
                t.alive = true; t.pos = p.base.pos + dir * 1.2f;
                t.radius = range * 0.5f; t.damage = wpn.damage;
                t.duration = 8.0f; t.tick_timer = 0;
            }
            w.audio->sfx_trap();
            break;
        }
    }

    // ECHO: призрачный повтор через 0.25 сек — реализуем мгновенным вторым проходом со сдвигом
    if (tabs & TAB_ECHO) {
        echo_attack(w, p.base.pos, dir, wpn);
    }
}

void echo_attack(CombatWorld& w, vec2 origin, vec2 dir, const Weapon& weapon) {
    // слабый повтор того же типа без скрижальных эффектов (кроме урона)
    float range = weapon.range / 10.0f;
    if (weapon.type == 0) {
        float half_angle = radians(50.0f);
        for (int i = 0; i < *w.enemy_count; i++) {
            Enemy& e = w.enemies[i];
            if (e.base.state == E_DEAD) continue;
            vec2 to_e = e.base.pos - origin;
            if (to_e.length() <= range + e.radius && angle_between(dir, to_e) <= half_angle)
                w.damage_enemy(i, weapon.damage * 0.4f, 0, origin);
        }
    } else if (weapon.type == 2) {
        int slot = -1;
        for (int q = 0; q < MAX_RINGS; q++) if (!w.rings[q].alive) { slot = q; break; }
        if (slot >= 0) {
            Ring& r = w.rings[slot];
            r.alive = true; r.center = origin; r.radius = 0.3f;
            r.max_radius = range * 0.7f; r.speed = range * 1.5f;
            r.damage = weapon.damage * 0.4f; r.tablets = 0; r.hit_mask = 0;
        }
    }
}

// ---- Обновления ----
void update_projectiles(CombatWorld& w, float dt) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile& pr = w.projectiles[i];
        if (!pr.alive) continue;
        pr.pos += pr.vel * dt;
        pr.lifetime -= dt;
        if (pr.lifetime <= 0 || !DungeonGen::circle_free(*w.dungeon, pr.pos.x, pr.pos.y, 0.08f)) {
            pr.alive = false;
            w.spawn_particles(pr.pos, 3, pr.from_player ? COL_PLAYER : COL_ENEMY0, 1.5f);
            continue;
        }
        if (pr.from_player) {
            for (int e = 0; e < *w.enemy_count; e++) {
                Enemy& en = w.enemies[e];
                if (en.base.state == E_DEAD) continue;
                if (distance2d(en.base.pos, pr.pos) < en.radius + 0.15f) {
                    w.damage_enemy(e, pr.damage, pr.tablets, pr.pos);
                    pr.alive = false;
                    break;
                }
            }
        } else {
            Player& p = *w.player;
            if (distance2d(p.base.pos, pr.pos) < PLAYER_RADIUS + 0.15f) {
                damage_player(w, pr.damage, pr.pos);
                pr.alive = false;
            }
        }
    }
}

void update_rings(CombatWorld& w, float dt) {
    for (int i = 0; i < MAX_RINGS; i++) {
        Ring& r = w.rings[i];
        if (!r.alive) continue;
        r.radius += r.speed * dt;
        if (r.radius >= r.max_radius) { r.alive = false; continue; }
        for (int e = 0; e < *w.enemy_count && e < 64; e++) {
            Enemy& en = w.enemies[e];
            if (en.base.state == E_DEAD) continue;
            uint64_t bit = 1ull << e;
            if (r.hit_mask & bit) continue;
            float d = distance2d(en.base.pos, r.center);
            if (std::fabs(d - r.radius) < en.radius + 0.25f) {
                r.hit_mask |= bit;
                w.damage_enemy(e, r.damage, r.tablets, r.center);
            }
        }
    }
}

void update_traps(CombatWorld& w, float dt) {
    for (int i = 0; i < MAX_TRAPS; i++) {
        Trap& t = w.traps[i];
        if (!t.alive) continue;
        t.duration -= dt;
        if (t.duration <= 0) { t.alive = false; continue; }
        t.tick_timer -= dt;
        if (t.tick_timer > 0) continue;
        t.tick_timer = 0.5f;
        for (int e = 0; e < *w.enemy_count; e++) {
            Enemy& en = w.enemies[e];
            if (en.base.state == E_DEAD) continue;
            if (distance2d(en.base.pos, t.pos) <= t.radius)
                w.damage_enemy(e, t.damage, 0, t.pos);
        }
    }
}

void update_particles(CombatWorld& w, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = w.particles[i];
        if (!p.alive) continue;
        p.life -= dt;
        if (p.life <= 0) { p.alive = false; continue; }
        p.vel.y -= 9.0f * dt; // гравитация
        p.pos += p.vel * dt;
        if (p.pos.y < 0.05f) { p.pos.y = 0.05f; p.vel.y *= -0.4f; p.vel.x *= 0.7f; p.vel.z *= 0.7f; }
    }
}
