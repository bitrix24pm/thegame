// tablet.h/cpp — скрижали-модификаторы и подбираемые предметы
#pragma once
#include "../data/structs.h"
#include "../math/math_utils.h"

// Положить предмет в массив (переиспользует мёртвый слот)
void pickup_spawn(Pickup* arr, vec2 pos, uint8_t kind, const Weapon& wpn, uint8_t tablet_flag);

// Проверить подбор игроком; возвращает true, если что-то подобрано.
bool pickups_update(Pickup* arr, Player& p, float dt, AudioEngine* audio);

// Сгенерировать содержимое комнаты кладов (по этажу)
void fill_treasure_room(const Dungeon& d, int room_i, Pickup* arr, Rng& rng, int floor_num);
