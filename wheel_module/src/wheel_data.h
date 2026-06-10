#pragma once
#include <cstdint>

namespace MonoWheel {

// Структура для хранения всех данных колеса
struct WheelData {
    // Основные данные
    float speed = 0;           // км/ч
    uint16_t pwm = 0;          // ШИМ (0-1000)
    float voltage = 0;         // В
    float current = 0;         // А
    float phase_current = 0;   // А
    float power = 0;           // Вт
    uint8_t battery_level = 0; // %
    float temperature = 0;     // °C
    float distance = 0;        // м
    float total_distance = 0;  // м
    uint32_t ride_time = 0;    // секунд
    
    // Дополнительные данные
    float pitch_angle = 0;     // градусы
    int16_t temperature_raw = 0;
    uint8_t charge_mode = 0;   // 0=нет, 1=зарядка
    uint16_t auto_off_sec = 0; // секунд до автоотключения
    uint16_t speed_alert = 0;  // км/ч * 10
    uint16_t speed_tiltback = 0; // км/ч * 10
    
    // Информация о колесе
    uint16_t version_raw = 0;
    char version_str[16] = {0};
    char model[16] = {0};
    
    // Флаги
    bool is_connected = false;
    bool is_charging = false;
    uint32_t error_flags = 0;
    uint16_t warnings = 0;
    
    // Время последнего обновления
    unsigned long last_update = 0;
    
    // Сброс данных
    void reset() {
        speed = 0;
        pwm = 0;
        voltage = 0;
        current = 0;
        phase_current = 0;
        power = 0;
        battery_level = 0;
        temperature = 0;
        pitch_angle = 0;
        is_connected = false;
    }
};

} // namespace MonoWheel