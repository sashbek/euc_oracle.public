#pragma once
#include "../uart_parser.h"

namespace MonoWheel {

class VeteranUartParser : public UartParser {
public:
    VeteranUartParser(HardwareSerial& serial, uint32_t baud_rate = 115200);
    
    bool begin(int8_t rx_pin) override;
    void feed(const uint8_t* data, size_t len) override;
    void loop() override;
    const WheelData& getData() const override { return data_; }
    bool isConnected() const override { return data_.is_connected; }
    void onDataUpdated(std::function<void(const WheelData&)> callback) override;
    const char* getName() const override { return "Veteran"; }
    
    // ISR
    void IRAM_ATTR processByteISR(uint8_t byte);
    
    // Задача
    void processPacket();
    
    HardwareSerial& serial_;
private:
    uint32_t baud_rate_;
    int8_t rx_pin_ = -1;
    WheelData data_;
    std::function<void(const WheelData&)> callback_;
    
    TaskHandle_t packet_task_handle_ = nullptr;
    
    // ISR данные
    volatile bool packet_ready_ = false;
    volatile uint32_t packet_cnt = 0;
    volatile uint32_t packet_cnt_old = 0;
    volatile uint8_t sync_state_ = 0;
    volatile uint8_t raw_index_ = 0;
    uint8_t raw_buffer_[43];
    
    unsigned long last_data_time_ = 0;
    
    void parsePacket(const uint8_t* data, size_t len);
    void updateModel(int ver);
    int calculateBatteryLevel(int voltage, int ver);
};

} // namespace MonoWheel