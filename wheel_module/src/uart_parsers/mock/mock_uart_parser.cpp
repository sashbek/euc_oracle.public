#include "mock_uart_parser.h"

namespace MonoWheel {

MockUartParser::MockUartParser() {
    data_.is_connected = true;
    strcpy(data_.model, "Mock Sherman");
    strcpy(data_.version_str, "001.0.00");
}

bool MockUartParser::begin(int8_t rx_pin_) {
    Serial.println("[MOCK] Mock parser initialized");
    return true;
}

void MockUartParser::feed(const uint8_t* data, size_t len) {
    // Ничего не делаем с UART данными
}

void MockUartParser::loop() {
    if (millis() - last_update_ > 200) {
        last_update_ = millis();
        generateData();
        
        if (callback_) {
            callback_(data_);
        }
    }
}

void MockUartParser::onDataUpdated(std::function<void(const WheelData&)> callback) {
    callback_ = callback;
}

void MockUartParser::generateData() {
    static float t = 0;
    t += 0.5f;
    
    // Генерируем синусоидальные данные
    data_.speed = 15.0f + 10.0f * sin(t * 0.5f);
    if (data_.speed < 0) data_.speed = 0;
    
    data_.voltage = 84.0f - (data_.speed / 30.0f) * 5.0f;
    data_.current = 5.0f + data_.speed * 1.5f;
    data_.power = data_.voltage * data_.current;
    data_.pwm = (uint16_t)(data_.speed * 33.3f);
    if (data_.pwm > 1000) data_.pwm = 1000;
    
    data_.battery_level = (uint8_t)((data_.voltage - 70.0f) / 14.0f * 100.0f);
    if (data_.battery_level > 100) data_.battery_level = 100;
    
    data_.temperature = 35.0f + (data_.current / 20.0f) * 15.0f;
    data_.temperature_raw = (int16_t)(data_.temperature * 100);
    
    data_.distance += data_.speed / 3600.0f * 500.0f;  // 500ms в часах
    data_.total_distance += data_.speed / 3600.0f * 500.0f;
    
    data_.phase_current = data_.current;
    data_.pitch_angle = 5.0f * sin(t * 0.3f);
    data_.ride_time = (uint32_t)t;
    
    data_.is_connected = true;
    data_.last_update = millis();
}

} // namespace MonoWheel