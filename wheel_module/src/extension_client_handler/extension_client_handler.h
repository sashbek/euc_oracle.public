#pragma once
#include "Arduino.h"
#include "log_defines.h"
#include "ble_client_handler.h"
#include "wheel_data.h"
#include "extension_protocol.h"
#include <vector>

namespace MonoWheel {

/**
 * @brief Клиентский обработчик для расширяющих устройств [XXXX]
 */
class ExtensionClientHandler : public BleClientHandler {
public:
    static ExtensionClientHandler& getInstance() {
        static ExtensionClientHandler instance;
        return instance;
    }
    
    /**
     * @brief Инициализация
     * @param prefix_code Код группы (4 символа)
     */
    bool begin(const char* prefix_code = "EXT1", uint8_t gp_capacity = 1);
    
    void loop();
    
    /**
     * @brief Отправить данные всем подключённым устройствам
     */
    void broadcast(const WheelData& data);
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    int getDeviceCount() const { return devices_.size(); }
    
    // ============ BleClientHandler interface ============
    bool isDeviceMatch(const NimBLEAdvertisedDevice* device) override;
    void onDeviceConnected(NimBLEClient* client) override;
    void onDeviceDisconnected(NimBLEClient* client, int reason) override;
    bool needsScan() override { return enabled_ && devices_.empty(); }
    const char* getHandlerName() const override { return "Extensions"; }
    
private:
    ExtensionClientHandler() = default;
    
    struct ExtensionDevice {
        NimBLEAddress address;
        NimBLEClient* client = nullptr;
        NimBLERemoteCharacteristic* chr = nullptr;
        bool connected = false;
    };
    
    bool enabled_ = true;
    char prefix_code_[5] = "EXT1";
    uint8_t gp_capacity_ = 1;
    
    std::vector<ExtensionDevice> devices_;
    
    unsigned long last_broadcast_time_ = 0;
    ExtensionPacket packet_;
};

} // namespace MonoWheel