// main.cpp — Architect of the Void. Точка входа, главный цикл, состояния игры.
// Чистая демосцена: C++17 + OpenGL 3.3 + SDL2. Никаких ассетов — только код и математика.
#include "platform.h"
#include "render/renderer.h"
#include "world/dungeon.h"
#include "game/player.h"
#include "game/enemy.h"
#include "game/combat.h"
#include "game/tablet.h"
#include "procedural/sound.h"
#include "data/constants.h"
#include <cstdio>
#include <cstring>
#include <cmath>

static void render_hud();  // forward decl (используется в render_world)

// ---------------- Глобальное состояние (всё статично, без аллокаций) ----------------
static Platform      g_platform;
static Renderer      g_renderer;
static AudioEngine   g_audio;
static Dungeon       g_dungeon;
static Player        g_player;
static Enemy         g_enemies[MAX_ENEMIES];
static Projectile    g_projectiles[MAX_PROJECTILES];
static Ring          g_rings[MAX_RINGS];
static Trap          g_traps[MAX_TRAPS];
static Particle      g_particles[MAX_PARTICLES];
static Pickup        g_pickups[MAX_PICKUPS];
static int           g_enemy_count = 0;
static CombatWorld   g_world;

static uint32_t g_seed = 0;
static int      g_floor = 1;
static float    g_time = 0;
static bool     g_show_debug = false;
static int      g_kills = 0;

enum GameState { ST_MENU, ST_PLAY, ST_DEAD, ST_WIN };
static GameState g_state = ST_MENU;
static float g_boss_intro_timer = 0;

// ---------------- Аудио-колбэк ----------------
static void audio_cb(int16_t* stream, int samples) { g_audio.mix(stream, samples); }

// ---------------- Инициализация мира ----------------
static void reset_dynamic() {
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_projectiles, 0, sizeof(g_projectiles));
    memset(g_rings, 0, sizeof(g_rings));
    memset(g_traps, 0, sizeof(g_traps));
    memset(g_particles, 0, sizeof(g_particles));
    memset(g_pickups, 0, sizeof(g_pickups));
    g_enemy_count = 0;
}

static void bind_world() {
    g_world.player = &g_player;
    g_world.enemies = g_enemies;
    g_world.enemy_count = &g_enemy_count;
    g_world.projectiles = g_projectiles;
    g_world.rings = g_rings;
    g_world.traps = g_traps;
    g_world.particles = g_particles;
    g_world.audio = &g_audio;
    g_world.dungeon = &g_dungeon;
}

static void populate_room(const Room& rm, int room_i, Rng& rng) {
    if (rm.type == ROOM_TREASURE) {
        fill_treasure_room(g_dungeon, room_i, g_pickups, rng, g_floor);
        return;
    }
    if (rm.type == ROOM_BOSS) {
        vec2 c = DungeonGen::random_pos_in_room(g_dungeon, room_i, rng);
        spawn_enemy(g_world, c, 2, g_floor, true);
        g_audio.sfx_boss();
        return;
    }
    for (int i = 0; i < rm.enemies; i++) {
        vec2 pos = DungeonGen::random_pos_in_room(g_dungeon, room_i, rng);
        uint8_t type = uint8_t(rng.next() % 3);
        spawn_enemy(g_world, pos, type, g_floor, false);
    }
}

static void load_floor(int floor_num) {
    g_floor = floor_num;
    // перебор сидов, пока генерация не удастся
    for (int attempt = 0; attempt < 16; attempt++) {
        if (DungeonGen::generate(g_dungeon, g_seed + uint32_t(floor_num * 7919u + attempt), floor_num)) break;
    }
    reset_dynamic();
    Rng rng(g_seed ^ uint32_t(floor_num * 104729u));
    for (int i = 1; i < g_dungeon.room_count; i++)
        populate_room(g_dungeon.rooms[i], i, rng);
    // игрок в центр стартовой комнаты
    const Room& r0 = g_dungeon.rooms[0];
    g_player.base.pos = vec2(r0.x + r0.w / 2 - DUNGEON_GRID_C / 2 + 0.5f,
                             r0.y + r0.h / 2 - DUNGEON_GRID_C / 2 + 0.5f);
    g_player.base.vel = vec2();
    g_renderer.camera.target = g_player.base.pos;
    g_audio.sfx_door();
}

static void new_game() {
    g_seed = uint32_t(SDL_GetTicks()) * 2654435761u ^ 0x9E3779B9u;
    player_init(g_player);
    g_kills = 0;
    g_time = 0;
    load_floor(1);
    g_state = ST_PLAY;
}

// Проверка: все ли враги на этаже мертвы; открывает выход (визуально — портал).
static bool floor_cleared() {
    for (int i = 0; i < g_enemy_count; i++)
        if (g_enemies[i].base.state != E_DEAD) return false;
    return g_enemy_count > 0 || g_dungeon.rooms[g_dungeon.room_count - 1].type == ROOM_BOSS;
}

static bool boss_dead() {
    for (int i = 0; i < g_enemy_count; i++)
        if (g_enemies[i].is_boss && g_enemies[i].base.state != E_DEAD) return false;
    return true;
}

// ---------------- Обновление геймплея ----------------
static void update_play(float dt, const InputState& in) {
    vec2 aim_world = g_renderer.camera.screen_to_world(float(in.mouse_x), float(in.mouse_y),
                                                       g_platform.width, g_platform.height);
    player_update(g_player, in, aim_world, dt, g_world);
    EnemyAI::update_all(g_world, dt, g_time);
    update_projectiles(g_world, dt);
    update_rings(g_world, dt);
    update_traps(g_world, dt);
    update_particles(g_world, dt);
    pickups_update(g_pickups, g_player, dt, &g_audio);

    // учёт убийств (для финала)
    static int last_alive = 0;
    int alive = 0;
    for (int i = 0; i < g_enemy_count; i++) if (g_enemies[i].base.state != E_DEAD) alive++;
    if (alive < last_alive) g_kills += last_alive - alive;
    last_alive = alive;

    // --- переход этажа: встать на пентаграмму выхода при зачищенной карте ---
    vec2 exit_pos = dungeon_exit_pos(g_dungeon);
    bool can_exit = false;
    if (g_floor >= MAX_FLOORS) {
        // финальный этаж: выход появляется только после смерти босса
        can_exit = boss_dead() && distance2d(g_player.base.pos, exit_pos) < 1.2f;
    } else {
        can_exit = distance2d(g_player.base.pos, exit_pos) < 1.2f;
    }
    if (can_exit) {
        if (g_floor >= MAX_FLOORS) {
            g_state = ST_WIN;
            g_audio.sfx_tablet();
        } else {
            load_floor(g_floor + 1);
        }
    }

    // смерть игрока
    if (g_player.hp_f <= 0) {
        g_state = ST_DEAD;
        g_audio.sfx_death();
    }

    g_renderer.camera.update(g_player.base.pos, dt);
}

// ---------------- Отрисовка мира ----------------
static uint32_t enemy_color(const Enemy& e) {
    if (e.is_boss) return COL_BOSS;
    return e.base.type == 0 ? COL_ENEMY0 : e.base.type == 1 ? COL_ENEMY1 : COL_ENEMY2;
}

static void draw_entity_shape(vec2 pos, float radius, uint32_t color, float spin, float lift,
                              bool is_boss, uint8_t type) {
    Renderer& R = g_renderer;
    vec3 c(pos.x, lift, pos.y);
    if (is_boss) {
        R.mesh_icosahedron(vec3(pos.x, 1.6f + std::sin(spin * 0.7f) * 0.3f, pos.y), 1.5f, color, 1.0f, spin);
        R.circle_outline(pos, radius + 0.5f, 0.12f, color, 0.5f, 40);
    } else if (type == 0) {
        // шип: маленький икосаэдр
        R.mesh_icosahedron(vec3(pos.x, 0.55f + lift * 0.2f, pos.y), 0.5f, color, 1.0f, spin);
    } else if (type == 1) {
        // глаз: кольцо + зрачок
        R.circle_outline(pos, radius, 0.14f, color, 0.9f, 24);
        R.quad(vec3(pos.x, 0.5f, pos.y), 0.16f, 0.16f, color, 1.0f, 0, 0.55f);
    } else {
        // разломщик: квадрат-плита
        R.quad(vec3(pos.x, 0.45f, pos.y), radius, radius, color, 0.95f, spin, 0.45f);
    }
}

static void render_world() {
    Renderer& R = g_renderer;
    R.begin_frame(g_time);
    R.camera.update(g_player.base.pos, 0.0f); // пересчитать vp для текущего кадра
    R.draw_floor(g_dungeon, g_time, g_player.base.pos);

    // --- выход (пентаграмма) ---
    vec2 exit_pos = dungeon_exit_pos(g_dungeon);
    bool open = (g_floor >= MAX_FLOORS) ? boss_dead() : true;
    uint32_t exit_col = open ? COL_GOLD : 0x334455;
    R.circle_outline(exit_pos, 1.0f, 0.1f, exit_col, open ? 0.8f : 0.3f, 32);
    R.circle_outline(exit_pos, 0.6f, 0.08f, exit_col, open ? 0.6f : 0.2f, 24);
    if (open) {
        // лучи
        for (int k = 0; k < 5; k++) {
            float a = TAU_F * k / 5 + g_time * 0.5f;
            R.line(exit_pos, exit_pos + vec2(std::cos(a), std::sin(a)) * 0.9f, 0.06f, exit_col, 0.5f);
        }
    }

    // --- ловушки ANCHOR ---
    for (int i = 0; i < MAX_TRAPS; i++) {
        Trap& t = g_traps[i];
        if (!t.alive) continue;
        float pulse = 0.5f + 0.5f * std::sin(g_time * 6.0f);
        R.circle_outline(t.pos, t.radius, 0.08f, 0xff8f4f, 0.35f + 0.25f * pulse, 28);
        R.quad(vec3(t.pos.x, 0, t.pos.y), 0.25f, 0.25f, 0xff8f4f, 0.8f, g_time * 2.0f, 0.02f);
    }

    // ---Pickup-ы---
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup& pk = g_pickups[i];
        if (!pk.alive) continue;
        float bob = 0.4f + 0.15f * std::sin(pk.bob);
        uint32_t col = pk.kind == 0 ? COL_GOLD : pk.kind == 1 ? 0x6bfff0 : COL_HP;
        R.billboard_shadow(pk.pos, 0.3f, 0.35f);
        if (pk.kind == 1) {
            R.mesh_icosahedron(vec3(pk.pos.x, bob + 0.3f, pk.pos.y), 0.35f, col, 1.0f, pk.bob);
        } else {
            R.quad(vec3(pk.pos.x, 0, pk.pos.y), 0.3f, 0.3f, col, 0.9f, pk.bob * 0.5f, bob);
        }
    }

    // --- враги (сортировка по экранной глубине x+y для корректного перекрытия) ---
    int order[MAX_ENEMIES]; int n_order = 0;
    for (int i = 0; i < g_enemy_count; i++) {
        if (g_enemies[i].base.state == E_DEAD) continue;
        order[n_order++] = i;
    }
    // простой insertion sort (n<=64)
    for (int a = 1; a < n_order; a++) {
        int v = order[a]; int b = a - 1;
        vec2 pv = g_enemies[v].base.pos;
        while (b >= 0) {
            vec2 qu = g_enemies[order[b]].base.pos;
            if (!(qu.x + qu.y > pv.x + pv.y)) break;
            order[b + 1] = order[b]; b--;
        }
        order[b + 1] = v;
    }
    for (int oi = 0; oi < n_order; oi++) {
        Enemy& e = g_enemies[order[oi]];
        uint32_t col = enemy_color(e);
        float spin = g_time * 2.0f + float(order[oi]);
        R.billboard_shadow(e.base.pos, e.radius * 1.4f, 0.4f);
        // телеграф атаки
        if (e.base.state == E_ATTACK_PREP) {
            float f = e.state_timer / (e.is_boss ? 0.45f : 0.55f);
            R.circle_outline(e.base.pos, e.radius + 1.2f * (1.0f - f), 0.08f, 0xffffff, 0.5f, 24);
        }
        draw_entity_shape(e.base.pos, e.radius, col, spin, 0.0f, e.is_boss, e.base.type);
        // хп-бар над врагом (в мире)
        if (e.base.hp < e.base.max_hp) {
            float wdt = e.is_boss ? 2.0f : 0.8f;
            vec2 a(e.base.pos.x - wdt / 2, e.base.pos.y - 0.01f);
            vec2 b(e.base.pos.x - wdt / 2 + wdt * float(e.base.hp) / float(e.base.max_hp), e.base.pos.y - 0.01f);
            R.line(a, b, 0.1f, COL_HP, 0.9f);
        }
    }

    // --- снаряды ---
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile& pr = g_projectiles[i];
        if (!pr.alive) continue;
        uint32_t col = pr.from_player ? COL_PLAYER : COL_ENEMY0;
        R.quad(vec3(pr.pos.x, 0, pr.pos.y), 0.14f, 0.14f, col, 1.0f, 0, 0.35f);
        R.line(pr.pos - pr.vel * 0.03f, pr.pos, 0.06f, col, 0.4f);
    }

    // --- кольца COMPASS ---
    for (int i = 0; i < MAX_RINGS; i++) {
        Ring& r = g_rings[i];
        if (!r.alive) continue;
        float fade = 1.0f - r.radius / r.max_radius;
        R.circle_outline(r.center, r.radius, 0.12f, 0x6bfff0, 0.35f + 0.5f * fade, 40);
    }

    // --- частицы ---
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = g_particles[i];
        if (!p.alive) continue;
        float a = clampf(p.life / p.max_life, 0.0f, 1.0f);
        R.quad(vec3(p.pos.x, 0, p.pos.z), p.size, p.size, p.color, a, 0, p.pos.y);
    }

    // --- игрок: икосаэдр + указатель прицела + конус BLADE превью ---
    Player& P = g_player;
    R.billboard_shadow(P.base.pos, PLAYER_RADIUS * 1.6f, 0.45f);
    float blink = (P.iframes > 0 && int(g_time * 20) % 2 == 0) ? 0.35f : 1.0f;
    R.mesh_icosahedron(vec3(P.base.pos.x, 0.7f, P.base.pos.y), 0.5f, COL_PLAYER, blink, g_time * 1.5f);
    // линия прицела
    R.line(P.base.pos, P.base.pos + P.aim * 1.2f, 0.05f, COL_PLAYER, 0.5f);
    // превью текущей атаки
    Weapon& wpn = P.weapons[P.weapon_idx];
    if (wpn.type != 255) {
        float range = wpn.range / 10.0f;
        if (wpn.type == 0) R.cone(P.base.pos, P.aim, range, radians(50.0f), COL_PLAYER, 0.10f);
        if (wpn.type == 2) R.circle_outline(P.base.pos, range, 0.06f, 0x6bfff0, 0.12f, 32);
        if (wpn.type == 3) R.circle_outline(P.base.pos + P.aim * 1.2f, range * 0.5f, 0.06f, 0xff8f4f, 0.12f, 24);
    }

    R.end_frame();
    render_hud();
}

// ---------------- HUD ----------------
static char g_buf[128];
static void render_hud() {
    Renderer& R = g_renderer;
    // HP
    R.hud_bar(20, R.screen_h - 40, 260, 20, g_player.hp_f / PLAYER_MAX_HP, COL_HP);
    snprintf(g_buf, sizeof(g_buf), "HP %d", int(maxf(0, g_player.hp_f)));
    R.text(20, R.screen_h - 66, g_buf, COL_TEXT, 2.0f);
    // dash cd
    float dash_ready = 1.0f - clampf(g_player.dash_cd / DASH_CD, 0.0f, 1.0f);
    R.hud_bar(300, R.screen_h - 40, 120, 20, dash_ready, COL_PLAYER);
    R.text(300, R.screen_h - 66, "DASH", COL_TEXT, 2.0f);

    // оружие
    for (int s = 0; s < 2; s++) {
        Weapon& w = g_player.weapons[s];
        int x = 20 + s * 240;
        uint32_t col = s == g_player.weapon_idx ? COL_GOLD : 0x44557a;
        const char* name = (w.type == 255) ? "-" : WEAPON_DEFS[w.type & 3].name;
        snprintf(g_buf, sizeof(g_buf), "%d:%s", s + 1, name);
        R.text(x, 20, g_buf, col, 2.2f);
        if (w.tablets | g_player.tablet_mask) {
            // список скрижалей коротко: буквы I R S F E
            uint8_t m = w.tablets | g_player.tablet_mask;
            char tabs[8] = {0}; int ti = 0;
            if (m & TAB_IMPULSE)   tabs[ti++] = 'I';
            if (m & TAB_RESONANCE) tabs[ti++] = 'R';
            if (m & TAB_STASIS)    tabs[ti++] = 'S';
            if (m & TAB_FRAGMENT)  tabs[ti++] = 'F';
            if (m & TAB_ECHO)      tabs[ti++] = 'E';
            R.text(x, 46, tabs, 0x6bfff0, 2.0f);
        }
    }

    // этаж/комнаты
    snprintf(g_buf, sizeof(g_buf), "VOID %d/%d  KILLS %d", g_floor, MAX_FLOORS, g_kills);
    R.text(R.screen_w - 320, 20, g_buf, COL_TEXT, 2.2f);

    // цель
    bool any_alive = false;
    for (int i = 0; i < g_enemy_count; i++)
        if (g_enemies[i].base.state != E_DEAD) any_alive = true;
    if (!any_alive && g_floor < MAX_FLOORS)
        R.text(R.screen_w / 2 - 190, R.screen_h - 110, "ROOMS CLEARED - GO TO PORTAL", COL_GOLD, 2.0f);
    else if (g_floor >= MAX_FLOORS && boss_dead())
        R.text(R.screen_w / 2 - 160, R.screen_h - 110, "THE ARCHITECT FALLS - ENTER VOID", COL_GOLD, 2.0f);

    if (g_show_debug) {
        static int fps_frames = 0; static float fps_t = 0; static float fps_val = 0;
        fps_frames++; fps_t += 1.0f / 60.0f;
        snprintf(g_buf, sizeof(g_buf), "FPS ~%d ENEMIES %d PARTS %d", int(fps_val), g_enemy_count, 0);
        R.text(20, R.screen_h - 100, g_buf, 0x88ff88, 2.0f);
    }
}

// ---------------- Экраны меню/финалов ----------------
static void render_menu() {
    Renderer& R = g_renderer;
    R.begin_frame(g_time);
    // декоративный фон: сетка квадов
    mat4 saved = R.camera.vp;
    R.camera.vp = mat4::identity();
    R.end_frame();
    R.camera.vp = saved;

    R.text(R.screen_w / 2 - 260, 120, "ARCHITECT OF THE VOID", COL_PLAYER, 4.0f);
    R.text(R.screen_w / 2 - 200, 190, "ISOMETRIC ACTION-RPG IN THE DARK", COL_TEXT, 2.2f);
    R.text(R.screen_w / 2 - 170, 300, "WASD MOVE  MOUSE AIM  LMB ATTACK", COL_TEXT, 2.2f);
    R.text(R.screen_w / 2 - 150, 330, "SPACE DASH  1/2/Q WEAPONS", COL_TEXT, 2.2f);
    R.text(R.screen_w / 2 - 190, 400, "PURIFY 5 LAYERS OF THE VOID", COL_GOLD, 2.2f);
    R.text(R.screen_w / 2 - 120, 430, "KILL THE ARCHITECT", COL_BOSS, 2.2f);
    snprintf(g_buf, sizeof(g_buf), "PRESS ENTER TO BEGIN");
    R.text(R.screen_w / 2 - 150, 520, (int(g_time * 2) % 2) ? g_buf : "", COL_HP, 2.4f);
    R.text(R.screen_w / 2 - 120, 600, "SEED ", COL_TEXT, 2.0f);
}

static void render_end(bool win) {
    Renderer& R = g_renderer;
    R.begin_frame(g_time);
    const char* title = win ? "VOID REBUILT" : "YOU ARE UNWRITTEN";
    uint32_t col = win ? COL_GOLD : COL_BOSS;
    R.text(R.screen_w / 2 - 220, 160, title, col, 4.0f);

    // 3 варианта финала: по числу собранных скрижалей и убитых
    if (win) {
        int tabs = 0;
        uint8_t m = g_player.tablet_mask;
        for (int b = 0; b < 5; b++) if (m & (1 << b)) tabs++;
        const char* ending;
        if (tabs >= 4)      ending = "ENDING A - NEW ARCHITECT: YOU KEEP THE TABLETS";
        else if (g_kills > 60) ending = "ENDING B - THE HOLLOW KING: ALL NAMES ERASED";
        else                ending = "ENDING C - QUIET DAWN: THE VOID FORGIVES";
        R.text(R.screen_w / 2 - 300, 260, ending, COL_TEXT, 2.2f);
    } else {
        R.text(R.screen_w / 2 - 260, 260, "THE GEOMETRY CONSUMED YOU", COL_TEXT, 2.2f);
    }
    snprintf(g_buf, sizeof(g_buf), "FLOOR %d  KILLS %d", g_floor, g_kills);
    R.text(R.screen_w / 2 - 130, 340, g_buf, COL_TEXT, 2.2f);
    R.text(R.screen_w / 2 - 210, 460, "ENTER - REBUILD FROM SEED  ESC - MENU",
           (int(g_time * 2) % 2) ? COL_HP : 0x101528, 2.2f);
    R.end_frame();
}

// ---------------- Главный цикл ----------------
int main(int argc, char** argv) {
    if (!g_platform.init("Architect of the Void", 1280, 720)) {
        printf("[fatal] platform init failed\n");
        return 1;
    }
    if (!g_renderer.init(g_platform.width, g_platform.height, &Platform::get_proc)) {
        printf("[fatal] renderer init failed\n");
        return 1;
    }
    init_camera(g_renderer.camera, float(g_platform.width) / float(g_platform.height));
    if (!audio_init(&audio_cb)) printf("[warn] audio unavailable\n");

    bind_world();
    // черновик подземелья для меню-фона
    g_seed = uint32_t(SDL_GetTicks()) * 2654435761u;
    player_init(g_player);
    DungeonGen::generate(g_dungeon, g_seed, 1);
    const Room& r0 = g_dungeon.rooms[0];
    g_player.base.pos = vec2(r0.x + r0.w / 2 - DUNGEON_GRID_C / 2 + 0.5f,
                             r0.y + r0.h / 2 - DUNGEON_GRID_C / 2 + 0.5f);
    g_renderer.camera.target = g_player.base.pos;

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 prev = SDL_GetPerformanceCounter();
    InputState in{};
    float fps_acc = 0; int fps_cnt = 0; float fps_shown = 0;

    bool running = true;
    while (running) {
        running = g_platform.poll_events(in);
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = float(double(now - prev) / double(freq));
        prev = now;
        if (dt > 0.05f) dt = 0.05f; // защита от свинпа
        g_time += dt;

        // FPS
        fps_acc += dt; fps_cnt++;
        if (fps_acc >= 0.5f) { fps_shown = float(fps_cnt) / fps_acc; fps_acc = 0; fps_cnt = 0; }

        if (in.key_pressed[SDL_SCANCODE_ESCAPE]) {
            if (g_state == ST_PLAY) g_state = ST_MENU;
            else if (g_state != ST_MENU) g_state = ST_MENU;
        }
        if (in.key_pressed[SDL_SCANCODE_F1]) g_show_debug = !g_show_debug;

        switch (g_state) {
            case ST_MENU:
                if (in.key_pressed[SDL_SCANCODE_RETURN] || in.lmb_pressed) new_game();
                render_menu();
                break;
            case ST_PLAY: {
                update_play(dt, in);
                render_world();
                if (g_show_debug) {
                    snprintf(g_buf, sizeof(g_buf), "FPS %.0f  ENT %d", fps_shown, g_enemy_count);
                    g_renderer.text(20, g_renderer.screen_h - 100, g_buf, 0x88ff88, 2.0f);
                }
                break;
            }
            case ST_DEAD:
                if (in.key_pressed[SDL_SCANCODE_RETURN]) new_game();
                render_end(false);
                break;
            case ST_WIN:
                if (in.key_pressed[SDL_SCANCODE_RETURN]) new_game();
                render_end(true);
                break;
        }
        g_platform.swap();
    }

    audio_shutdown();
    g_renderer.shutdown();
    g_platform.shutdown();
    return 0;
}
