#include "extension_manager.h"
#include <cstring>

namespace MonoWheel {

// ============ ScanCallbacks ============

void ExtensionManager::ScanCallbacks::onResult(const NimBLEAdvertisedDevice* device) {
    // Выводим ВСЕ устройства
    if (!device->haveName()) {
        return;
    }

    Serial.printf("[EXT] Found: %s", device->getAddress().toString().c_str());
    Serial.printf(" - \"%s\"", device->getName().c_str());
    Serial.printf(" (RSSI: %d)\n", device->getRSSI());
    
    if (device->haveName()) {
        std::string name = device->getName();
        
        // Проверяем формат [XXXX]
        if (name.length() >= 6 && 
            name[0] == '[' && name[5] == ']' &&
            strncmp(name.c_str() + 1, parent_->prefix_code_, 4) == 0) {
            
            Serial.printf("[EXT] >>> MATCH: %s <<<\n", name.c_str());
            
            // Проверяем, нет ли уже такого
            for (auto& dev : parent_->devices_) {
                if (dev.address == device->getAddress()) {
                    Serial.println("[EXT] Already in list, skipping");
                    return;
                }
            }
            
            ExtensionDevice dev;
            dev.address = device->getAddress();
            dev.adv_device = new NimBLEAdvertisedDevice(*device);
            dev.connected = false;
            parent_->devices_.push_back(dev);
        }
    }
}

void ExtensionManager::ScanCallbacks::onScanEnd(const NimBLEScanResults& results, int reason) {
    Serial.printf("[EXT] Scan ended, reason: %d, total: %d\n", reason, results.getCount());
    parent_->scanning_ = false;
    
    // Подключаемся ко всем найденным
    for (auto& dev : parent_->devices_) {
        if (!dev.connected && dev.adv_device != nullptr) {
            Serial.printf("[EXT] Connecting to %s...\n", dev.address.toString().c_str());
            parent_->connectToDevice(dev);
        }
    }
}

// ============ begin ============

bool ExtensionManager::begin(const char* prefix_code) {
    if (prefix_code) {
        strncpy(prefix_code_, prefix_code, 4);
        prefix_code_[4] = '\0';
    }
    
    Serial.printf("[EXT] Initialized with prefix: [%s]\n", prefix_code_);
    return true;
}

// ============ startScan ============

void ExtensionManager::startScan() {
    if (scanning_) {
        Serial.println("[EXT] Already scanning");
        return;
    }
    
    scanning_ = true;
    
    // Переинициализируем BLE (как в ThirdEye)
    //if (NimBLEDevice::isInitialized()) {
    //    NimBLEDevice::deinit(true);
    //    delay(300);
    //}
    //
    //NimBLEDevice::init("");
    //NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    //
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan == nullptr) {
        Serial.println("[EXT] Failed to get scan object");
        scanning_ = false;
        return;
    }
    
    pScan->setScanCallbacks(new ScanCallbacks(this), true);
    pScan->setActiveScan(true);
    pScan->setInterval(80);
    pScan->setWindow(40);
    pScan->setMaxResults(0);
    
    Serial.printf("[EXT] Scanning for [%s]...\n", prefix_code_);
    
    bool started = pScan->start(1000, false);
    
    if (!started) {
        Serial.println("[EXT] Failed to start scan");
        scanning_ = false;
    } else {
        Serial.println("[EXT] Scan started, waiting for results...");
    }
}

// ============ loop ============

void ExtensionManager::loop() {
    if (!enabled_) return;
    
    for (auto& dev : devices_) {
        if (dev.connected && dev.client != nullptr) {
            if (!dev.client->isConnected()) {
                Serial.printf("[EXT] Device disconnected: %s\n", dev.address.toString().c_str());
                dev.connected = false;
                dev.chr = nullptr;
                dev.client = nullptr;
            }
        }
    }
}

// ============ broadcast ============

void ExtensionManager::broadcast(const WheelData& data) {
    if (!enabled_) return;
    
    unsigned long now = millis();
    if (now - last_broadcast_time_ < 200) return;
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
    if (data.is_charging) packet_.flags |= FLAG_CHARGING;
    if (data.is_connected) packet_.flags |= FLAG_CONNECTED;
    
    uint8_t* raw = (uint8_t*)&packet_;
    packet_.crc = 0;
    for (size_t i = 0; i < sizeof(packet_) - 1; i++) {
        packet_.crc ^= raw[i];
    }
    
    for (auto& dev : devices_) {
        if (dev.connected && dev.chr != nullptr) {
            dev.chr->writeValue((const uint8_t*)&packet_, sizeof(packet_), true);
        }
    }
}

// ============ connectToDevice ============

bool ExtensionManager::connectToDevice(ExtensionDevice& dev) {
    if (dev.adv_device == nullptr) return false;
    
    Serial.printf("[EXT] Connecting to %s...\n", dev.address.toString().c_str());
    
    dev.client = NimBLEDevice::createClient();
    if (dev.client == nullptr) return false;
    
    if (!dev.client->connect(dev.adv_device, true)) {
        Serial.println("[EXT] Connection failed");
        NimBLEDevice::deleteClient(dev.client);
        dev.client = nullptr;
        delete dev.adv_device;
        dev.adv_device = nullptr;
        return false;
    }
    
    NimBLERemoteService* svc = dev.client->getService(EXTENSION_SERVICE_UUID);
    if (svc == nullptr) {
        Serial.println("[EXT] Service not found");
        dev.client->disconnect();
        NimBLEDevice::deleteClient(dev.client);
        dev.client = nullptr;
        delete dev.adv_device;
        dev.adv_device = nullptr;
        return false;
    }
    
    dev.chr = svc->getCharacteristic(EXTENSION_CHARACTERISTIC_UUID);
    if (dev.chr == nullptr || !dev.chr->canWrite()) {
        Serial.println("[EXT] Characteristic not writable");
        dev.client->disconnect();
        NimBLEDevice::deleteClient(dev.client);
        dev.client = nullptr;
        dev.chr = nullptr;
        delete dev.adv_device;
        dev.adv_device = nullptr;
        return false;
    }
    
    dev.connected = true;
    delete dev.adv_device;
    dev.adv_device = nullptr;
    
    Serial.printf("[EXT] Connected! MTU: %d\n", dev.client->getMTU());
    
    return true;
}

} // namespace MonoWheel