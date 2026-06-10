#pragma once
#include "led_core/led_effect.h"

namespace MonoWheel {
namespace Effects {

// 2. Свечение конкретным цветом
uint32_t staticColor(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 3. Переливание 3 цвета
uint32_t gradient3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 4. Переливание 5 цветов
uint32_t gradient5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 5. Белый с переливами
uint32_t whiteShift(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 6. Перемигивание 2 цвета
uint32_t blink2(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 7. Перемигивание 3 цвета
uint32_t blink3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 8. Перемигивание 5 цветов
uint32_t blink5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 9. Одиночное мигание 1 цвет
uint32_t flash1_1(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 10. Одиночное мигание 3 цвета
uint32_t flash1_3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 11. Одиночное мигание 5 цветов
uint32_t flash1_5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 12. Двойное мигание 1 цвет
uint32_t flash2_1(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 13. Двойное мигание 3 цвета
uint32_t flash2_3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 14. Двойное мигание 5 цветов
uint32_t flash2_5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 15. Тройное мигание 1 цвет
uint32_t flash3_1(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 16. Тройное мигание 3 цвета
uint32_t flash3_3(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 17. Тройное мигание 5 цветов
uint32_t flash3_5(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 18. Клонировать ДХО
uint32_t cloneDrl(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

// 19. Полицейское мигание
uint32_t police(int idx, unsigned long t, const LedMetrics& m, const StripConfig& cfg);

} // namespace Effects
} // namespace MonoWheel