// tablet.cpp — логика подбираемых предметов: оружие, скрижали, здоровье
#include "tablet.h"
#include "../data/constants.h"
#include "../world/dungeon.h"

void pickup_spawn(Pickup* arr, vec2 pos, uint8_t kind, const Weapon& wpn, uint8_t tablet_flag) {
    int slot = -1;
    for (int i = 0; i < MAX_PICKUPS; i++) if (!arr[i].alive) { slot = i; break; }
    if (slot < 0) return;
    Pickup& pk = arr[slot];
    pk.alive = true;
    pk.pos = pos;
    pk.kind = kind;
    pk.weapon = wpn;
    pk.tablet_flag = tablet_flag;
    pk.bob = float(slot) * 0.7f;
}

bool pickups_update(Pickup* arr, Player& p, float dt, AudioEngine* audio) {
    bool got = false;
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup& pk = arr[i];
        if (!pk.alive) continue;
        pk.bob += dt * 3.0f;
        if (distance2d(pk.pos, p.base.pos) > PLAYER_RADIUS + 0.6f) continue;

        switch (pk.kind) {
            case 0: { // оружие -> в свободный слот или заменяет текущий
                int slot = (p.weapons[0].type == 255) ? 0 :
                           (p.weapons[1].type == 255) ? 1 : int(p.weapon_idx);
                p.weapons[slot] = pk.weapon;
                p.weapon_idx = uint8_t(slot);
                audio->sfx_pickup();
                break;
            }
            case 1: { // скрижаль -> применяется ко всем оружиям и к игроку
                p.tablet_mask |= pk.tablet_flag;
                p.weapons[0].tablets |= pk.tablet_flag;
                if (p.weapons[1].type != 255) p.weapons[1].tablets |= pk.tablet_flag;
                audio->sfx_tablet();
                break;
            }
            case 2: { // здоровье
                p.hp_f = minf(PLAYER_MAX_HP, p.hp_f + 35.0f);
                p.base.hp = int16_t(p.hp_f);
                audio->sfx_pickup();
                break;
            }
        }
        pk.alive = false;
        got = true;
    }
    return got;
}

void fill_treasure_room(const Dungeon& d, int room_i, Pickup* arr, Rng& rng, int floor_num) {
    const Room& rm = d.rooms[room_i];
    // 1-2 предмета: оружие / скрижаль / здоровье
    int items = 1 + int(rng.next() % 2);
    for (int k = 0; k < items; k++) {
        vec2 pos = DungeonGen::random_pos_in_room(d, room_i, rng);
        uint32_t roll = rng.next() % 100u;
        if (roll < 45u || floor_num <= 1) {
            // оружие: случайный из 4 типов
            int wt = int(rng.next() % 4);
            Weapon w{uint8_t(wt), WEAPON_DEFS[wt].dmg, WEAPON_DEFS[wt].rng, WEAPON_DEFS[wt].spd, 0};
            pickup_spawn(arr, pos, 0, w, 0);
        } else if (roll < 80u) {
            // скрижаль: одна из ещё не собранных
            static const uint8_t all_tabs[5] = {TAB_IMPULSE, TAB_RESONANCE, TAB_STASIS, TAB_FRAGMENT, TAB_ECHO};
            uint8_t flag = all_tabs[rng.range(0, 4)];
            Weapon dummy{};
            pickup_spawn(arr, pos, 1, dummy, flag);
        } else {
            Weapon dummy{};
            pickup_spawn(arr, pos, 2, dummy, 0);
        }
    }
    (void)rm;
}
