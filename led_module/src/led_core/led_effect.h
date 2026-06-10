#pragma once
#include "led_types.h"

namespace MonoWheel {
namespace Effects {

/**
 * @brief Чистая функция эффекта
 * @param led_index Индекс светодиода в ленте
 * @param time_ms Текущее время в миллисекундах
 * @param params Параметры (скорость, цвета и т.д.)
 * @param strip_config Конфигурация ленты
 * @return Цвет в формате 0x00RRGGBB
 */
using EffectFunction = uint32_t (*)(int led_index, unsigned long time_ms, 
                                     const LedMetrics& params,
                                     const StripConfig& strip_config);

// Параметры эффекта
struct EffectParams {
    uint32_t colors[5];         // 5 цветов
    int num_colors = 1;         // Количество используемых цветов
    uint8_t speed = 100;        // Базовая скорость (1-1000)
    uint8_t brightness = 255;   // Яркость (0-255)
};

// Теперь extern после определения структуры
extern EffectParams g_effectParams;

} // namespace Effects
} // namespace MonoWheel
