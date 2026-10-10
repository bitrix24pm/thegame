// sound.h — собственный FM-синтезатор + генеративная музыка. Без сэмплов.
#pragma once
#include <cstdint>
#include <cmath>
#include <vector>
#include "../math/math_utils.h"

constexpr int SAMPLE_RATE = 44100;
constexpr int MAX_VOICES = 12;

struct FMSynth {
    float carrier_freq;
    float modulator_freq;
    float modulator_amp;
    float attack, decay, sustain, release;
};

// Активный голос (инстанс звука)
struct Voice {
    bool active = false;
    FMSynth p;
    float phase = 0, mod_phase = 0;
    float t = 0;          // время с начала
    float duration = 0;   // суммарная длительность ADSR
    float freq_slide = 0; // Гц/сек (для "вжух" эффектов)
    float noise_amt = 0;  // подмесь шума (перкуссия)
    float rng_state = 1;
};

class AudioEngine {
public:
    // --- Инициализация SDL-аудио выполняется снаружи; здесь микшер ---
    void mix(int16_t* stream, int samples) {
        for (int i = 0; i < samples; i++) stream[i] = 0;
        for (int v = 0; v < MAX_VOICES; v++) {
            if (!voices[v].active) continue;
            mix_voice(voices[v], stream, samples);
        }
        mix_music(stream, samples);
    }

    void play(const FMSynth& s, float duration, float slide = 0, float noise = 0) {
        // найти свободный голос или самый старый
        int idx = -1;
        for (int i = 0; i < MAX_VOICES; i++) if (!voices[i].active) { idx = i; break; }
        if (idx < 0) { idx = 0; float best = -1;
            for (int i = 0; i < MAX_VOICES; i++) if (voices[i].t > best) { best = voices[i].t; idx = i; } }
        Voice& v = voices[idx];
        v.active = true; v.p = s; v.phase = 0; v.mod_phase = 0; v.t = 0;
        v.duration = duration; v.freq_slide = slide; v.noise_amt = noise;
        v.rng_state = 12345.f + float(idx);
    }

    // --- Библиотека звуков ---
    static FMSynth hit()    { return {220, 110, 50, 0.005f, 0.08f, 0.0f, 0.05f}; }
    static FMSynth swing()  { return {160, 240, 90, 0.01f, 0.09f, 0.0f, 0.05f}; }
    static FMSynth shoot()  { return {880, 440, 300, 0.002f, 0.06f, 0.0f, 0.02f}; }
    static FMSynth ring_atk(){return {120, 60, 200, 0.01f, 0.25f, 0.1f, 0.2f}; }
    static FMSynth trap_set(){ return {330, 165, 40, 0.02f, 0.15f, 0.2f, 0.1f}; }
    static FMSynth death()  { return {140, 70, 120, 0.005f, 0.3f, 0.0f, 0.2f}; }
    static FMSynth hurt()   { return {90, 45, 60, 0.005f, 0.2f, 0.0f, 0.1f}; }
    static FMSynth dash()   { return {500, 900, 400, 0.005f, 0.12f, 0.0f, 0.05f}; }
    static FMSynth pickup() { return {660, 990, 60, 0.005f, 0.1f, 0.3f, 0.15f}; }
    static FMSynth tablet_s(){return {440, 880, 150, 0.01f, 0.3f, 0.2f, 0.3f}; }
    static FMSynth boss_roar(){return {60, 30, 80, 0.1f, 0.5f, 0.6f, 0.5f}; }
    static FMSynth door()   { return {200, 100, 30, 0.05f, 0.3f, 0.1f, 0.2f}; }
    static FMSynth explode(){ return {80, 40, 300, 0.002f, 0.35f, 0.0f, 0.2f}; }

    void sfx_hit()     { play(hit(), 0.2f, 0, 0.4f); }
    void sfx_swing()   { play(swing(), 0.15f, -300, 0.2f); }
    void sfx_shoot()   { play(shoot(), 0.12f, -600, 0); }
    void sfx_ring()    { play(ring_atk(), 0.5f, 100, 0); }
    void sfx_trap()    { play(trap_set(), 0.4f, 0, 0); }
    void sfx_death()   { play(death(), 0.6f, -80, 0.5f); }
    void sfx_hurt()    { play(hurt(), 0.4f, -60, 0.6f); }
    void sfx_dash()    { play(dash(), 0.2f, -400, 0.3f); }
    void sfx_pickup()  { play(pickup(), 0.35f, 0, 0); }
    void sfx_tablet()  { play(tablet_s(), 0.9f, 50, 0); }
    void sfx_boss()    { play(boss_roar(), 1.4f, 20, 0.3f); }
    void sfx_door()    { play(door(), 0.6f, 0, 0.1f); }
    void sfx_explode() { play(explode(), 0.7f, -40, 0.9f); }

    // --- Генеративная фоновая музыка: тёмный эмбиент-дрон + арпеджио ---
    void set_music(bool on) { music_on = on; }
    bool music_enabled() const { return music_on; }

private:
    Voice voices[MAX_VOICES];
    bool music_on = true;

    void mix_voice(Voice& v, int16_t* stream, int samples) {
        const FMSynth& p = v.p;
        float total = p.attack + p.decay + p.release + 0.02f;
        float dur = v.duration > 0 ? v.duration : total;
        for (int i = 0; i < samples; i++) {
            float t = v.t;
            if (t >= dur) { v.active = false; return; }
            // ADSR
            float env;
            if (t < p.attack) env = t / maxf(p.attack, 1e-4f);
            else if (t < p.attack + p.decay) {
                float dt = (t - p.attack) / maxf(p.decay, 1e-4f);
                env = 1.0f - dt * (1.0f - p.sustain);
            } else if (t < dur - p.release) env = p.sustain;
            else {
                float rt = (dur - t) / maxf(p.release, 1e-4f);
                env = p.sustain * clampf(rt, 0, 1);
            }
            float freq = p.carrier_freq + v.freq_slide * t;
            v.mod_phase += TAU_F * p.modulator_freq / SAMPLE_RATE;
            float mod = std::sin(v.mod_phase) * p.modulator_amp;
            v.phase += TAU_F * (freq + mod) / SAMPLE_RATE;
            float s = std::sin(v.phase) * env;
            if (v.noise_amt > 0) {
                // быстрый LCG-шум
                v.rng_state = std::fmod(v.rng_state * 1103515245.0f + 12345.0f, 2147483648.0f);
                float n = (v.rng_state / 2147483648.0f) * 2.0f - 1.0f;
                s = s * (1 - v.noise_amt) + n * env * v.noise_amt;
            }
            add_sample(stream, i, s * 0.5f);
            v.t += 1.0f / SAMPLE_RATE;
        }
    }

    // Музыкальный секвенсор: 8-нотное арпеджио минорного лада + дрон
    int music_step_time = 0; // отсчитываемые сэмплы
    int music_note = 0;
    float drone_phase = 0, drone_phase2 = 0;
    float arp_phase = 0;
    float arp_env = 0;

    void mix_music(int16_t* stream, int samples) {
        if (!music_on) return;
        static const float scale[8] = {55.f, 65.41f, 73.42f, 82.41f, 110.f, 130.81f, 146.83f, 164.81f};
        const int note_len = SAMPLE_RATE / 4; // ~0.25 c на ноту
        for (int i = 0; i < samples; i++) {
            // Дрон: два расстроенных осциллятора
            drone_phase += TAU_F * 55.0f / SAMPLE_RATE;
            drone_phase2 += TAU_F * 55.3f / SAMPLE_RATE;
            float drone = (std::sin(drone_phase) + std::sin(drone_phase2)) * 0.045f;

            // Арпеджио огибающая
            if (music_step_time <= 0) {
                music_step_time = note_len;
                // псевдослучайный выбор ступени
                music_note = (music_note * 7 + 3) & 7;
                arp_env = 1.0f;
                arp_freq_cur = scale[music_note] * 2.0f;
            }
            music_step_time--;
            arp_env *= 0.99992f;
            arp_phase += TAU_F * arp_freq_cur / SAMPLE_RATE;
            float arp = std::sin(arp_phase) * arp_env * 0.06f;

            add_sample(stream, i, drone + arp);
        }
    }
    float arp_freq_cur = 110.f;

    static void add_sample(int16_t* stream, int i, float s) {
        int v = int(s * 32767.0f) + stream[i];
        stream[i] = int16_t(clampf(float(v), -32000.f, 32000.f));
    }
};
