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

    // LED Effects (0x40 - 0x5F)
    LED_EFFECT_MODE                     = 0x40,
    LED_BASE_SPEED                      = 0x41,
    LED_COLOR_1                         = 0x43,
    LED_COLOR_2                         = 0x47,
    LED_COLOR_3                         = 0x4B,
    LED_COLOR_4                         = 0x4F,
    LED_COLOR_5                         = 0x53,
    LED_SPECIAL_SHOW_CHARGE             = 0x57,
    LED_SPECIAL_SPEED_BOOST             = 0x58,
    LED_SPEED_THRESHOLD_1               = 0x59,
    LED_SPEED_BOOST_1                   = 0x5B,
    LED_SPEED_THRESHOLD_2               = 0x5D,
    LED_SPEED_BOOST_2                   = 0x5F,

    // Extentions (0x62 - 0x62)
    EXT_PROXY_PEER_GP                   = 0x62,

};

// ============================================================================
// Индексы настроек (только для персистентных регистров)
// ============================================================================
enum class SettingIndex : uint16_t {
    // System
    BOOT_COUNT                          = 0x80,
    DEVICE_NAME                         = 0x81,

    // LED Effects
    LED_EFFECT_MODE                     = 0x40,
    LED_BASE_SPEED                      = 0x41,
    LED_COLOR_1                         = 0x42,
    LED_COLOR_2                         = 0x43,
    LED_COLOR_3                         = 0x44,
    LED_COLOR_4                         = 0x45,
    LED_COLOR_5                         = 0x46,
    LED_SPECIAL_SHOW_CHARGE             = 0x47,
    LED_SPECIAL_SPEED_BOOST             = 0x48,
    LED_SPEED_THRESHOLD_1               = 0x49,
    LED_SPEED_BOOST_1                   = 0x4A,
    LED_SPEED_THRESHOLD_2               = 0x4B,
    LED_SPEED_BOOST_2                   = 0x4C,

    // Extentions
    EXT_PROXY_PEER_GP                   = 0x20,

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
   @desc Тип устройства (0=WHEEL, 1=LED) \
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
   @default MonoWheel_LED
   @desc Имя устройства \
 */ \
    REG(0x27, 16, true, true, true, (const uint8_t*)"MonoWheel_LED") \
     \
    /* @name LED_EFFECT_MODE
   @type ENUM
   @group LED Effects
   @default 2
   @enum 0:STATIC_COLOR,1:BREATHING,2:GRADIENT_3,3:GRADIENT_5,4:WHITE_SHIFT,5:BLINK_2,6:BLINK_3,7:BLINK_5,8:SINGLE_FLASH_1,9:SINGLE_FLASH_3,10:SINGLE_FLASH_5,11:DOUBLE_FLASH_1,12:DOUBLE_FLASH_3,13:DOUBLE_FLASH_5,14:TRIPLE_FLASH_1,15:TRIPLE_FLASH_3,16:TRIPLE_FLASH_5,17:CLONE_DRL,18:POLICE
   @desc Режим эффекта LED лент \
 */ \
    REG(0x40, 1, true, true, true, (const uint8_t*)"\x02") \
     \
    /* @name LED_BASE_SPEED
   @type UINT16
   @group LED Effects
   @unit %
   @min 1
   @max 1000
   @default 100
   @desc Базовая скорость анимации \
 */ \
    REG(0x41, 2, true, true, true, (const uint8_t*)"\x64\x00") \
     \
    /* @name LED_COLOR_1
   @type COLOR_RGBW
   @group LED Effects
   @default #FF000000
   @desc Цвет 1 (RGBW) \
 */ \
    REG(0x43, 4, true, true, true, (const uint8_t*)"\xFF\x00\x00\x00") \
     \
    /* @name LED_COLOR_2
   @type COLOR_RGBW
   @group LED Effects
   @default #00FF0000
   @desc Цвет 2 (RGBW) \
 */ \
    REG(0x47, 4, true, true, true, (const uint8_t*)"\x00\xFF\x00\x00") \
     \
    /* @name LED_COLOR_3
   @type COLOR_RGBW
   @group LED Effects
   @default #0000FF00
   @desc Цвет 3 (RGBW) \
 */ \
    REG(0x4B, 4, true, true, true, (const uint8_t*)"\x00\x00\xFF\x00") \
     \
    /* @name LED_COLOR_4
   @type COLOR_RGBW
   @group LED Effects
   @default #FFFF0000
   @desc Цвет 4 (RGBW) \
 */ \
    REG(0x4F, 4, true, true, true, (const uint8_t*)"\xFF\xFF\x00\x00") \
     \
    /* @name LED_COLOR_5
   @type COLOR_RGBW
   @group LED Effects
   @default #FFFFFF00
   @desc Цвет 5 (RGBW) \
 */ \
    REG(0x53, 4, true, true, true, (const uint8_t*)"\xFF\xFF\xFF\x00") \
     \
    /* @name LED_SPECIAL_SHOW_CHARGE
   @type BOOLEAN
   @group LED Effects
   @default True
   @desc Показать заряд при включении \
 */ \
    REG(0x57, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name LED_SPECIAL_SPEED_BOOST
   @type BOOLEAN
   @group LED Effects
   @default True
   @desc Ускорять анимации пропорционально скорости \
 */ \
    REG(0x58, 1, true, true, true, (const uint8_t*)"\x01") \
     \
    /* @name LED_SPEED_THRESHOLD_1
   @type UINT16
   @group LED Effects
   @unit km/h
   @scale 0.1
   @min 0
   @max 300
   @default 10
   @desc Значение скорости 1 \
 */ \
    REG(0x59, 2, true, true, true, (const uint8_t*)"\x0A\x00") \
     \
    /* @name LED_SPEED_BOOST_1
   @type UINT16
   @group LED Effects
   @unit %
   @min 10
   @max 1000
   @default 200
   @desc Скорость анимации для скорости 1 \
 */ \
    REG(0x5B, 2, true, true, true, (const uint8_t*)"\xC8\x00") \
     \
    /* @name LED_SPEED_THRESHOLD_2
   @type UINT16
   @group LED Effects
   @unit km/h
   @scale 0.1
   @min 0
   @max 300
   @default 25
   @desc Значение скорости 2 \
 */ \
    REG(0x5D, 2, true, true, true, (const uint8_t*)"\x19\x00") \
     \
    /* @name LED_SPEED_BOOST_2
   @type UINT16
   @group LED Effects
   @unit %
   @min 10
   @max 1000
   @default 500
   @desc Скорость анимации для скорости 2 \
 */ \
    REG(0x5F, 2, true, true, true, (const uint8_t*)"\xF4\x01") \
     \
    /* @name EXT_PROXY_PEER_GP
   @type STRING
   @group Extentions
   @default EXT1
   @desc Идентификатор группы подключаемых внешних устройств \
 */ \
    REG(0x62, 6, true, true, true, (const uint8_t*)"EXT1") \


#define SETTINGS_MAPPING \
    index_map_[MonoWheelRegs::SettingIndex::BOOT_COUNT] = 0x1A;\
    index_map_[MonoWheelRegs::SettingIndex::DEVICE_NAME] = 0x27;\
    index_map_[MonoWheelRegs::SettingIndex::LED_EFFECT_MODE] = 0x40;\
    index_map_[MonoWheelRegs::SettingIndex::LED_BASE_SPEED] = 0x41;\
    index_map_[MonoWheelRegs::SettingIndex::LED_COLOR_1] = 0x43;\
    index_map_[MonoWheelRegs::SettingIndex::LED_COLOR_2] = 0x47;\
    index_map_[MonoWheelRegs::SettingIndex::LED_COLOR_3] = 0x4B;\
    index_map_[MonoWheelRegs::SettingIndex::LED_COLOR_4] = 0x4F;\
    index_map_[MonoWheelRegs::SettingIndex::LED_COLOR_5] = 0x53;\
    index_map_[MonoWheelRegs::SettingIndex::LED_SPECIAL_SHOW_CHARGE] = 0x57;\
    index_map_[MonoWheelRegs::SettingIndex::LED_SPECIAL_SPEED_BOOST] = 0x58;\
    index_map_[MonoWheelRegs::SettingIndex::LED_SPEED_THRESHOLD_1] = 0x59;\
    index_map_[MonoWheelRegs::SettingIndex::LED_SPEED_BOOST_1] = 0x5B;\
    index_map_[MonoWheelRegs::SettingIndex::LED_SPEED_THRESHOLD_2] = 0x5D;\
    index_map_[MonoWheelRegs::SettingIndex::LED_SPEED_BOOST_2] = 0x5F;\
    index_map_[MonoWheelRegs::SettingIndex::EXT_PROXY_PEER_GP] = 0x62;
