#include "ble_manager.h"

namespace MonoWheel {

// ============ ScanCallbacks ============

void BleManager::ScanCallbacks::onResult(const NimBLEAdvertisedDevice* device) {
    if (!device->haveName())
        return;

    LOG_F("[BLE] Scan: %s", device->getAddress().toString().c_str());
    if (device->haveName()) {
        LOG_F(" \"%s\"", device->getName().c_str());
    }
    LOG_F(" (RSSI: %d)\n", device->getRSSI());
    
    for (auto* handler : parent_->client_handlers_) {
        if (handler->isDeviceMatch(device)) {
            LOG_F("[BLE] -> Accepted by %s\n", handler->getHandlerName());
            
            // Проверяем, нет ли уже такого в pending или connected
            NimBLEAddress addr = device->getAddress();
            
            for (auto& pd : parent_->pending_devices_) {
                if (pd.adv_device->getAddress() == addr) return;
            }
            for (auto& cd : parent_->connected_devices_) {
                if (cd.client->getPeerAddress() == addr) return;
            }
            
            PendingDevice pd;
            pd.adv_device = new NimBLEAdvertisedDevice(*device);
            pd.handler = handler;
            parent_->pending_devices_.push_back(pd);

            break;
        }
    }
}

void BleManager::ScanCallbacks::onScanEnd(const NimBLEScanResults& results, int reason) {
    LOG_F("[BLE] Scan ended: %d total, %d pending\n", 
                  results.getCount(), parent_->pending_devices_.size());
    parent_->scanning_ = false;
    //parent_->connectPendingDevices();
}

// ============ beginServer ============

bool BleManager::beginServer(const char* device_name) {
    if (server_handler_ == nullptr) {
        LOG_LN("[BLE] No server handler registered");
        return false;
    }
    
    mode_ = BleMode::SERVER;
    
    NimBLEDevice::init(device_name);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    
    NimBLEServer* server = NimBLEDevice::createServer();
    
    // Хендлер сам настраивает сервер
    server_handler_->initServer(server);
    
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData scanData;
    scanData.setName(device_name);
    adv->setScanResponseData(scanData);
    adv->start();
    
    LOG_F("[BLE] Server started: %s\n", device_name);
    return true;
}

void BleManager::stopServer() {
    if (mode_ != BleMode::SERVER) return;
    
    NimBLEDevice::getAdvertising()->stop();
    NimBLEDevice::deinit(true);
    
    server_handler_ = nullptr;
    mode_ = BleMode::OFF;
    
    LOG_LN("[BLE] Server stopped");
}

// ============ beginScan ============

void BleManager::beginScan() {
    mode_ = BleMode::SCAN;
    scanning_ = false;
    last_scan_time_ = 0;
    
    // Переинициализируем BLE
    if (NimBLEDevice::isInitialized()) {
        NimBLEDevice::deinit(true);
        delay(300);
    }
    
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    
    LOG_LN("[BLE] Scan mode ready");
}

void BleManager::stopScan() {
    if (scanning_) {
        LOG_LN("[BLE] Scan stopped");
        NimBLEDevice::getScan()->stop();
        scanning_ = false;
    }
    delay(500);
}

// ============ beginClient ============

void BleManager::beginClient() {
    mode_ = BleMode::CLIENT;
    
    if (!NimBLEDevice::isInitialized()) {
        NimBLEDevice::init("");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    }
    
    LOG_LN("[BLE] Client mode ready");
}

void BleManager::stopClient() {
    // Отключаем всех
    for (auto& cd : connected_devices_) {
        cd.client->disconnect();
        NimBLEDevice::deleteClient(cd.client);
    }
    connected_devices_.clear();
}

// ============ Регистрация ============

void BleManager::registerClientHandler(BleClientHandler* handler) {
    client_handlers_.push_back(handler);
    LOG_F("[BLE] Registered client handler: %s\n", handler->getHandlerName());
}

void BleManager::registerServerHandler(BleServerHandler* handler) {
    server_handler_ = handler;
    LOG_F("[BLE] Registered server handler\n");
}

// ============ requestScan ============

void BleManager::requestScan() {
    if (scanning_) return;
    
    scanning_ = true;
    
    // Очищаем старые pending
    for (auto& pd : pending_devices_) {
        delete pd.adv_device;
    }
    pending_devices_.clear();
    
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan == nullptr) {
        LOG_LN("[BLE] Failed to get Scan entity");
        scanning_ = false;
        return;
    }
    
    pScan->setScanCallbacks(new ScanCallbacks(this), false);
    pScan->clearResults();
    pScan->setActiveScan(true);
    pScan->setInterval(80);
    pScan->setWindow(40);
    pScan->setMaxResults(0);
    
    LOG_LN("[BLE] Scan started");
    bool started = pScan->start(5000, false);  // 5 секунд
    
    if (!started) {
        LOG_LN("[BLE] Failed to start scan");
    } else {
        LOG_LN("[BLE] Scan started, waiting for results...");
    }
}

// ============ connectPendingDevices ============

void BleManager::connectPendingDevices() {
    delay(100);
    LOG_LN("[BLE] Connecting to pending devices...");
    for (auto& pd : pending_devices_) {
        NimBLEAddress addr = pd.adv_device->getAddress();

        LOG_F("[BLE] Connecting to %s for %s...\n", 
                      addr.toString().c_str(),
                      pd.handler->getHandlerName());

        //NimBLEDevice::setMTU(32);

        NimBLEClient* pClient_ = NimBLEDevice::createClient();
        if (pClient_ == nullptr) {
            Serial.println("[BLE] Failed to create BLE client");
            continue;
        }
        pClient_->setConnectionParams(6, 12, 0, 200);
        
        Serial.printf("[BLE] Attempting connection...\n");

        bool connected = pClient_->connect(pd.adv_device, true, false);

        if (!connected) {
            Serial.printf("[BLE] First attempt failed, trying with params...\n");
            pClient_->setConnectionParams(6, 12, 0, 200);
            pClient_->setConnectTimeout(10);
            connected = pClient_->connect(addr, true);
        }

        if (!connected) {
            LOG_F("[BLE] Connection failed\n");
            NimBLEDevice::deleteClient(pClient_);
            delete pd.adv_device;
            continue;;
        }
        
        LOG_F("[BLE] Connected! MTU: %d\n", pClient_->getMTU());
        
        ConnectedDevice cd;
        cd.client = pClient_;
        cd.handler = pd.handler;
        connected_devices_.push_back(cd);
        
        pd.handler->onDeviceConnected(pClient_);
        
        delete pd.adv_device;
    }
    pending_devices_.clear();
}

// ============ loop ============

void BleManager::loop() {
    for (auto it = connected_devices_.begin(); it != connected_devices_.end(); ) {
        if (!it->client->isConnected()) {
            LOG_F("[BLE] Disconnected: %s\n", it->handler->getHandlerName());
            it->handler->onDeviceDisconnected(it->client, 0);
            NimBLEDevice::deleteClient(it->client);
            it = connected_devices_.erase(it);
        } else {
            ++it;
        }
    }
}

int BleManager::getConnectedCount() const {
    return connected_devices_.size();
}


int BleManager::getPendingsCount() const {
    return pending_devices_.size();
}

} // namespace MonoWheel