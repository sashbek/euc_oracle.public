#include "effects.h"
#include <FastLED.h>

namespace MonoWheel {
namespace Effects {
    
// Глобальные параметры эффекта
EffectParams g_effectParams;

// Вспомогательные функции
static uint32_t crgbToUint(const CRGB& c) {
    return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
}

static CRGB uintToCrgb(uint32_t v) {
    return CRGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

static CRGB lerpColor(CRGB a, CRGB b, uint8_t t) {
    return blend(a, b, t);
}

// Внешние параметры эффекта (устанавливаются перед вызовом)
extern EffectParams g_effectParams;

// ============ Статичный цвет ============
uint32_t staticColor(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) {
    return crgbToUint(uintToCrgb(g_effectParams.colors[0]));
}

// ============ Переливание 3 цвета ============
uint32_t gradient3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) {
    int period = map(g_effectParams.speed, 1, 1000, 5000, 500);
    uint8_t phase = (t % period) * 255 / period;
    
    float pos = float(idx) / cfg.count;
    int color_idx = int(pos * 3);
    float frac = pos * 3 - color_idx;
    
    if (color_idx >= 2) { color_idx = 1; frac = 1.0f; }
    
    CRGB c1 = uintToCrgb(g_effectParams.colors[color_idx]);
    CRGB c2 = uintToCrgb(g_effectParams.colors[color_idx + 1]);
    
    CRGB result = lerpColor(c1, c2, uint8_t(frac * 255));
    CHSV hsv = rgb2hsv_approximate(result);
    hsv.hue += phase;
    result = hsv;
    
    return crgbToUint(result);
}

// ============ Переливание 5 цветов ============
uint32_t gradient5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) {
    int period = map(g_effectParams.speed, 1, 1000, 8000, 800);
    uint8_t phase = (t % period) * 255 / period;
    
    float pos = float(idx) / cfg.count;
    int color_idx = int(pos * 5);
    float frac = pos * 5 - color_idx;
    
    if (color_idx >= 4) { color_idx = 3; frac = 1.0f; }
    
    CRGB c1 = uintToCrgb(g_effectParams.colors[color_idx]);
    CRGB c2 = uintToCrgb(g_effectParams.colors[color_idx + 1]);
    
    CRGB result = lerpColor(c1, c2, uint8_t(frac * 255));
    CHSV hsv = rgb2hsv_approximate(result);
    hsv.hue += phase;
    result = hsv;
    
    return crgbToUint(result);
}

// ============ Белый с переливами ============
uint32_t whiteShift(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) {
    int period = map(g_effectParams.speed, 1, 1000, 3000, 300);
    uint8_t hue = (t / 50) % 255;  // Медленный сдвиг оттенка
    uint8_t sat = 30 + sin8((t % period) * 255 / period) / 4;  // Лёгкое изменение насыщенности
    
    CRGB result = CHSV(hue, sat, 200);
    return crgbToUint(result);
}

// ============ Перемигивание ============
static uint32_t blinkN(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg, int n) {
    int period = map(g_effectParams.speed, 1, 1000, 2000, 200);
    int half = period / 2;
    int phase = (t % period);
    
    // Все светодиоды меняют цвет одновременно
    int color_idx = 0;
    if (phase < half) {
        color_idx = ((t / half) % n);
    }
    
    return crgbToUint(uintToCrgb(g_effectParams.colors[color_idx]));
}

uint32_t blink2(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return blinkN(idx, t, m, cfg, 2); }
uint32_t blink3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return blinkN(idx, t, m, cfg, 3); }
uint32_t blink5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return blinkN(idx, t, m, cfg, 5); }

// ============ Одиночное/двойное/тройное мигание ============
static uint32_t flashN_M(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg, int flashes, int num_colors) {
    int period = map(g_effectParams.speed, 1, 1000, 3000, 300);
    int flash_duration = period / (flashes * 3);  // 1/3 времени — вспышка, 2/3 — пауза
    int phase = t % period;
    
    int flash_num = phase / flash_duration;
    bool is_on = (flash_num < flashes * 2) && (flash_num % 2 == 0);
    
    if (is_on) {
        int color_idx = (flash_num / 2) % num_colors;
        return crgbToUint(uintToCrgb(g_effectParams.colors[color_idx]));
    }
    
    return 0;  // Выключено
}

uint32_t flash1_1(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 1, 1); }
uint32_t flash1_3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 1, 3); }
uint32_t flash1_5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 1, 5); }
uint32_t flash2_1(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 2, 1); }
uint32_t flash2_3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 2, 3); }
uint32_t flash2_5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 2, 5); }
uint32_t flash3_1(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 3, 1); }
uint32_t flash3_3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 3, 3); }
uint32_t flash3_5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) { return flashN_M(idx, t, m, cfg, 3, 5); }

// ============ Клонировать ДХО ============
uint32_t cloneDrl(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) {
    if (m.drl_on) {
        return crgbToUint(CRGB::White);
    }
    return 0;
}

// ============ Полицейское мигание ============
uint32_t police(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg) {
    int period = map(g_effectParams.speed, 1, 1000, 1000, 100);
    int half = period / 2;
    bool is_red = (t % period) < half;
    
    // Половина ленты — красный, половина — синий
    bool left_side = (idx < cfg.count / 2);
    
    if (is_red) {
        return left_side ? crgbToUint(CRGB::Red) : crgbToUint(CRGB::Blue);
    } else {
        return left_side ? crgbToUint(CRGB::Blue) : crgbToUint(CRGB::Red);
    }
}

} // namespace Effects
} // namespace MonoWheel