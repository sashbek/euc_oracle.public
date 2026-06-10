#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include "wheel_data.h"

namespace MonoWheel {

// UUID сервиса для расширений
#define EXTENSION_SERVICE_UUID        "e19752de-87eb-45b0-afbb-a526aeba51aa"
#define EXTENSION_CHARACTERISTIC_UUID "baf7cc6c-7858-4225-a071-d6a73403ff26"

// Размер пакета данных (21 байт)
#pragma pack(push, 1)
struct ExtensionPacket {
    uint8_t header;          // 0xAA
    uint16_t speed;          // км/ч * 100 (2 байта)
    uint8_t battery;         // % (1 байт)
    uint16_t pwm;            // ШИМ * 10 (2 байта)
    int16_t temperature;     // °C * 10 (2 байта)
    uint8_t flags;           // Битовые флаги (1 байт)
    uint16_t voltage;        // В * 100 (2 байта)
    uint16_t current;        // А * 100 (2 байта)
    uint16_t power;          // Вт * 10 (2 байта)
    uint16_t distance;       // км * 10 (2 байта)
    uint32_t ride_time;      // секунды (4 байта)
    uint8_t crc;             // XOR всех байтов (1 байт)
    // Итого: 21 байт
};
#pragma pack(pop)

// Битовые флаги
enum ExtensionFlags : uint8_t {
    FLAG_CHARGING = 0x01,
    FLAG_CONNECTED = 0x02,
    FLAG_ALERT = 0x04,
};

/**
 * @brief Менеджер расширяющих устройств
 * 
 * После подключения ThirdEye ищет устройства с именем [XXXX]* 
 * и отправляет им данные в одностороннем порядке.
 */
class ExtensionManager {
public:
    static ExtensionManager& getInstance() {
        static ExtensionManager instance;
        return instance;
    }
    
    /**
     * @brief Инициализация
     * @param prefix_code Код группы (4 символа)
     */
    bool begin(const char* prefix_code = "EXT1");
    
    /**
     * @brief Основной цикл
     */
    void loop();
    
    /**
     * @brief Отправить данные всем подключённым устройствам
     */
    void broadcast(const WheelData& data);
    
    /**
     * @brief Включить/выключить
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    /**
     * @brief Количество подключённых устройств
     */
    int getDeviceCount() const { return devices_.size(); }
    
    /**
     * @brief Найти устройства группы и пдключить
     */
    void startScan();
private:
    ExtensionManager() = default;
    
    // Устройство
    struct ExtensionDevice {
        NimBLEAddress address;
        NimBLEAdvertisedDevice* adv_device = nullptr;  // Только до подключения
        NimBLEClient* client = nullptr;
        NimBLERemoteCharacteristic* chr = nullptr;
        bool connected = false;
    };
    
    // Сканирование
    class ScanCallbacks : public NimBLEScanCallbacks {
    public:
        ScanCallbacks(ExtensionManager* parent) : parent_(parent) {}
        void onResult(const NimBLEAdvertisedDevice* device) override;
        void onScanEnd(const NimBLEScanResults& results, int reason) override;
    private:
        ExtensionManager* parent_;
    };
    
    bool connectToDevice(ExtensionDevice& dev);
    bool sendPacket(ExtensionDevice& dev, const WheelData& data);
    
    bool enabled_ = true;
    char prefix_code_[5] = "EXT1";
    
    std::vector<ExtensionDevice> devices_;
    
    bool scanning_ = false;
    unsigned long last_scan_time_ = 0;
    unsigned long last_broadcast_time_ = 0;
    
    // Буфер для пакета
    ExtensionPacket packet_;
};

} // namespace MonoWheel
