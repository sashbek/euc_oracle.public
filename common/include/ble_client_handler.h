#pragma once
#include <NimBLEDevice.h>

namespace MonoWheel {

/**
 * @brief Интерфейс для обработчиков BLE клиентов
 */
class BleClientHandler {
public:
    virtual ~BleClientHandler() = default;
    
    /**
     * @brief Проверить, подходит ли устройство этому обработчику
     * @param device Найденное устройство
     * @return true если устройство нужно этому обработчику
     */
    virtual bool isDeviceMatch(const NimBLEAdvertisedDevice* device) = 0;
    
    /**
     * @brief Получить устройство после подключения
     * @param client Подключённый клиент
     */
    virtual void onDeviceConnected(NimBLEClient* client) = 0;
    
    /**
     * @brief Устройство отключилось
     * @param client Отключившийся клиент
     * @param reason Причина отключения
     */
    virtual void onDeviceDisconnected(NimBLEClient* client, int reason) = 0;
    
    /**
     * @brief Нужно ли этому обработчику новое сканирование?
     */
    virtual bool needsScan() = 0;
    
    /**
     * @brief Получить имя обработчика для логов
     */
    virtual const char* getHandlerName() const = 0;
};

} // namespace MonoWheel