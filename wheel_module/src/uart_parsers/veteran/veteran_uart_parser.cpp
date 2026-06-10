#include "veteran_uart_parser.h"
#include <cstdio>
#include <cstring>

namespace MonoWheel {

static VeteranUartParser* g_veteran_instance = nullptr;

// ============ ISR ============

static void IRAM_ATTR veteranUartISR() {
    if (g_veteran_instance == nullptr) return;

    Serial.print("[T1] ");
    Serial.println(millis());
    
    while (g_veteran_instance->serial_.available()) {
        uint8_t byte = g_veteran_instance->serial_.read();
        g_veteran_instance->processByteISR(byte);
    }
}

// ============ Задача обработки ============

static void packetTask(void* param) {
    VeteranUartParser* parser = (VeteranUartParser*)param;
    
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        parser->processPacket();
    }
}

// ============ Конструктор ============

VeteranUartParser::VeteranUartParser(HardwareSerial& serial, uint32_t baud_rate)
    : serial_(serial), baud_rate_(baud_rate), callback_(nullptr) {
}

// ============ begin ============

bool VeteranUartParser::begin(int8_t rx_pin) {
    rx_pin_ = rx_pin;
    
    if (rx_pin_ >= 0) {
        serial_.begin(baud_rate_, SERIAL_8N1, rx_pin_, -1);
    } else {
        serial_.begin(baud_rate_, SERIAL_8N1);
    }
    
    g_veteran_instance = this;
    serial_.onReceive(veteranUartISR);
    
    BaseType_t ret = xTaskCreate(packetTask, "veteran_pkt", 4096, this, 2, &packet_task_handle_);
    
    Serial.printf("[VETERAN] Initialized: baud=%d, RX=GPIO%d, task=%s\n", 
                  baud_rate_, rx_pin_, ret == pdPASS ? "OK" : "FAIL");
    return true;
}

// ============ feed ============

void VeteranUartParser::feed(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        processByteISR(data[i]);
    }
}

// ============ loop ============

void VeteranUartParser::loop() {
    if (data_.is_connected && (millis() - last_data_time_ > 1000)) {
        data_.is_connected = false;
    }
    
    // Только колбэк — всё остальное в main.cpp
    if (callback_ && packet_cnt_old < packet_cnt) {
        packet_cnt_old = packet_cnt;
        callback_(data_);
    }
}

// ============ onDataUpdated ============

void VeteranUartParser::onDataUpdated(std::function<void(const WheelData&)> callback) {
    callback_ = callback;
}

// ============ processByteISR (минимальный ISR) ============

void IRAM_ATTR VeteranUartParser::processByteISR(uint8_t byte) {
    switch (sync_state_) {
        case 0:  // Ждём 0xDC
            if (byte == 0xDC) {
                sync_state_ = 1;
            }
            break;
            
        case 1:  // Ждём 0x5A
            if (byte == 0x5A) {
                sync_state_ = 2;
            } else {
                sync_state_ = (byte == 0xDC) ? 1 : 0;
            }
            break;
            
        case 2:  // Ждём 0x5C
            if (byte == 0x5C) {
                // Найден заголовок! Копируем заголовок и начинаем сбор данных
                raw_buffer_[0] = 0xDC;
                raw_buffer_[1] = 0x5A;
                raw_buffer_[2] = 0x5C;
                raw_index_ = 3;
                sync_state_ = 3;
            } else {
                sync_state_ = (byte == 0xDC) ? 1 : 0;
            }
            break;
            
        case 3:  // Собираем 40 байт данных
            raw_buffer_[raw_index_] = byte;
            raw_index_++;
            
            if (raw_index_ >= 43) {  // 3 заголовок + 40 данные
                // Сбрасываем остаток пакета
                serial_.flush();
                
                // Уведомляем задачу
                packet_ready_ = true;
                sync_state_ = 0;
                
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xTaskNotifyFromISR(packet_task_handle_, 0, eNoAction, &xHigherPriorityTaskWoken);
                if (xHigherPriorityTaskWoken) {
                    portYIELD_FROM_ISR();
                }
            }
            break;
    }
}

// ============ processPacket (в задаче) ============

void VeteranUartParser::processPacket() {
    if (!packet_ready_) return;
    packet_ready_ = false;
    
    // Копируем буфер локально для безопасности
    uint8_t local_buffer[43];
    memcpy(local_buffer, raw_buffer_, 43);
    
    parsePacket(local_buffer, 43);
}

// ============ parsePacket ============

void VeteranUartParser::parsePacket(const uint8_t* data, size_t len) {
    if (len < 38) return;
    
    int voltage_raw       = (data[4] << 8) | data[5];
    int speed_raw         = (int16_t)((data[6] << 8) | data[7]);
    int distance_raw      = (data[10] << 24) | (data[11] << 16) | (data[8] << 8) | data[9];
    int total_distance_raw = (data[14] << 24) | (data[15] << 16) | (data[12] << 8) | data[13];
    int phase_current_raw = (int16_t)((data[16] << 8) | data[17]);
    int temp_raw          = (int16_t)((data[18] << 8) | data[19]);
    int auto_off          = (data[20] << 8) | data[21];
    int charge_mode       = (data[22] << 8) | data[23];
    int ver               = (data[28] << 8) | data[29];
    int pitch_raw         = (int16_t)((data[32] << 8) | data[33]);
    int hw_pwm            = (data[34] << 8) | data[35];
    
    data_.voltage        = voltage_raw / 100.0f;
    data_.speed          = abs(speed_raw) / 10.0f;
    data_.distance       = distance_raw;
    data_.total_distance = total_distance_raw;
    data_.phase_current  = abs(phase_current_raw) / 10.0f;
    data_.temperature_raw = temp_raw;
    data_.temperature    = temp_raw / 100.0f;
    data_.auto_off_sec   = auto_off;
    data_.charge_mode    = charge_mode;
    data_.version_raw    = ver;
    data_.pitch_angle    = pitch_raw / 100.0f;
    data_.pwm            = hw_pwm / 100.0f;
    data_.is_charging    = (charge_mode != 0);
    
    snprintf(data_.version_str, sizeof(data_.version_str), "%03d.%01d.%02d", 
             ver / 1000, (ver % 1000) / 100, ver % 100);
    
    updateModel(ver);
    data_.battery_level = calculateBatteryLevel(voltage_raw, ver);
    data_.current = data_.phase_current * 0.7f;
    data_.power   = data_.voltage * data_.current;
    data_.is_connected = true;
    last_data_time_ = millis();
    
    ++packet_cnt;
}

// ============ updateModel ============

void VeteranUartParser::updateModel(int ver) {
    const char* model = "Unknown";
    
    if (ver <= 1000)      model = "Sherman";
    else if (ver <= 2000) model = "Abrams";
    else if (ver <= 3000) model = "Sherman S";
    else if (ver <= 4000) model = "Patton";
    else if (ver <= 5000) model = "Lynx";
    else if (ver <= 6000) model = "Sherman L";
    else if (ver <= 7000) model = "Patton S";
    else if (ver <= 8000) model = "Oryx";
    else if (ver == 42000) model = "Nosfet Apex";
    else if (ver == 43000) model = "Nosfet Aero";
    else if (ver == 44000) model = "Nosfet Aeon";
    
    strncpy(data_.model, model, sizeof(data_.model) - 1);
}

// ============ calculateBatteryLevel ============

int VeteranUartParser::calculateBatteryLevel(int voltage, int ver) {
    int ver_major = ver / 1000;
    
    if (ver_major < 4) {
        if (voltage <= 7935) return 0;
        if (voltage >= 9870) return 100;
        return (voltage - 7935) / 20;
    } else if (ver_major == 4 || ver_major == 7 || ver == 43000) {
        if (voltage <= 9918) return 0;
        if (voltage >= 12337) return 100;
        return (voltage - 9918) / 24;
    } else if (ver_major == 5 || ver_major == 6 || ver == 42000 || ver == 44000) {
        if (voltage <= 11902) return 0;
        if (voltage >= 14805) return 100;
        return (voltage - 11902) / 29;
    } else if (ver_major == 8) {
        if (voltage <= 13886) return 0;
        if (voltage >= 17272) return 100;
        return (voltage - 13886) / 34;
    }
    
    return 1;
}

} // namespace MonoWheel