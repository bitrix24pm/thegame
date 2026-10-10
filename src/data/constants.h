// constants.h — константы и параметры баланса (все в constexpr для экономии)
#pragma once

// --- Мир / тайлы --- (MAX_ROOMS_C, DUNGEON_GRID_C объявлены в structs.h)
constexpr int TILE_SIZE = 1;              // 1 тайл = 1 мировая единица
constexpr int MAX_FLOORS = 5;             // этажей пустоты + финал

// --- Комната ---
enum RoomType : uint8_t { ROOM_START = 0, ROOM_NORMAL = 1, ROOM_TREASURE = 2, ROOM_BOSS = 3 };

// --- Игрок ---
constexpr float PLAYER_SPEED = 6.0f;
constexpr float PLAYER_RADIUS = 0.35f;
constexpr float PLAYER_MAX_HP = 100.0f;
constexpr float DASH_SPEED = 22.0f;
constexpr float DASH_TIME = 0.15f;        // сек
constexpr float DASH_CD = 1.2f;           // сек
constexpr float IFRAME_TIME = 0.25f;      // неуязвимость после удара

// --- Враги ---
constexpr int MAX_ENEMIES = 64;
constexpr int MAX_PROJECTILES = 128;
constexpr int MAX_PARTICLES = 512;
constexpr int MAX_RINGS = 16;
constexpr int MAX_TRAPS = 16;
constexpr int MAX_PICKUPS = 16;

// Оружие: type(0..3), damage, range(в десятых), speed(атак/сек*10)
struct WeaponDef { const char* name; uint8_t dmg; uint8_t rng; uint8_t spd; };
constexpr WeaponDef WEAPON_DEFS[4] = {
    {"BLADE",   18, 35, 25},   // конус ближний
    {"CHISEL",  12, 90, 15},   // луч/снаряд
    {"COMPASS", 15, 50, 10},   // кольцо AoE
    {"ANCHOR",  10, 30,  8},   // ловушка
};

// Скрижали
enum TabletFlag : uint8_t {
    TAB_IMPULSE   = 0x01,
    TAB_RESONANCE = 0x02,
    TAB_STASIS    = 0x04,
    TAB_FRAGMENT  = 0x08,
    TAB_ECHO      = 0x10,
};
constexpr const char* TABLET_NAMES[5] = {"IMPULSE", "RESONANCE", "STASIS", "FRAGMENT", "ECHO"};
constexpr const char* TABLET_DESCS[5] = {
    "Отталкивание врагов",
    "Взрыв при убийстве",
    "Замедление целей",
    "Осколки при попадании",
    "Призрачный повтор удара",
};

// Палитра "Архитектора Пустоты" — тёмно-синий/циан/маджента
constexpr uint32_t COL_BG       = 0x05060e;
constexpr uint32_t COL_FLOOR    = 0x0a0d1c;
constexpr uint32_t COL_GRID     = 0x2a4a7a;
constexpr uint32_t COL_PLAYER   = 0x7fe8ff;
constexpr uint32_t COL_ENEMY0   = 0xff5f7a; // осквернённый шип
constexpr uint32_t COL_ENEMY1   = 0xffb14f; // блуждающий глаз
constexpr uint32_t COL_ENEMY2   = 0xc26bff; // разломщик
constexpr uint32_t COL_BOSS     = 0xff2fd6;
constexpr uint32_t COL_HP       = 0x3dff8f;
constexpr uint32_t COL_TEXT     = 0xbfd8ff;
constexpr uint32_t COL_GOLD     = 0xffdf6b;

inline float rgba_r(uint32_t c) { return ((c >> 16) & 0xFF) / 255.0f; }
inline float rgba_g(uint32_t c) { return ((c >> 8) & 0xFF) / 255.0f; }
inline float rgba_b(uint32_t c) { return (c & 0xFF) / 255.0f; }
