#pragma once
#include <Arduino.h>
#include <functional>
#include "wheel_data.h"

namespace MonoWheel {

/**
 * @brief Базовый интерфейс для всех UART парсеров
 */
class UartParser {
public:
    virtual ~UartParser() = default;
    
    /**
     * @brief Инициализация парсера
     * @return true если успешно
     */
    virtual bool begin(int8_t rx_pin_) = 0;
    
    /**
     * @brief Обработка входящих данных
     * @param data Указатель на данные
     * @param len Длина данных
     */
    virtual void feed(const uint8_t* data, size_t len) = 0;
    
    /**
     * @brief Основной цикл (вызывать в loop)
     */
    virtual void loop() = 0;
    
    /**
     * @brief Получить последние данные
     * @return Константная ссылка на структуру данных
     */
    virtual const WheelData& getData() const = 0;
    
    /**
     * @brief Проверить, подключено ли колесо
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Установить колбэк при обновлении данных
     */
    virtual void onDataUpdated(std::function<void(const WheelData&)> callback) = 0;
    
    /**
     * @brief Получить имя парсера
     */
    virtual const char* getName() const = 0;
};

// Тип колбэка для обновления данных
using DataUpdateCallback = std::function<void(const WheelData&)>;

} // namespace MonoWheel