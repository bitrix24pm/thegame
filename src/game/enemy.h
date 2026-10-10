// enemy.h/cpp — ИИ врагов: конечный автомат, 3 типа + босс, телеграф атак
#pragma once
#include "../data/structs.h"
#include "combat.h"

struct EnemyAI {
    // Обновить всех врагов (движение, FSM, атаки)
    static void update_all(CombatWorld& w, float dt, float time_sec);
};

// Спавн врага; type 0..2 или boss
void spawn_enemy(CombatWorld& w, vec2 pos, uint8_t type, int floor_num, bool is_boss = false);
