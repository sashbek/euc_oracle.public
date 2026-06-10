#include "extension_client_handler.h"
#include <cstring>

namespace MonoWheel {

bool ExtensionClientHandler::begin(const char* prefix_code, uint8_t gp_capacity) {
    if (prefix_code) {
        strncpy(prefix_code_, prefix_code, 4);
        prefix_code_[4] = '\0';
    }

    gp_capacity_ = gp_capacity;
    
    LOG_F("[EXT] Initialized with prefix: [%s], capacity: %u\n", prefix_code_, gp_capacity_);
    return true;
}

// ============ BleClientHandler interface ============

bool ExtensionClientHandler::isDeviceMatch(const NimBLEAdvertisedDevice* device) {
    if (!device->haveName()) return false;
    
    std::string name = device->getName();
    
    // Проверяем формат [XXXX]
    if (name.length() >= 6 && 
        name[0] == '[' && name[5] == ']' &&
        strncmp(name.c_str() + 1, prefix_code_, 4) == 0) {
        
        LOG_F("[EXT] Match: %s\n", name.c_str());
        return true;
    }
    
    return false;
}

void ExtensionClientHandler::onDeviceConnected(NimBLEClient* client) {
    if (devices_.size() >= gp_capacity_) {
        LOG_LN("[EXT] Too much ext devices, increase capacity");
        client->disconnect();
        return;
    }

    // Получаем сервис и характеристику
    NimBLERemoteService* svc = client->getService(EXTENSION_SERVICE_UUID);
    if (svc == nullptr) {
        LOG_LN("[EXT] Service not found");
        client->disconnect();
        return;
    }
    
    NimBLERemoteCharacteristic* chr = svc->getCharacteristic(EXTENSION_CHARACTERISTIC_UUID);
    if (chr == nullptr || !chr->canWrite()) {
        LOG_LN("[EXT] Characteristic not writable");
        client->disconnect();
        return;
    }
    
    // Добавляем устройство
    ExtensionDevice dev;
    dev.address = client->getPeerAddress();
    dev.client = client;
    dev.chr = chr;
    dev.connected = true;
    devices_.push_back(dev);
    
    LOG_F("[EXT] Device connected: %s, total: %d\n", 
                  dev.address.toString().c_str(), devices_.size());
}

void ExtensionClientHandler::onDeviceDisconnected(NimBLEClient* client, int reason) {
    NimBLEAddress addr = client->getPeerAddress();
    
    devices_.erase(
        std::remove_if(devices_.begin(), devices_.end(),
            [&addr](const ExtensionDevice& dev) {
                return dev.address == addr;
            }),
        devices_.end()
    );
    
    LOG_F("[EXT] Device disconnected: %s, total: %d\n", 
                  addr.toString().c_str(), devices_.size());
}

// ============ loop ============

void ExtensionClientHandler::loop() {
    if (!enabled_) return;
    
    // Проверка подключённых устройств
    for (auto it = devices_.begin(); it != devices_.end(); ) {
        if (it->client == nullptr || !it->client->isConnected()) {
            LOG_F("[EXT] Removing dead device: %s\n", it->address.toString().c_str());
            it = devices_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============ broadcast ============

void ExtensionClientHandler::broadcast(const WheelData& data) {
    if (!enabled_ || devices_.empty()) return;
    
    unsigned long now = millis();
    if (now - last_broadcast_time_ < 200) return;  // Не чаще 5 раз в секунду
    last_broadcast_time_ = now;
    
    // Заполняем пакет
    memset(&packet_, 0, sizeof(packet_));
    packet_.header = 0xAA;
    packet_.speed = (uint16_t)(data.speed * 100);
    packet_.battery = data.battery_level;
    packet_.pwm = (uint16_t)(data.pwm * 10);
    packet_.temperature = (int16_t)(data.temperature * 10);
    packet_.voltage = (uint16_t)(data.voltage * 100);
    packet_.current = (uint16_t)(data.current * 100);
    packet_.power = (uint16_t)(data.power * 10);
    packet_.distance = (uint16_t)(data.distance / 100);
    packet_.ride_time = data.ride_time;
    
    packet_.flags = 0;
    if (data.is_charging) packet_.flags |= EXT_FLAG_CHARGING;
    if (data.is_connected) packet_.flags |= EXT_FLAG_CONNECTED;
    
    // CRC
    uint8_t* raw = (uint8_t*)&packet_;
    packet_.crc = 0;
    for (size_t i = 0; i < sizeof(packet_) - 1; i++) {
        packet_.crc ^= raw[i];
    }
    
    // Отправляем всем
    for (auto& dev : devices_) {
        if (dev.connected && dev.chr != nullptr) {
            dev.chr->writeValue((const uint8_t*)&packet_, sizeof(packet_), true);
        }
    }
}

} // namespace MonoWheel