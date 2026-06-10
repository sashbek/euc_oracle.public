#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace SettingsProtocol {
    
    // ============ BLE UUIDs ============
    #define SETTINGS_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
    #define SETTINGS_WRITE_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
    #define SETTINGS_READ_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a9"
    
    // ============ Маркеры пакетов ============
    constexpr uint8_t WRITE_START_MARKER[4] = {0x55, 0xAA, 0x55, 0xAA};
    constexpr uint8_t WRITE_END_MARKER[4]   = {0x55, 0xBB, 0x55, 0xBB};
    
    // ============ Адресное пространство ============
    namespace AddrSpace {
        constexpr uint16_t PROTOCOL     = 0x0000;  // 0x00-0x0F: протокольные регистры
        constexpr uint16_t SYSTEM       = 0x0010;  // 0x10-0x3F: системные
        constexpr uint16_t THIRD_EYE    = 0x0040;  // 0x40-0x5F: ThirdEye
        constexpr uint16_t LED_PROXY    = 0x0060;  // 0x60-0x7F: LED прокси
        constexpr uint16_t LED_EFFECTS  = 0x0080;  // 0x80-0x9F: LED эффекты
        constexpr uint16_t UART         = 0x00A0;  // 0xA0-0xBF: UART
        constexpr uint16_t DYNAMIC      = 0x00C0;  // 0xC0-0xFF: динамические данные
        constexpr uint16_t MAX_ADDR     = 0x00FF;
    }
    
    // ============ Протокольные регистры (0x00 - 0x0F) ============
    enum class ProtoReg : uint16_t {
        READ_ADDR_L    = 0x00,  // R/W: Младший байт адреса для чтения
        READ_ADDR_H    = 0x01,  // R/W: Старший байт адреса для чтения
        READ_SIZE      = 0x02,  // R/W: Размер данных для чтения (0 = отмена)
        CMD            = 0x03,  // W: Команда
        STATUS         = 0x04,  // R: Статус/ошибка
        VERSION        = 0x05,  // R: Версия протокола
        DEVICE_TYPE    = 0x06,  // R: Тип устройства
    };
    
    // ============ Команды (пишутся в ProtoReg::CMD) ============
    enum class Command : uint8_t {
        NONE           = 0x00,
        START_READ     = 0x01,  // Начать чтение (адрес и размер уже заданы)
        SAVE_FLASH     = 0x02,  // Сохранить настройки в NVS
        LOAD_FLASH     = 0x03,  // Загрузить настройки из NVS
        RESET_DEFAULTS = 0x04,  // Сбросить к значениям по умолчанию
        SOFT_RESET     = 0x05,  // Программный сброс устройства
    };
    
    // ============ Статусы/ошибки (читаются из ProtoReg::STATUS) ============
    enum class Status : uint8_t {
        OK             = 0x00,
        ERR_MARKER     = 0x01,  // Неверный маркер
        ERR_ADDR       = 0x02,  // Неверный адрес
        ERR_SIZE       = 0x03,  // Неверный размер
        ERR_OVERFLOW   = 0x04,  // Переполнение буфера
        ERR_PROTECTED  = 0x05,  // Запись запрещена
        ERR_FLASH      = 0x06,  // Ошибка NVS
        ERR_BUSY       = 0x07,  // Устройство занято
        DATA_READY     = 0x80,  // Данные готовы для чтения
    };
    
    // ============ Типы устройств ============
    enum class DeviceType : uint8_t {
        WHEEL = 0x00,
        LED   = 0x01,
    };

    // ============ UART протокол ============
    enum class UartParserType : uint8_t {
        MOCK           = 0x00,
        VETERAN        = 0x01,  // Начать чтение (адрес и размер уже заданы)
    };
    
    // ============ Формат пакета записи ============
    /*
     * Структура данных в WRITE характеристике:
     * 
     * [4 байта] WRITE_START_MARKER = {0x55, 0xAA, 0x55, 0xAA}
     * [Повторяющиеся блоки]:
     *   [2 байта] address - адрес регистра (little-endian)
     *   [2 байта] size    - размер данных (little-endian)
     *   [N байт]  data    - данные
     * [4 байта] WRITE_END_MARKER = {0x55, 0xBB, 0x55, 0xBB}
     * 
     * Пример записи яркости ThirdEye = 200:
     *   55 AA 55 AA          // Старт
     *   44 00                // Адрес 0x0044 (BRIGHTNESS)
     *   01 00                // Размер 1 байт
     *   C8                   // Значение 200
     *   55 BB 55 BB          // Конец
     * 
     * Пример записи нескольких параметров:
     *   55 AA 55 AA          // Старт
     *   40 00 01 00 01       // ENABLED = 1
     *   41 00 01 00 01       // TOP_COLUMN = SPEED
     *   44 00 01 00 C8       // BRIGHTNESS = 200
     *   55 BB 55 BB          // Конец
     */
    
    // ============ Формат чтения данных ============
    /*
     * Алгоритм чтения:
     * 1. Записать младший байт адреса в ProtoReg::READ_ADDR_L (0x00)
     * 2. Записать старший байт адреса в ProtoReg::READ_ADDR_H (0x01)
     * 3. Записать размер данных в ProtoReg::READ_SIZE (0x02)
     * 4. Записать Command::START_READ (0x01) в ProtoReg::CMD (0x03)
     * 5. Читать данные из READ характеристики
     * 
     * Формат данных в READ характеристике:
     *   [2 байта] offset - смещение от запрошенного адреса (little-endian)
     *   [до 20 байт] data - данные
     * 
     * Пример запроса на чтение 32 байт с адреса 0x0040 (настройки ThirdEye):
     *   55 AA 55 AA
     *   00 00 01 00 40       // READ_ADDR_L = 0x40
     *   01 00 01 00 00       // READ_ADDR_H = 0x00
     *   02 00 01 00 20       // READ_SIZE = 32
     *   03 00 01 00 01       // CMD = START_READ
     *   55 BB 55 BB
     * 
     * Ответ в READ характеристике (первый пакет):
     *   00 00                // offset = 0
     *   01 01 06 01 80 ...   // данные (до 20 байт)
     */
    
    constexpr size_t MAX_CHUNK_SIZE = 20;  // Максимальный размер чанка данных
    constexpr size_t MAX_WRITE_BLOCK = 64; // Максимальный размер блока записи
    
    // ============ Версия протокола ============
    constexpr uint8_t PROTOCOL_VERSION = 0x01;
}

// ============ Вспомогательные функции ============
namespace SettingsHelpers {
    
    /**
     * @brief Проверка совпадения маркера
     */
    inline bool check_marker(const uint8_t* data, const uint8_t* marker, size_t len = 4) {
        return memcmp(data, marker, len) == 0;
    }
    
    /**
     * @brief Поиск маркера в буфере
     * @return Индекс маркера или -1 если не найден
     */
    inline int find_marker(const uint8_t* data, size_t data_len, 
                          const uint8_t* marker, size_t marker_len = 4) {
        if (data_len < marker_len) return -1;
        for (size_t i = 0; i <= data_len - marker_len; i++) {
            if (check_marker(data + i, marker, marker_len)) return i;
        }
        return -1;
    }
    
    /**
     * @brief Получить имя команды
     */
    inline const char* getCommandName(SettingsProtocol::Command cmd) {
        switch (cmd) {
            case SettingsProtocol::Command::NONE: return "NONE";
            case SettingsProtocol::Command::START_READ: return "START_READ";
            case SettingsProtocol::Command::SAVE_FLASH: return "SAVE_FLASH";
            case SettingsProtocol::Command::LOAD_FLASH: return "LOAD_FLASH";
            case SettingsProtocol::Command::RESET_DEFAULTS: return "RESET_DEFAULTS";
            case SettingsProtocol::Command::SOFT_RESET: return "SOFT_RESET";
            default: return "UNKNOWN";
        }
    }
    
    /**
     * @brief Получить имя статуса
     */
    inline const char* getStatusName(SettingsProtocol::Status status) {
        switch (status) {
            case SettingsProtocol::Status::OK: return "OK";
            case SettingsProtocol::Status::ERR_MARKER: return "ERR_MARKER";
            case SettingsProtocol::Status::ERR_ADDR: return "ERR_ADDR";
            case SettingsProtocol::Status::ERR_SIZE: return "ERR_SIZE";
            case SettingsProtocol::Status::ERR_OVERFLOW: return "ERR_OVERFLOW";
            case SettingsProtocol::Status::ERR_PROTECTED: return "ERR_PROTECTED";
            case SettingsProtocol::Status::ERR_FLASH: return "ERR_FLASH";
            case SettingsProtocol::Status::ERR_BUSY: return "ERR_BUSY";
            case SettingsProtocol::Status::DATA_READY: return "DATA_READY";
            default: return "UNKNOWN";
        }
    }
}