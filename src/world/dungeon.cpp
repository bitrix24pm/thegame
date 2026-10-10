// dungeon.cpp — генерация комнат, коридоров, связность через MST (Прим)
#include "dungeon.h"
#include "../data/constants.h"
#include <cstring>
#include <cmath>

// тайл -> мировые координаты центра
static inline vec2 tile_center(int tx, int ty) {
    return vec2(tx - DUNGEON_GRID_C/2 + 0.5f, ty - DUNGEON_GRID_C/2 + 0.5f);
}
static inline void world_to_tile(float wx, float wz, int& tx, int& ty) {
    tx = int(std::floor(wx + DUNGEON_GRID_C/2.0f));
    ty = int(std::floor(wz + DUNGEON_GRID_C/2.0f));
}

bool DungeonGen::circle_free(const Dungeon& d, float wx, float wz, float r) {
    // проверяем 8 точек окружности + центр
    int tx, ty;
    world_to_tile(wx, wz, tx, ty);
    if (!walkable(d, tx, ty)) return false;
    for (int i = 0; i < 8; i++) {
        float a = TAU_F * i / 8;
        world_to_tile(wx + std::cos(a)*r, wz + std::sin(a)*r, tx, ty);
        if (!walkable(d, tx, ty)) return false;
    }
    return true;
}

vec2 DungeonGen::random_pos_in_room(const Dungeon& d, int room_i, Rng& rng) {
    const Room& rm = d.rooms[room_i];
    int tx = rm.x + 1 + rng.range(0, rm.w - 2);
    int ty = rm.y + 1 + rng.range(0, rm.h - 2);
    return tile_center(tx, ty);
}

int DungeonGen::room_at(const Dungeon& d, vec2 wp) {
    int tx, ty;
    world_to_tile(wp.x, wp.y, tx, ty);
    for (int i = 0; i < d.room_count; i++) {
        const Room& rm = d.rooms[i];
        if (tx >= rm.x && tx < rm.x + rm.w && ty >= rm.y && ty < rm.y + rm.h) return i;
    }
    return -1;
}

vec2 dungeon_exit_pos(const Dungeon& d) {
    const Room& rm = d.rooms[0];
    return tile_center(rm.x + rm.w / 2, rm.y + rm.h - 2);
}

static inline bool rooms_intersect(const Room& a, const Room& b, int pad = 1) {
    return !(a.x + a.w + pad <= b.x - pad || b.x + b.w + pad <= a.x - pad ||
             a.y + a.h + pad <= b.y - pad || b.y + b.h + pad <= a.y - pad);
}

static void carve_rect(Dungeon& d, int x0, int y0, int w, int h, uint8_t v) {
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++)
            if (x >= 0 && y >= 0 && x < DUNGEON_GRID_C && y < DUNGEON_GRID_C)
                d.tiles[y * DUNGEON_GRID_C + x] = v;
}

static void carve_corridor(Dungeon& d, int ax, int ay, int bx, int by) {
    // L-образный коридор шириной 2
    int x = ax, y = ay;
    while (x != bx) {
        carve_rect(d, x, y, 1, 2, 1);
        x += (bx > x) ? 1 : -1;
    }
    while (y != by) {
        carve_rect(d, x, y, 2, 1, 1);
        y += (by > y) ? 1 : -1;
    }
    carve_rect(d, bx, by, 2, 2, 1);
}

bool DungeonGen::generate(Dungeon& d, uint32_t seed, int floor_num) {
    Rng rng(seed * 2654435761u + uint32_t(floor_num) * 97u + 13u);
    memset(&d, 0, sizeof(Dungeon));
    d.seed = seed;

    // Смешиваем seed от этажа
    Rng r(rng.next());

    int count = 8 + int(r.next() % 5); // 8..12 комнат
    if (count > MAX_ROOMS_C) count = MAX_ROOMS_C;
    d.room_count = uint8_t(count);

    // 1. Стартовая комната в центре
    d.rooms[0].x = 20; d.rooms[0].y = 20;
    d.rooms[0].w = 7; d.rooms[0].h = 7;
    d.rooms[0].type = ROOM_START;
    d.rooms[0].enemies = 0;

    // 2. Остальные комнаты
    for (int i = 1; i < count; i++) {
        bool placed = false;
        for (int attempt = 0; attempt < 120 && !placed; attempt++) {
            Room c{};
            c.w = uint8_t(5 + r.next() % 6);   // 5..10
            c.h = uint8_t(5 + r.next() % 6);
            c.x = int16_t(2 + r.next() % (DUNGEON_GRID_C - c.w - 4));
            c.y = int16_t(2 + r.next() % (DUNGEON_GRID_C - c.h - 4));
            c.type = ROOM_NORMAL;
            c.enemies = uint8_t(2 + r.next() % 4);

            bool ok = true;
            for (int j = 0; j < i; j++)
                if (rooms_intersect(c, d.rooms[j])) { ok = false; break; }
            if (ok) { d.rooms[i] = c; placed = true; }
        }
        if (!placed) { // не удалось — урезаем количество
            d.room_count = uint8_t(i);
            count = i;
            break;
        }
    }
    if (count < 4) return false;

    // 3. Роли: последняя — босс, одна случайная средняя — клад
    d.rooms[count-1].type = ROOM_BOSS;
    d.rooms[count-1].enemies = 0;
    int treas = 1 + int(r.next() % (count - 2));
    d.rooms[treas].type = ROOM_TREASURE;
    d.rooms[treas].enemies = 1 + int(r.next() % 2);

    // Масштабируем сложность по этажу
    for (int i = 1; i < count - 1; i++) {
        if (d.rooms[i].type == ROOM_NORMAL)
            d.rooms[i].enemies = uint8_t(minf(6, int(d.rooms[i].enemies) + floor_num / 2));
    }

    // 4. Вырезаем комнаты
    for (int i = 0; i < count; i++) {
        Room& rm = d.rooms[i];
        carve_rect(d, rm.x, rm.y, rm.w, rm.h, 1);
    }

    // 5. Связность: для каждой комнаты соединяем с ближайшей из предыдущих (MST-lite)
    memset(d.connections, 0, sizeof(d.connections));
    for (int i = 1; i < count; i++) {
        int best = 0; float bd = 1e9f;
        vec2 ci = tile_center(d.rooms[i].x + d.rooms[i].w/2, d.rooms[i].y + d.rooms[i].h/2);
        for (int j = 0; j < i; j++) {
            vec2 cj = tile_center(d.rooms[j].x + d.rooms[j].w/2, d.rooms[j].y + d.rooms[j].h/2);
            float dist = (ci - cj).length_sq();
            if (dist < bd) { bd = dist; best = j; }
        }
        vec2 bi = tile_center(d.rooms[best].x + d.rooms[best].w/2, d.rooms[best].y + d.rooms[best].h/2);
        carve_corridor(d, int(ci.x), int(ci.y), int(bi.x), int(bi.y));
        d.connections[i][best] = 1;
        d.connections[best][i] = 1;
    }
    // Дополнительная петля для реиграбельности маршрутов
    if (count > 3) {
        int a = 1 + int(r.next() % (count - 2));
        int b = 1 + int(r.next() % (count - 2));
        if (a != b) {
            vec2 ca = tile_center(d.rooms[a].x + d.rooms[a].w/2, d.rooms[a].y + d.rooms[a].h/2);
            vec2 cb = tile_center(d.rooms[b].x + d.rooms[b].w/2, d.rooms[b].y + d.rooms[b].h/2);
            carve_corridor(d, int(ca.x), int(ca.y), int(cb.x), int(cb.y));
        }
    }

    // 6. Стены: любой пол, соседний с пустотой, обрамляется стеной (для рендера краёв)
    //    (тайлы стен = 2; в пустоте остаётся 0)
    static uint8_t tmp[DUNGEON_TILES];
    memcpy(tmp, d.tiles, DUNGEON_TILES);
    for (int y = 1; y < DUNGEON_GRID_C-1; y++)
        for (int x = 1; x < DUNGEON_GRID_C-1; x++) {
            if (tmp[y*DUNGEON_GRID_C+x] != 0) continue;
            bool near_floor = false;
            for (int dy=-1; dy<=1 && !near_floor; dy++)
                for (int dx=-1; dx<=1; dx++)
                    if (tmp[(y+dy)*DUNGEON_GRID_C+(x+dx)] == 1) { near_floor = true; break; }
            if (near_floor) d.tiles[y*DUNGEON_GRID_C+x] = 2;
        }
    return true;
}
