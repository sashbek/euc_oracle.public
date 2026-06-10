#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <functional>
#include <vector>
#include <unordered_map>

#include "settings_protocol.h"
#include "settings_registers.h"
#include "settings_register_map.h"

using SettingsChangeCallback = std::function<void(uint16_t addr, const uint8_t* data, size_t size)>;

class SettingsManager {
public:
    static SettingsManager& getInstance() {
        static SettingsManager instance;
        return instance;
    }
    
    bool begin(MonoWheelRegs::DeviceType device_type, const char* nvs_namespace = "settings");
    
    // ============ Доступ по индексу (только персистентные настройки) ============
    
    bool getBool(MonoWheelRegs::SettingIndex index, bool default_val = false) const;
    uint8_t getUint8(MonoWheelRegs::SettingIndex index, uint8_t default_val = 0) const;
    uint16_t getUint16(MonoWheelRegs::SettingIndex index, uint16_t default_val = 0) const;
    uint32_t getUint32(MonoWheelRegs::SettingIndex index, uint32_t default_val = 0) const;
    int8_t getInt8(MonoWheelRegs::SettingIndex index, int8_t default_val = 0) const;
    void getString(MonoWheelRegs::SettingIndex index, char* buf, size_t buf_size, const char* default_val = "") const;
    void getBytes(MonoWheelRegs::SettingIndex index, uint8_t* buf, size_t size) const;
    
    void setBool(MonoWheelRegs::SettingIndex index, bool value);
    void setUint8(MonoWheelRegs::SettingIndex index, uint8_t value);
    void setUint16(MonoWheelRegs::SettingIndex index, uint16_t value);
    void setUint32(MonoWheelRegs::SettingIndex index, uint32_t value);
    void setInt8(MonoWheelRegs::SettingIndex index, int8_t value);
    void setString(MonoWheelRegs::SettingIndex index, const char* value);
    void setBytes(MonoWheelRegs::SettingIndex index, const uint8_t* data, size_t size);
    
    // ============ Прямой доступ по адресу ============
    
    bool writeRegister(uint16_t addr, const uint8_t* data, size_t size);
    bool readRegister(uint16_t addr, uint8_t* data, size_t size) const;
    
    template<typename T>
    bool write(MonoWheelRegs::RegAddr addr, const T& value) {
        return writeRegister((uint16_t)addr, (const uint8_t*)&value, sizeof(T));
    }
    
    template<typename T>
    bool read(MonoWheelRegs::RegAddr addr, T& value) const {
        return readRegister((uint16_t)addr, (uint8_t*)&value, sizeof(T));
    }
    
    // ============ BLE обработка ============
    
    void handleWriteData(const uint8_t* data, size_t len);
    size_t handleReadData(uint8_t* data, size_t max_len);
    bool isDataReady() const { return data_ready_; }
    
    // ============ NVS ============
    
    bool saveToFlash();
    bool loadFromFlash();
    bool resetToDefaults();
    
    // ============ Подписка ============
    
    void subscribe(SettingsChangeCallback cb) { change_cb_ = cb; }
    void unsubscribe() { change_cb_ = nullptr; }
    
    // ============ Статус ============
    
    MonoWheelRegs::Status getStatus() const { return status_; }
    MonoWheelRegs::DeviceType getDeviceType() const { return device_type_; }
    void clearError() { status_ = MonoWheelRegs::Status::OK; }
    
    // ============ Информация ============
    
    uint32_t getBootCount() const;
    uint32_t getUptime() const;

    // ============ Boot ==================

    void incBootCount();
    
private:
    SettingsManager() = default;
    
    // Индекс -> адрес
    uint16_t indexToAddr(MonoWheelRegs::SettingIndex index) const;
    size_t indexToSize(MonoWheelRegs::SettingIndex index) const;
    
    // Внутренние методы
    void processCommand(MonoWheelRegs::Command cmd);
    void processWritePacket(const uint8_t* data, size_t len);
    void startRead();
    void initRegMap();
    void notifyChange(uint16_t addr, const uint8_t* data, size_t size);
    
    // Запись в RAM (без проверок, для внутреннего использования)
    bool writeRamRegister(uint16_t addr, const uint8_t* data, size_t size);
    
    struct RegMeta {
        size_t size;
        bool readable;
        bool writable;
        bool persistent;
        const uint8_t* def_val;
    };
    
    // Хранилища
    std::unordered_map<uint16_t, std::vector<uint8_t>> regs_;
    std::unordered_map<uint16_t, RegMeta> meta_;
    std::unordered_map<MonoWheelRegs::SettingIndex, uint16_t> index_map_;
    
    // Буфер для BLE пакетов
    std::vector<uint8_t> write_buf_;
    
    // Состояние чтения
    bool data_ready_ = false;
    uint16_t read_addr_ = 0;
    uint16_t read_size_ = 0;
    uint16_t read_offset_ = 0;
    
    // Системные
    MonoWheelRegs::DeviceType device_type_;
    MonoWheelRegs::Status status_ = MonoWheelRegs::Status::OK;
    Preferences prefs_;
    SettingsChangeCallback change_cb_ = nullptr;
    
    bool init_ = false;
};