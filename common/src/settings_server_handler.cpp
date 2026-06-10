#include "settings_server_handler.h"
#include "settings_protocol.h"
#include "settings_manager.h"
#include "settings_registers.h"

namespace MonoWheel {

// ============ ServerCallbacks ============

void SettingsServerHandler::ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    parent_->client_connected_ = true;
    
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║           BLE CLIENT CONNECTED               ║");
    Serial.println("╚══════════════════════════════════════════════╝");
    Serial.printf("  Handle: %d\n", connInfo.getConnHandle());
    Serial.printf("  Address: %s\n", connInfo.getAddress().toString().c_str());
    Serial.printf("  MTU: %d\n", connInfo.getMTU());
    Serial.println("═══════════════════════════════════════════════");
    
    if (parent_->on_connect_cb_) {
        parent_->on_connect_cb_();
    }
}

void SettingsServerHandler::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    parent_->client_connected_ = false;
    
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║           BLE CLIENT DISCONNECTED            ║");
    Serial.println("╚══════════════════════════════════════════════╝");
    Serial.printf("  Address: %s\n", connInfo.getAddress().toString().c_str());
    Serial.printf("  Reason: 0x%02X", reason);
    switch (reason) {
        case 0x08: Serial.println(" (TIMEOUT)"); break;
        case 0x13: Serial.println(" (USER_TERMINATED)"); break;
        case 0x16: Serial.println(" (LOCAL_TERMINATED)"); break;
        default: Serial.println();
    }
    Serial.println("═══════════════════════════════════════════════");
    
    if (parent_->on_disconnect_cb_) {
        parent_->on_disconnect_cb_(reason);
    }
    
    // Перезапускаем рекламу
    NimBLEDevice::getAdvertising()->start();
}

void SettingsServerHandler::ServerCallbacks::onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) {
    Serial.printf("[SETTINGS] MTU changed to %d\n", MTU);
}

// ============ WriteCallback ============

void SettingsServerHandler::WriteCallback::onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) {
       std::string value = pChar->getValue();
    const uint8_t* data = (const uint8_t*)value.data();
    size_t len = value.size();
    
    Serial.println("\n┌──────────────────────────────────────────────┐");
    Serial.println("│           BLE WRITE RECEIVED                 │");
    Serial.println("└──────────────────────────────────────────────┘");
    Serial.printf("  Size: %d bytes\n", len);
    Serial.printf("  Time: %lu ms\n", millis());
    
    // Вывод данных
    Serial.print("  Data: ");
    for (size_t i = 0; i < len && i < 32; i++) {
        Serial.printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            Serial.print("\n        ");
        }
    }
    if (len > 32) Serial.print("...");
    Serial.println();
    
    // Проверяем маркеры
    if (len >= 4) {
        bool has_start = SettingsHelpers::check_marker(data, SettingsProtocol::WRITE_START_MARKER);
        Serial.printf("  Start marker: %s\n", has_start ? "FOUND" : "NOT FOUND");
        
        if (len >= 8) {
            bool has_end = false;
            for (size_t i = 0; i + 3 < len; i++) {
                if (SettingsHelpers::check_marker(data + i, SettingsProtocol::WRITE_END_MARKER)) {
                    has_end = true;
                    Serial.printf("  End marker at offset %d\n", i);
                    break;
                }
            }
            if (!has_end) {
                Serial.println("  End marker: NOT FOUND");
            }
        }
    }
    
    // Парсим блоки для отладки
    if (len > 4 && SettingsHelpers::check_marker(data, SettingsProtocol::WRITE_START_MARKER)) {
        size_t pos = 4;
        int block_num = 0;
        while (pos + 4 <= len) {
            // Проверяем на маркер конца
            if (SettingsHelpers::check_marker(data + pos, SettingsProtocol::WRITE_END_MARKER)) {
                break;
            }
            
            uint16_t addr = data[pos] | (data[pos + 1] << 8);
            uint16_t size = data[pos + 2] | (data[pos + 3] << 8);
            pos += 4;
            
            Serial.printf("  Block %d: addr=0x%04X, size=%d", block_num++, addr, size);
            
            if (pos + size <= len) {
                Serial.print(", data=");
                for (size_t i = 0; i < size && i < 8; i++) {
                    Serial.printf("%02X ", data[pos + i]);
                }
                if (size > 8) Serial.print("...");
                Serial.println();
                
                if (addr == 0x0003 && size == 1) {
                    Serial.printf("    -> Command: %s\n", 
                                  data[pos] == 0x01 ? "START_READ" :
                                  data[pos] == 0x02 ? "SAVE_FLASH" :
                                  data[pos] == 0x03 ? "LOAD_FLASH" :
                                  data[pos] == 0x04 ? "RESET_DEFAULTS" : "UNKNOWN");
                }
            }
            pos += size;
        }
    }
    
    Serial.println("──────────────────────────────────────────────");

    auto& settings = SettingsManager::getInstance();
    settings.handleWriteData(data, len);
    
    // ВАЖНО: Сначала передаем данные в SettingsManager
    if (parent_->on_write_cb_) {
        parent_->on_write_cb_(data, len);
    }
    
    // ДИАГНОСТИКА
    auto status = settings.getStatus();
    bool data_ready = settings.isDataReady();
    
    Serial.printf("[BLE] Settings status: 0x%02X, data_ready: %s\n",
                    (uint8_t)status,
                    data_ready ? "YES" : "NO");
    
    if (data_ready) {
        Serial.println("[BLE] Data is ready, preparing to send...");
        
        if (parent_->pReadChar_ && parent_->client_connected_) {
            uint8_t buffer[2] = "\0";
            size_t out_len = 1;
            //size_t out_len = settings.handleReadData(buffer, sizeof(buffer));
            
            Serial.printf("[BLE] handleReadData returned %d bytes\n", out_len);
            
            if (out_len > 0) {
                parent_->pReadChar_->setValue(buffer, out_len);
                parent_->pReadChar_->notify();
                
                Serial.println("\n┌──────────────────────────────────────────────┐");
                Serial.println("│           SENDING NOTIFICATION               │");
                Serial.println("└──────────────────────────────────────────────┘");
                Serial.printf("  Size: %d bytes\n", out_len);
                Serial.print("  Data: ");
                for (size_t i = 0; i < out_len && i < 32; i++) {
                    Serial.printf("%02X ", buffer[i]);
                }
                Serial.println();
                Serial.println("──────────────────────────────────────────────");
            }
        }
    }
}

// ============ ReadCallback ============

void SettingsServerHandler::ReadCallback::onRead(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) {
    Serial.println("\n┌──────────────────────────────────────────────┐");
    Serial.println("│           BLE READ REQUEST                   │");
    Serial.println("└──────────────────────────────────────────────┘");
    Serial.printf("  Time: %lu ms\n", millis());
    
    uint8_t buffer[128] = {0};
    size_t len = 0;
    
    if (parent_->on_read_cb_) {
        parent_->on_read_cb_(buffer, &len);
    }
    
    if (len > 0) {
        pChar->setValue(buffer, len);
        
        Serial.printf("  Returning: %d bytes\n", len);
        Serial.print("  Data: ");
        for (size_t i = 0; i < len && i < 32; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        if (len > 32) Serial.print("...");
        Serial.println();
    } else {
        Serial.println("  No data to return");
    }
    
    Serial.println("──────────────────────────────────────────────");
}

// ============ initServer ============

void SettingsServerHandler::initServer(NimBLEServer* server) {
    server->setCallbacks(new ServerCallbacks(this));
    
    NimBLEService* svc = server->createService(SETTINGS_SERVICE_UUID);
    
    auto* wChar = svc->createCharacteristic(SETTINGS_WRITE_CHAR_UUID, NIMBLE_PROPERTY::WRITE);
    wChar->setCallbacks(new WriteCallback(this));
    
    auto* rChar = svc->createCharacteristic(SETTINGS_READ_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    rChar->setCallbacks(new ReadCallback(this));
    
    pReadChar_ = rChar;
    
    svc->start();
    
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SETTINGS_SERVICE_UUID);
}

// ============ notifyDataReady ============

void SettingsServerHandler::notifyDataReady() {
    if (pReadChar_ && client_connected_) {
        pReadChar_->notify();
        Serial.println("[SETTINGS] Notification sent: data ready");
    }
}

} // namespace MonoWheel