#pragma once
#include <NimBLEDevice.h>

namespace MonoWheel {

/**
 * @brief Интерфейс для обработчика BLE сервера
 */
class BleServerHandler {
public:
    virtual ~BleServerHandler() = default;
    
    /**
     * @brief Создан сервер
     * @param server Отключившийся клиент
     */
    virtual void initServer(NimBLEServer* server) = 0;
    
};

} // namespace MonoWheel