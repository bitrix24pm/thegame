// player.h — игрок: движение (WASD), прицел мышью, рывок, слоты оружия
#pragma once
#include "../data/structs.h"
#include "../platform.h"
#include "combat.h"

// Инициализация игрока стартовым оружием
void player_init(Player& p);

// Обновление: ввод -> движение/рывок/атака/смена оружия. attack_out = true в кадр атаки.
bool player_update(Player& p, const InputState& in, vec2 aim_world, float dt, CombatWorld& w);
