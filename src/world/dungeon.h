// dungeon.h/cpp — процедурный генератор подземелья (комнаты + коридоры, МСТ-связность)
#pragma once
#include "../data/structs.h"
#include "../math/math_utils.h"

struct DungeonGen {
    // Сгенерировать подземелье; вернуть true при успехе.
    static bool generate(Dungeon& d, uint32_t seed, int floor_num);
    // Проверка проходимости тайла (пол)
    static inline bool walkable(const Dungeon& d, int tx, int ty) {
        if (tx < 0 || ty < 0 || tx >= DUNGEON_GRID_C || ty >= DUNGEON_GRID_C) return false;
        return d.tiles[ty * DUNGEON_GRID_C + tx] == 1;
    }
    // Коллизия круга с картой (мировые координаты: тайл (tx,ty) => центр (tx-23.5, ty-23.5))
    static bool circle_free(const Dungeon& d, float wx, float wz, float r);
    // Случайная позиция внутри комнаты i
    static vec2 random_pos_in_room(const Dungeon& d, int room_i, Rng& rng);
};
