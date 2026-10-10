// combat.h — боевая система: оружие, скрижали, коллизии, урон, эффекты
#pragma once
#include "../data/structs.h"
#include "../data/constants.h"
#include "../procedural/sound.h"

struct CombatWorld {
    Player* player = nullptr;
    Enemy* enemies = nullptr;      // массив MAX_ENEMIES
    int* enemy_count = nullptr;
    Projectile* projectiles = nullptr;
    Ring* rings = nullptr;
    Trap* traps = nullptr;
    Particle* particles = nullptr;
    AudioEngine* audio = nullptr;
    const Dungeon* dungeon = nullptr;

    void damage_enemy(int idx, float dmg, uint8_t tablets, vec2 from_pos);
    void kill_enemy(int idx, uint8_t tablets);
    void spawn_explosion(vec2 pos, float radius, uint32_t color);
    void spawn_particles(vec2 pos, int n, uint32_t color, float speed);
    void apply_tablets(uint8_t flags, int enemy_idx, vec2 from_pos);
};

// Атаки игрока (по типу оружия)
void perform_attack(CombatWorld& w);
void echo_attack(CombatWorld& w, vec2 origin, vec2 dir, const Weapon& weapon);

// Обновления
void update_projectiles(CombatWorld& w, float dt);
void update_rings(CombatWorld& w, float dt);
void update_traps(CombatWorld& w, float dt);
void update_particles(CombatWorld& w, float dt);

// Урон по игроку
void damage_player(CombatWorld& w, float dmg, vec2 from_pos);
