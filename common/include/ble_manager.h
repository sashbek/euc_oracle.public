#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include "log_defines.h"
#include "ble_client_handler.h"
#include "ble_server_handler.h"

namespace MonoWheel {

enum class BleMode {
    OFF,
    SERVER,
    SCAN,
    CLIENT,
};

class BleManager {
public:
    static BleManager& getInstance() {
        static BleManager instance;
        return instance;
    }
    
    // ============ Инициализация ============
    
    /**
     * @brief Запустить режим сервера
     */
    bool beginServer(const char* device_name);
    
    /**
     * @brief Остановить сервер
     */
    void stopServer();
    
    /**
     * @brief Запустить режим сканирования
     */
    void beginScan();
    
    /**
     * @brief Остановить режим сканирования
     */
    void stopScan();
    
    /**
     * @brief Запустить режим клиента
     */
    void beginClient();
    
    /**
     * @brief Остановить режим клиента
     */
    void stopClient();

    // ============ Регистрация обработчиков ============
    
    /**
     * @brief Зарегистрировать обработчик устройств
     */
    void registerClientHandler(BleClientHandler* handler);
    
    /**
     * @brief Зарегистрировать обработчик устройств
     */
    void registerServerHandler(BleServerHandler* handler);
    
    // ============ Основной цикл ============
    
    void loop();
    
    // ============ Сканирование ============
    
    /**
     * @brief Запросить сканирование
     */
    void requestScan();
    
    /**
     * @brief Идёт ли сканирование?
     */
    bool isScanning() const { return scanning_; }
    
    // ============ Статус ============
    
    BleMode getMode() const { return mode_; }
    int getConnectedCount() const;
    int getPendingsCount() const;
    
    void connectPendingDevices();
    
private:
    BleManager() = default;
    
    // Режим
    BleMode mode_ = BleMode::OFF;
    
    // Обработчики клиентов
    std::vector<BleClientHandler*> client_handlers_;
    
    // Обработчик сервера
    BleServerHandler* server_handler_;

    // Сканирование
    bool scanning_ = false;
    unsigned long last_scan_time_ = 0;
    
    // Найденные устройства (адрес -> обработчик)
    struct PendingDevice {
        NimBLEAdvertisedDevice* adv_device;
        BleClientHandler* handler;
    };
    std::vector<PendingDevice> pending_devices_;
    
    // Подключённые устройства
    struct ConnectedDevice {
        NimBLEClient* client;
        BleClientHandler* handler;
    };
    std::vector<ConnectedDevice> connected_devices_;
    
    // Колбэки
    class ScanCallbacks : public NimBLEScanCallbacks {
    public:
        ScanCallbacks(BleManager* parent) : parent_(parent) {}
        void onResult(const NimBLEAdvertisedDevice* device) override;
        void onScanEnd(const NimBLEScanResults& results, int reason) override;
    private:
        BleManager* parent_;
    };
    
    friend class ScanCallbacks;
};

} // namespace MonoWheel