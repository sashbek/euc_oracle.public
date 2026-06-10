#include "extension_server_handler.h"

namespace MonoWheel {

// ============ DataCallback ============

void ExtensionServerHandler::DataCallback::onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) {
    std::string value = pChar->getValue();
    
    if (value.size() != sizeof(ExtensionPacket)) {
        return;
    }
    
    const ExtensionPacket* packet = (const ExtensionPacket*)value.data();
    
    // Проверка CRC
    uint8_t crc = 0;
    const uint8_t* raw = (const uint8_t*)packet;
    for (size_t i = 0; i < sizeof(ExtensionPacket) - 1; i++) {
        crc ^= raw[i];
    }
    
    if (crc != packet->crc) {
        return;
    }
    
    // Сохраняем пакет
    parent_->last_packet_ = *packet;
    parent_->last_packet_time_ = millis();
    parent_->packet_count_++;
    parent_->data_updated_ = true;
    
    // Вызываем колбэк
    if (parent_->data_callback_) {
        parent_->data_callback_(*packet);
    }
}

// ============ ServerCallbacks ============

void ExtensionServerHandler::ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    parent_->connected_ = true;
    if (parent_->conn_callback_) {
        parent_->conn_callback_(true);
    }
}

void ExtensionServerHandler::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    parent_->connected_ = false;
    if (parent_->conn_callback_) {
        parent_->conn_callback_(false);
    }
}

// ============ setDeviceName ============

void ExtensionServerHandler::setDeviceName(const char* name) {
    if (name) {
        strncpy(device_name_, name, sizeof(device_name_) - 1);
        device_name_[sizeof(device_name_) - 1] = '\0';
    }
}

// ============ initServer ============

void ExtensionServerHandler::initServer(NimBLEServer* server) {
    server->setCallbacks(new ServerCallbacks(this));
    
    NimBLEService* svc = server->createService(EXTENSION_SERVICE_UUID);
    
    NimBLECharacteristic* chr = svc->createCharacteristic(
        EXTENSION_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    chr->setCallbacks(new DataCallback(this));
    
    svc->start();
    
    // Настройка рекламы
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(EXTENSION_SERVICE_UUID);
    
    NimBLEAdvertisementData advData;
    advData.setName(device_name_);
    advData.setCompleteServices(NimBLEUUID(EXTENSION_SERVICE_UUID));
    adv->setAdvertisementData(advData);
    
    Serial.printf("[EXT_SRV] Server initialized: %s\n", device_name_);
}

// ============ isDataUpdated ============

bool ExtensionServerHandler::isDataUpdated() {
    if (data_updated_) {
        data_updated_ = false;
        return true;
    }
    return false;
}

} // namespace MonoWheel