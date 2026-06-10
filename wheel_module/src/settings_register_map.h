#pragma once

namespace MonoWheelRegs {

// ============================================================================
// АДРЕСА РЕГИСТРОВ - единый enum для всех регистров
// ============================================================================
enum class RegAddr : uint16_t {
    // Other (0x00 - 0x06)
    READ_ADDR_L                         = 0x00,
    READ_ADDR_H                         = 0x01,
    READ_SIZE                           = 0x02,
    CMD                                 = 0x03,
    STATUS                              = 0x04,
    VERSION                             = 0x05,
    DEVICE_TYPE                         = 0x06,

    // System (0x10 - 0x27)
    DEVICE_ID                           = 0x10,
    FW_VERSION                          = 0x14,
    BOOT_COUNT                          = 0x1A,
    DEVICE_NAME                         = 0x27,

    // ThirdEye (0x40 - 0x57)
    THIRD_EYE_ENABLED                   = 0x40,
    THIRD_EYE_TOP_COLUMN                = 0x41,
    THIRD_EYE_BOTTOM_COLUMN             = 0x42,
    THIRD_EYE_SHOW_NAMES                = 0x43,
    THIRD_EYE_BRIGHTNESS                = 0x44,
    THIRD_EYE_AUTO_CONNECT              = 0x45,
    THIRD_EYE_RECONN_INTERVAL           = 0x46,
    THIRD_EYE_TARGET_NAME               = 0x47,
    THIRD_EYE_CONN_STATE                = 0x57,

    // Extentions (0x60 - 0x80)
    EXT_PROXY_ENABLE                    = 0x60,
    EXT_PROXY_PEER_GP                   = 0x62,
    EXT_PROXY_GP_CAP                    = 0x80,

    // UART (0xA0 - 0xA0)
    UART_PROTOCOL                       = 0xA0,

};

// ============================================================================
// Индексы настроек (только для персистентных регистров)
// ============================================================================
enum class SettingIndex : uint16_t {
    // System
    BOOT_COUNT                          = 0x80,
    DEVICE_NAME                         = 0x81,

    // ThirdEye
    THIRD_EYE_ENABLED                   = 0x00,
    THIRD_EYE_TOP_COLUMN                = 0x01,
    THIRD_EYE_BOTTOM_COLUMN             = 0x02,
    THIRD_EYE_SHOW_NAMES                = 0x03,
    THIRD_EYE_BRIGHTNESS                = 0x04,
    THIRD_EYE_AUTO_CONNECT              = 0x05,
    THIRD_EYE_RECONN_INTERVAL           = 0x06,
    THIRD_EYE_TARGET_NAME               = 0x07,

    // Extentions
    EXT_PROXY_ENABLE                    = 0x20,
    EXT_PROXY_PEER_GP                   = 0x21,
    EXT_PROXY_GP_CAP                    = 0x22,

    // UART
    UART_PROTOCOL                       = 0x60,

    MAX_INDEX
};

}

// ============================================================================
// Карта регистров
// Формат: REG(addr, size, readable, writable, persistent, default_value)
// ============================================================================

#define SETTINGS_REGISTER_MAP \
    /* @name READ_ADDR_L
   @type UINT8
   @hidden true
   @desc Младший байт адреса для чтения \
 */ \
    REG(0x00, 1, true, true, false, nullptr) \
     \
    /* @name READ_ADDR_H
   @type UINT8
   @hidden true
   @desc Старший байт адреса для чтения \
 */ \
    REG(0x01, 1, true, true, false, nullptr) \
     \
    /* @name READ_SIZE
   @type UINT8
   @hidden true
   @desc Размер данных для чтениe \
 */ \
    REG(0x02, 1, true, true, false, nullptr) \
     \
    /* @name CMD
   @type UINT8
   @readable false
   @hidden true
   @desc Команда управления \
 */ \
    REG(0x03, 1, false, true, false, nullptr) \
     \
    /* @name STATUS
   @type UINT8
   @writable false
   @hidden true
   @desc Статус последней операции \
 */ \
    REG(0x04, 1, true, false, false, nullptr) \
     \
    /* @name VERSION
   @type UINT8
   @writable false
   @desc Версия протокола \
 */ \
    REG(0x05, 1, true, false, false, nullptr) \
     \
    /* @name DEVICE_TYPE
   @type UINT8
   @writable false
   @desc Тип устройства (0=WHEEL, 1=EXT) \
 */ \
    REG(0x06, 1, true, false, false, nullptr) \
     \
    /* @name DEVICE_ID
   @type UINT32
   @writable false
   @group System
   @desc Уникальный идентификатор устройства (MAC) \
 */ \
    REG(0x10, 4, true, false, false, nullptr) \
     \
    /* @name FW_VERSION
   @type UINT32
   @writable false
   @group System
   @desc Версия прошивки (major.minor.patch.build) \
 */ \
    REG(0x14, 4, true, false, false, nullptr) \
     \
    /* @name BOOT_COUNT
   @type UINT32
   @group System
   @desc Счетчик загрузок устройства \
 */ \
    REG(0x1A, 4, true, true, true, nullptr) \
     \
    /* @name DEVICE_NAME
   @type STRING
   @group System
   @default MonoWheel
   @desc Имя устройства \
 */ \
    REG(0x27, 16, true, true, true, (const uint8_t*)"MonoWheel") \
     \
    /* @name THIRD_EYE_ENABLED
   @type BOOLEAN
   @group ThirdEye
   @default True
   @desc Включение дисплея ThirdEye на шлеме \
 */ \
    REG(0x40, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name THIRD_EYE_TOP_COLUMN
   @type ENUM
   @group ThirdEye
   @default 1
   @enum 0:NONE,1:SPEED,2:PWM,3:VOLTAGE,4:CURRENT,5:POWER,6:BATTERY,7:TEMP,8:DISTANCE,9:RIDE_TIME
   @desc Верхнее поле дисплея ThirdEye \
 */ \
    REG(0x41, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name THIRD_EYE_BOTTOM_COLUMN
   @type ENUM
   @group ThirdEye
   @default 6
   @enum 0:NONE,1:SPEED,2:PWM,3:VOLTAGE,4:CURRENT,5:POWER,6:BATTERY,7:TEMP,8:DISTANCE,9:RIDE_TIME
   @desc Нижнее поле дисплея ThirdEye \
 */ \
    REG(0x42, 1, true, true, true, (const uint8_t*)"\x06") \
     \
    /* @name THIRD_EYE_SHOW_NAMES
   @type BOOLEAN
   @group ThirdEye
   @default True
   @desc Показывать названия полей на дисплее \
 */ \
    REG(0x43, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name THIRD_EYE_BRIGHTNESS
   @type UINT8
   @group ThirdEye
   @min 0
   @max 255
   @default 128
   @desc Яркость дисплея ThirdEye \
 */ \
    REG(0x44, 1, true, true, true, (const uint8_t*)"\x80") \
     \
    /* @name THIRD_EYE_AUTO_CONNECT
   @type BOOLEAN
   @group ThirdEye
   @default True
   @desc Автоматическое подключение к ThirdEye \
 */ \
    REG(0x45, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name THIRD_EYE_RECONN_INTERVAL
   @type UINT8
   @group ThirdEye
   @unit s
   @min 1
   @max 60
   @default 10
   @desc Интервал попыток переподключения \
 */ \
    REG(0x46, 1, true, true, true, (const uint8_t*)"\x0A") \
     \
    /* @name THIRD_EYE_TARGET_NAME
   @type STRING
   @group ThirdEye
   @default ThirdEye_
   @desc BLE имя устройства ThirdEye (префикс) \
 */ \
    REG(0x47, 16, true, true, true, (const uint8_t*)"ThirdEye_") \
     \
    /* @name THIRD_EYE_CONN_STATE
   @type ENUM
   @writable false
   @group ThirdEye
   @enum 0:DISCONNECTED,1:SCANNING,2:CONNECTING,3:CONNECTED,255:ERROR
   @desc Состояние подключения к ThirdEye \
 */ \
    REG(0x57, 1, true, false, false, nullptr) \
     \
    /* @name EXT_PROXY_ENABLE
   @type BOOLEAN
   @group Extentions
   @default True
   @desc Отправка данных на внешние модули \
 */ \
    REG(0x60, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name EXT_PROXY_PEER_GP
   @type STRING
   @group Extentions
   @default EXT1
   @desc Идентификатор группы подключаемых внешних устройств \
 */ \
    REG(0x62, 6, true, true, true, (const uint8_t*)"EXT1") \
     \
    /* @name EXT_PROXY_GP_CAP
   @type UINT8
   @group Extentions
   @min 0
   @max 3
   @default 1
   @desc Максимальное число подключаемых внешних устройств \
 */ \
    REG(0x80, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name UART_PROTOCOL
   @type ENUM
   @group UART
   @default 0
   @enum 0:MOCK,1:VETERAN
   @desc Протокол обмена с контроллером колеса \
 */ \
    REG(0xA0, 1, true, true, true, (const uint8_t*)"\x00") \


#define SETTINGS_MAPPING \
    index_map_[MonoWheelRegs::SettingIndex::BOOT_COUNT] = 0x1A;\
    index_map_[MonoWheelRegs::SettingIndex::DEVICE_NAME] = 0x27;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_ENABLED] = 0x40;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_TOP_COLUMN] = 0x41;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_BOTTOM_COLUMN] = 0x42;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_SHOW_NAMES] = 0x43;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_BRIGHTNESS] = 0x44;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_AUTO_CONNECT] = 0x45;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_RECONN_INTERVAL] = 0x46;\
    index_map_[MonoWheelRegs::SettingIndex::THIRD_EYE_TARGET_NAME] = 0x47;\
    index_map_[MonoWheelRegs::SettingIndex::EXT_PROXY_ENABLE] = 0x60;\
    index_map_[MonoWheelRegs::SettingIndex::EXT_PROXY_PEER_GP] = 0x62;\
    index_map_[MonoWheelRegs::SettingIndex::EXT_PROXY_GP_CAP] = 0x80;\
    index_map_[MonoWheelRegs::SettingIndex::UART_PROTOCOL] = 0xA0;
