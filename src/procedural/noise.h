// noise.h — собственный Perlin-шум (2D), без внешних библиотек
#pragma once
#include <cstdint>
#include "../math/math_utils.h"

class Noise {
public:
    explicit Noise(uint32_t seed = 1234u) { reseed(seed); }

    void reseed(uint32_t seed) {
        Rng r(seed);
        for (int i = 0; i < 256; i++) perm[i] = uint8_t(i);
        for (int i = 255; i > 0; i--) {
            int j = int(r.next() % uint32_t(i + 1));
            uint8_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
        }
        for (int i = 0; i < 256; i++) perm512[i] = perm[i & 255], perm512[256 + i] = perm[i];
    }

    // Классический градиентный шум Перлина, результат ~[-1,1]
    float noise2(float x, float y) const {
        int xi = fastfloor(x) & 255, yi = fastfloor(y) & 255;
        float xf = x - std::floor(x), yf = y - std::floor(y);
        float u = fade(xf), v = fade(yf);
        int aa = perm512[perm512[xi] + yi], ab = perm512[perm512[xi] + yi + 1];
        int ba = perm512[perm512[xi + 1] + yi], bb = perm512[perm512[xi + 1] + yi + 1];
        float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
        float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);
        return lerp(x1, x2, v) * 1.4f; // нормализация к [-1,1]
    }

    // Фрактальный шум (fbm)
    float fbm(float x, float y, int octaves = 4) const {
        float sum = 0, amp = 0.5f, freq = 1.0f;
        for (int i = 0; i < octaves; i++) {
            sum += noise2(x * freq, y * freq) * amp;
            amp *= 0.5f; freq *= 2.0f;
        }
        return sum;
    }

private:
    uint8_t perm[256], perm512[512];
    static int fastfloor(float x) { return x < 0 ? int(x) - 1 : int(x); }
    static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static float lerp(float a, float b, float t) { return a + t * (b - a); }
    static float grad(uint8_t h, float x, float y) {
        switch (h & 3) {
            case 0: return  x + y;
            case 1: return -x + y;
            case 2: return  x - y;
            default: return -x - y;
        }
    }
};
