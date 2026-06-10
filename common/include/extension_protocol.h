#pragma once
#include <cstdint>

namespace MonoWheel {

// ============================================================================
// UUID для расширений
// ============================================================================
#define EXTENSION_SERVICE_UUID        "e19752de-87eb-45b0-afbb-a526aeba51aa"
#define EXTENSION_CHARACTERISTIC_UUID "baf7cc6c-7858-4225-a071-d6a73403ff26"

// ============================================================================
// Структура пакета данных (21 байт)
// ============================================================================
#pragma pack(push, 1)
struct ExtensionPacket {
    uint8_t header;          // 0xAA
    uint16_t speed;          // км/ч * 100 (2 байта)
    uint8_t battery;         // % (1 байт)
    uint16_t pwm;            // ШИМ * 10 (2 байта)
    int16_t temperature;     // °C * 10 (2 байта)
    uint8_t flags;           // Битовые флаги (1 байт)
    uint16_t voltage;        // В * 100 (2 байта)
    uint16_t current;        // А * 100 (2 байта)
    uint16_t power;          // Вт * 10 (2 байта)
    uint16_t distance;       // км * 10 (2 байта)
    uint32_t ride_time;      // секунды (4 байта)
    uint8_t crc;             // XOR всех байтов кроме crc (1 байт)
    // Итого: 21 байт
};
#pragma pack(pop)

// ============================================================================
// Флаги
// ============================================================================
enum ExtensionFlags : uint8_t {
    EXT_FLAG_CHARGING   = 0x01,
    EXT_FLAG_CONNECTED  = 0x02,
    EXT_FLAG_ALERT      = 0x04,
};

} // namespace MonoWheel