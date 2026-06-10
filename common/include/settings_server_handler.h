#pragma once
#include "ble_server_handler.h"
#include <functional>

namespace MonoWheel {

/**
 * @brief Серверный обработчик для настройки с телефона
 */
class SettingsServerHandler : public BleServerHandler {
public:
    void onClientConnected(std::function<void()> cb) { on_connect_cb_ = cb; }
    void onClientDisconnected(std::function<void(int)> cb) { on_disconnect_cb_ = cb; }
    void onDataWritten(std::function<void(const uint8_t*, size_t)> cb) { on_write_cb_ = cb; }
    void onDataRead(std::function<void(uint8_t*, size_t*)> cb) { on_read_cb_ = cb; }
    
    bool isClientConnected() const { return client_connected_; }
    
    /**
     * @brief Отправить уведомление о готовности данных
     */
    void notifyDataReady();
    
    // BleServerHandler
    void initServer(NimBLEServer* server) override;
    
private:
    bool client_connected_ = false;
    NimBLECharacteristic* pReadChar_ = nullptr;
    
    std::function<void()> on_connect_cb_;
    std::function<void(int)> on_disconnect_cb_;
    std::function<void(const uint8_t*, size_t)> on_write_cb_;
    std::function<void(uint8_t*, size_t*)> on_read_cb_;
    
    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(SettingsServerHandler* parent) : parent_(parent) {}
        void onConnect(NimBLEServer*, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer*, NimBLEConnInfo& connInfo, int reason) override;
        void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override;
    private:
        SettingsServerHandler* parent_;
    };
    
    class WriteCallback : public NimBLECharacteristicCallbacks {
    public:
        WriteCallback(SettingsServerHandler* parent) : parent_(parent) {}
        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override;
    private:
        SettingsServerHandler* parent_;
    };
    
    class ReadCallback : public NimBLECharacteristicCallbacks {
    public:
        ReadCallback(SettingsServerHandler* parent) : parent_(parent) {}
        void onRead(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override;
    private:
        SettingsServerHandler* parent_;
    };
};

} // namespace MonoWheel