#pragma once
#include "../uart_parser.h"

namespace MonoWheel {

/**
 * @brief Мок-парсер для тестирования (генерирует фейковые данные)
 */
class MockUartParser : public UartParser {
public:
    MockUartParser();
    
    bool begin(int8_t rx_pin_) override;
    void feed(const uint8_t* data, size_t len) override;
    void loop() override;
    const WheelData& getData() const override { return data_; }
    bool isConnected() const override { return true; }
    void onDataUpdated(std::function<void(const WheelData&)> callback) override;
    const char* getName() const override { return "Mock"; }
    
private:
    WheelData data_;
    std::function<void(const WheelData&)> callback_;
    unsigned long last_update_ = 0;
    
    void generateData();
};

} // namespace MonoWheel