#pragma once
#include <cstdint>

namespace MonoWheel {

struct LedMetrics {
    float speed = 0;           // км/ч
    uint8_t battery = 0;       // %
    uint16_t pwm = 0;
    float temperature = 0;     // °C
    float voltage = 0;         // В
    bool is_charging = false;
    bool is_connected = false;
    bool headlight_on = false;
    bool drl_on = false;
    bool buzzer_on = false;
};

struct StripConfig {
    int count;              // Количество светодиодов
    const uint8_t* rows;    // Строки для "ёлочки" (nullptr для линейной)
    int num_rows;           // Количество строк (0 для линейной)
};

} // namespace MonoWheel