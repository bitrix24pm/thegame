// structs.h — все структуры данных игры (компактные, как в ТЗ)
#pragma once
#include <cstdint>
#include "../math/vec2.h"
#include "../math/vec3.h"

constexpr int MAX_ROOMS_C = 16;
constexpr int DUNGEON_GRID_C = 48;
constexpr int DUNGEON_TILES = DUNGEON_GRID_C * DUNGEON_GRID_C;

// --- Состояния ИИ врага (конечный автомат) ---
enum EnemyState : uint8_t {
    E_IDLE = 0, E_CHASE = 1, E_ATTACK_PREP = 2, E_ATTACK = 3, E_RECOVER = 4, E_DEAD = 5
};

// Минимальная структура сущности (~22 байта по ТЗ)
struct Entity {
    vec2 pos;        // 8
    vec2 vel;        // 8
    int16_t hp;      // 2
    int16_t max_hp;  // 2
    uint8_t state;   // 1
    uint8_t type;    // 1
};

// Оружие: 5 байт по ТЗ
struct Weapon {
    uint8_t type;     // 0=BLADE,1=CHISEL,2=COMPASS,3=ANCHOR
    uint8_t damage;
    uint8_t range;    // в десятых долях
    uint8_t speed;    // атак/сек * 10
    uint8_t tablets;  // битовая маска модификаторов
};

struct Player {
    Entity base;
    vec2 aim;           // направление прицела
    float dash_timer;   // >0 — рывок активен
    float dash_cd;      // кулдаун (сек)
    float iframes;      // неуязвимость
    float attack_cd;
    uint8_t weapon_idx;
    Weapon weapons[2];
    uint8_t tablet_mask; // собранные скрижали игрока
    float hp_f;          // точное HP
};

struct Enemy {
    Entity base;
    float state_timer;
    float slow_timer;
    float speed_mult;
    float knock_res;     // сопротивление отбрасыванию
    float radius;
    bool is_boss;
};

struct Projectile {
    vec2 pos, vel;
    float damage;
    float lifetime;
    bool from_player;
    uint8_t tablets;
    bool alive;
};

struct Ring {           // кольцо COMPASS
    vec2 center;
    float radius, max_radius, speed, damage;
    uint8_t tablets;
    uint64_t hit_mask;  // враги уже получили урон (до 64)
    bool alive;
};

struct Trap {           // ANCHOR-ловушка
    vec2 pos;
    float radius, damage;
    float duration;
    float tick_timer;
    bool alive;
};

struct Particle {
    vec3 pos, vel;
    float life, max_life, size;
    uint32_t color;
    bool alive;
};

struct Pickup {
    vec2 pos;
    uint8_t kind;       // 0=weapon, 1=tablet, 2=health
    Weapon weapon;
    uint8_t tablet_flag;
    float bob;
    bool alive;
};

// --- Подземелье ---
struct Room {
    int16_t x, y;
    uint8_t w, h;
    uint8_t type;
    uint8_t enemies;
    uint8_t cleared;
};

struct Dungeon {
    Room rooms[MAX_ROOMS_C];
    uint8_t room_count;
    uint8_t connections[MAX_ROOMS_C][MAX_ROOMS_C];
    uint32_t seed;
    // тайлмапа: 0=пустота, 1=пол, 2=стена
    uint8_t tiles[DUNGEON_TILES];
};
