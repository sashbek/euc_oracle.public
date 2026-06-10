#pragma once
#include <Arduino.h>
#include <functional>
#include "extension_protocol.h"
#include "ble_server_handler.h"

namespace MonoWheel {

// Forward declaration
class BleServerHandler;

/**
 * @brief Серверный обработчик для устройств-расширений
 * 
 * Реализует BleServerHandler для регистрации в BleManager.
 * Принимает данные от основного устройства (колеса) и предоставляет их
 * через колбэки или опрос.
 */
class ExtensionServerHandler : public BleServerHandler {
public:
    static ExtensionServerHandler& getInstance() {
        static ExtensionServerHandler instance;
        return instance;
    }
    
    // ============ Конфигурация ============
    
    /**
     * @brief Установить имя устройства
     */
    void setDeviceName(const char* name);
    
    // ============ Статус ============
    
    bool isConnected() const { return connected_; }
    
    /**
     * @brief Получить последние полученные данные
     */
    const ExtensionPacket& getData() const { return last_packet_; }
    
    /**
     * @brief Проверить, были ли обновлены данные
     */
    bool isDataUpdated();
    
    /**
     * @brief Получить количество принятых пакетов
     */
    unsigned long getPacketCount() const { return packet_count_; }
    
    /**
     * @brief Получить время последнего пакета
     */
    unsigned long getLastPacketTime() const { return last_packet_time_; }
    
    // ============ Колбэки ============
    
    void onDataReceived(std::function<void(const ExtensionPacket&)> callback) {
        data_callback_ = callback;
    }
    
    void onConnectionChanged(std::function<void(bool connected)> callback) {
        conn_callback_ = callback;
    }
    
    // ============ BleServerHandler interface ============
    void initServer(NimBLEServer* server) override;
    
private:
    ExtensionServerHandler() = default;
    
    // BLE колбэки
    class DataCallback : public NimBLECharacteristicCallbacks {
    public:
        DataCallback(ExtensionServerHandler* parent) : parent_(parent) {}
        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override;
    private:
        ExtensionServerHandler* parent_;
    };
    
    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(ExtensionServerHandler* parent) : parent_(parent) {}
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    private:
        ExtensionServerHandler* parent_;
    };
    
    friend class DataCallback;
    friend class ServerCallbacks;
    
    // Состояние
    bool connected_ = false;
    unsigned long last_packet_time_ = 0;
    unsigned long packet_count_ = 0;
    bool data_updated_ = false;
    
    // Имя устройства
    char device_name_[32] = "[EXT1]Device";
    
    // Последний пакет
    ExtensionPacket last_packet_;
    
    // Колбэки
    std::function<void(const ExtensionPacket&)> data_callback_;
    std::function<void(bool connected)> conn_callback_;
};

} // namespace MonoWheel