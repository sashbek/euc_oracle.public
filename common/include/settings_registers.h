#pragma once
#include <cstdint>

namespace MonoWheelRegs {

// ============================================================================
// Типы данных
// ============================================================================
enum class Command : uint8_t {
    NONE            = 0x00,
    START_READ      = 0x01,
    SAVE_FLASH      = 0x02,
    LOAD_FLASH      = 0x03,
    RESET_DEFAULTS  = 0x04,
    SOFT_RESET      = 0x05,
};

enum class Status : uint8_t {
    OK              = 0x00,
    ERR_MARKER      = 0x01,
    ERR_ADDR        = 0x02,
    ERR_SIZE        = 0x03,
    ERR_PROTECTED   = 0x04,
    ERR_FLASH       = 0x05,
    ERR_BUSY        = 0x06,
    DATA_READY      = 0x80,
};

enum class DeviceType : uint8_t {
    WHEEL = 0x00,
    LED   = 0x01,
};

enum class ThirdEyeColumn : uint8_t {
    NONE        = 0,
    SPEED       = 1,
    PWM         = 2,
    VOLTAGE     = 3,
    CURRENT     = 4,
    POWER       = 5,
    BATTERY     = 6,
    TEMP        = 7,
    DISTANCE    = 8,
    RIDE_TIME   = 9,
};

enum class ThirdEyeConnState : uint8_t {
    DISCONNECTED = 0,
    SCANNING     = 1,
    CONNECTING   = 2,
    CONNECTED    = 3,
    ERROR        = 0xFF,
};

// ============================================================================
// Значения по умолчанию
// ============================================================================
namespace Defaults {
    constexpr uint8_t  THIRD_EYE_ENABLED = 1;
    constexpr uint8_t  THIRD_EYE_TOP_COL = 1;
    constexpr uint8_t  THIRD_EYE_BOTTOM_COL = 6;
    constexpr uint8_t  THIRD_EYE_SHOW_NAMES = 1;
    constexpr uint8_t  THIRD_EYE_BRIGHTNESS = 128;
    constexpr uint8_t  THIRD_EYE_AUTO_CONNECT = 1;
    constexpr uint8_t  THIRD_EYE_RECONN_INT = 10;
    constexpr char     THIRD_EYE_TARGET_NAME[] = "ThirdEye_";
    
    constexpr uint8_t  LED_PROXY_ENABLED = 1;
    constexpr uint8_t  LED_PROXY_SEND_RATE = 60;
    constexpr uint8_t  LED_PROXY_PEER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    constexpr uint8_t  LED_BRIGHTNESS = 128;
    constexpr uint8_t  LED_EFFECT_MODE = 1;
    constexpr uint8_t  LED_EFFECT_SPEED = 128;
    
    constexpr uint8_t  UART_PROTOCOL = 0;
    
    constexpr uint8_t  LOG_LEVEL = 3;
    constexpr char     DEVICE_NAME[] = "MonoWheel";
}

} // namespace MonoWheelRegs