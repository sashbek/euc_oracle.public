#include "settings_manager.h"
#include "settings_register_map.h"
#include <cstdio>

// ============ Инициализация ============

bool SettingsManager::begin(MonoWheelRegs::DeviceType device_type, const char* nvs_namespace) {
    if (init_) return true;
    
    device_type_ = device_type;
    
    if (!prefs_.begin(nvs_namespace, false)) {
        status_ = MonoWheelRegs::Status::ERR_FLASH;
        Serial.println("[SETTINGS] Failed to open NVS");
        return false;
    }
    
    Serial.printf("[SETTINGS] NVS opened: %s\n", nvs_namespace);
    
    initRegMap();
    
    if (!loadFromFlash()) {
        Serial.println("[SETTINGS] No saved settings, using defaults");
        resetToDefaults();
    } else {
        Serial.println("[SETTINGS] Settings loaded from NVS");
    }
    
    // Системные значения
    uint32_t devId = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
    writeRamRegister(0x10, (uint8_t*)&devId, 4);
    
    uint32_t fwVer = (1 << 24) | (0 << 16) | (0 << 8) | 0;
    writeRamRegister(0x14, (uint8_t*)&fwVer, 4);
    
    uint16_t hwRev = 0x0100;
    writeRamRegister(0x18, (uint8_t*)&hwRev, 2);
    
    uint8_t devType = (uint8_t)device_type_;
    writeRamRegister(0x06, &devType, 1);
    
    writeRamRegister(0x05, (uint8_t*)&SettingsProtocol::PROTOCOL_VERSION, 1);
    
    // Увеличиваем boot count
    uint32_t bootCount;
    read((MonoWheelRegs::RegAddr)0x1A, bootCount);
    write((MonoWheelRegs::RegAddr)0x1A, bootCount + 1);
    
    init_ = true;
    status_ = MonoWheelRegs::Status::OK;
    
    Serial.printf("[SETTINGS] Initialized, boot count: %d\n", bootCount + 1);
    
    return true;
}

void SettingsManager::initRegMap() {
    Serial.println("[SETTINGS] Initializing register map...");
    
    #define REG(addr, sz, rd, wr, per, def) \
        do { \
            regs_[addr].resize(sz); \
            meta_[addr] = {sz, rd, wr, per, def}; \
            if (def) { \
                memcpy(regs_[addr].data(), def, sz); \
            } else { \
                memset(regs_[addr].data(), 0, sz); \
            } \
        } while(0);
    
    SETTINGS_REGISTER_MAP
    
    #undef REG
    
    // Маппинг SettingIndex -> адрес
    SETTINGS_MAPPING
    
    Serial.printf("[SETTINGS] Total: %d, Persistent: %d\n", regs_.size(), index_map_.size());
}

// ============ Доступ по индексу ============

uint16_t SettingsManager::indexToAddr(MonoWheelRegs::SettingIndex index) const {
    auto it = index_map_.find(index);
    return (it != index_map_.end()) ? it->second : 0xFFFF;
}

size_t SettingsManager::indexToSize(MonoWheelRegs::SettingIndex index) const {
    uint16_t addr = indexToAddr(index);
    if (addr == 0xFFFF) return 0;
    auto it = meta_.find(addr);
    return (it != meta_.end()) ? it->second.size : 0;
}

bool SettingsManager::getBool(MonoWheelRegs::SettingIndex index, bool default_val) const {
    return getUint8(index, default_val ? 1 : 0) != 0;
}

uint8_t SettingsManager::getUint8(MonoWheelRegs::SettingIndex index, uint8_t default_val) const {
    uint16_t addr = indexToAddr(index);
    if (addr == 0xFFFF) return default_val;
    uint8_t val;
    return readRegister(addr, &val, 1) ? val : default_val;
}

uint16_t SettingsManager::getUint16(MonoWheelRegs::SettingIndex index, uint16_t default_val) const {
    uint16_t addr = indexToAddr(index);
    if (addr == 0xFFFF) return default_val;
    uint16_t val;
    return readRegister(addr, (uint8_t*)&val, 2) ? val : default_val;
}

uint32_t SettingsManager::getUint32(MonoWheelRegs::SettingIndex index, uint32_t default_val) const {
    uint16_t addr = indexToAddr(index);
    if (addr == 0xFFFF) return default_val;
    uint32_t val;
    return readRegister(addr, (uint8_t*)&val, 4) ? val : default_val;
}

int8_t SettingsManager::getInt8(MonoWheelRegs::SettingIndex index, int8_t default_val) const {
    return (int8_t)getUint8(index, (uint8_t)default_val);
}

void SettingsManager::getString(MonoWheelRegs::SettingIndex index, char* buf, size_t buf_size, const char* default_val) const {
    uint16_t addr = indexToAddr(index);
    if (addr == 0xFFFF) {
        strncpy(buf, default_val, buf_size - 1);
        buf[buf_size - 1] = '\0';
        return;
    }
    size_t len = indexToSize(index);
    if (!readRegister(addr, (uint8_t*)buf, min(len, buf_size - 1))) {
        strncpy(buf, default_val, buf_size - 1);
        buf[buf_size - 1] = '\0';
        return;
    }
    buf[min(len, buf_size - 1)] = '\0';
}

void SettingsManager::getBytes(MonoWheelRegs::SettingIndex index, uint8_t* buf, size_t size) const {
    uint16_t addr = indexToAddr(index);
    if (addr != 0xFFFF) {
        readRegister(addr, buf, size);
    }
}

void SettingsManager::setBool(MonoWheelRegs::SettingIndex index, bool value) {
    setUint8(index, value ? 1 : 0);
}

void SettingsManager::setUint8(MonoWheelRegs::SettingIndex index, uint8_t value) {
    uint16_t addr = indexToAddr(index);
    if (addr != 0xFFFF) {
        writeRegister(addr, &value, 1);
    }
}

void SettingsManager::setUint16(MonoWheelRegs::SettingIndex index, uint16_t value) {
    uint16_t addr = indexToAddr(index);
    if (addr != 0xFFFF) {
        writeRegister(addr, (uint8_t*)&value, 2);
    }
}

void SettingsManager::setUint32(MonoWheelRegs::SettingIndex index, uint32_t value) {
    uint16_t addr = indexToAddr(index);
    if (addr != 0xFFFF) {
        writeRegister(addr, (uint8_t*)&value, 4);
    }
}

void SettingsManager::setInt8(MonoWheelRegs::SettingIndex index, int8_t value) {
    setUint8(index, (uint8_t)value);
}

void SettingsManager::setString(MonoWheelRegs::SettingIndex index, const char* value) {
    uint16_t addr = indexToAddr(index);
    if (addr != 0xFFFF) {
        size_t max_len = indexToSize(index);
        writeRegister(addr, (const uint8_t*)value, min(strlen(value), max_len));
    }
}

void SettingsManager::setBytes(MonoWheelRegs::SettingIndex index, const uint8_t* data, size_t size) {
    uint16_t addr = indexToAddr(index);
    if (addr != 0xFFFF) {
        writeRegister(addr, data, min(size, indexToSize(index)));
    }
}

// ============ Прямой доступ ============

bool SettingsManager::writeRegister(uint16_t addr, const uint8_t* data, size_t size) {
    if (!init_) return false;

    Serial.println("[SETTINGS] writing reg");
    
    auto meta_it = meta_.find(addr);
    if (meta_it == meta_.end()) {
        status_ = MonoWheelRegs::Status::ERR_ADDR;
        return false;
    }
    
    // Проверка на writable (кроме динамических регистров 0xC0-0xFF)
    bool is_dynamic = (addr >= 0xC0 && addr <= 0xFF);
    if (!is_dynamic && !meta_it->second.writable) {
        status_ = MonoWheelRegs::Status::ERR_PROTECTED;
        return false;
    }
    
    if (size > meta_it->second.size) {
        status_ = MonoWheelRegs::Status::ERR_SIZE;
        return false;
    }
    
    auto reg_it = regs_.find(addr);
    if (reg_it == regs_.end()) return false;
    
    // Проверка изменений
    bool changed = false;
    for (size_t i = 0; i < size; i++) {
        if (reg_it->second[i] != data[i]) {
            changed = true;
            break;
        }
    }
    
    bool is_cmd = (addr == 0x03);
    
    if (!changed && !is_cmd) {
        status_ = MonoWheelRegs::Status::OK;
        Serial.println("[SETTINGS] !c and !cmd");
        return true;
    }
    
    memcpy(reg_it->second.data(), data, size);
    
    // Сохраняем в NVS только персистентные
    if (meta_it->second.persistent) {
        Serial.println("[SETTINGS] 1");
        char key[8];
        snprintf(key, sizeof(key), "r%04X", addr);
        prefs_.putBytes(key, data, size);
    }
    
    notifyChange(addr, data, size);
        Serial.println("[SETTINGS] 2");
    
    if (addr == 0x03 && size == 1) {
        processCommand((MonoWheelRegs::Command)data[0]);
        Serial.println("[SETTINGS] 3");
    }
    
    status_ = MonoWheelRegs::Status::OK;
        Serial.println("[SETTINGS] 4");
    return true;
}

bool SettingsManager::readRegister(uint16_t addr, uint8_t* data, size_t size) const {
    if (!init_) return false;
    auto it = regs_.find(addr);
    if (it == regs_.end()) return false;
    size_t copy = min(size, it->second.size());
    memcpy(data, it->second.data(), copy);
    return true;
}

bool SettingsManager::writeRamRegister(uint16_t addr, const uint8_t* data, size_t size) {
    auto it = regs_.find(addr);
    if (it == regs_.end()) return false;
    size_t copy = min(size, it->second.size());
    memcpy(it->second.data(), data, copy);
    notifyChange(addr, data, copy);
    return true;
}

// ============ Информация ============

uint32_t SettingsManager::getBootCount() const {
    uint32_t val = 0;
    readRegister(0x1A, (uint8_t*)&val, 4);
    return val;
}

uint32_t SettingsManager::getUptime() const {
    return millis() / 1000;
}

// ============ Boot ==================

void SettingsManager::incBootCount() {
    uint32_t val = 0;
    readRegister(0x1A, (uint8_t*)&val, 4);
    val += 1;
    writeRegister(0x1A, (uint8_t*)&val, 4);
}

// ============ Вспомогательные методы ============

void SettingsManager::notifyChange(uint16_t addr, const uint8_t* data, size_t size) {
    if (change_cb_) {
        change_cb_(addr, data, size);
    }
}

// ============ Обработка команд ============

void SettingsManager::processCommand(MonoWheelRegs::Command cmd) {
    Serial.printf("[SETTINGS] Processing command: 0x%02X\n", (uint8_t)cmd);
    
    switch (cmd) {
        case MonoWheelRegs::Command::START_READ:
            startRead();
            break;
        case MonoWheelRegs::Command::SAVE_FLASH:
            saveToFlash();
            break;
        case MonoWheelRegs::Command::LOAD_FLASH:
            loadFromFlash();
            break;
        case MonoWheelRegs::Command::RESET_DEFAULTS:
            resetToDefaults();
            break;
        case MonoWheelRegs::Command::SOFT_RESET:
            Serial.println("[SETTINGS] Soft reset requested");
            delay(100);
            ESP.restart();
            break;
        default:
            break;
    }
}

void SettingsManager::startRead() {
    uint8_t addrL = 0, addrH = 0, size = 0;
    
    readRegister(0x00, &addrL, 1);
    readRegister(0x01, &addrH, 1);
    readRegister(0x02, &size, 1);
    
    read_addr_ = addrL | (addrH << 8);
    read_size_ = size;
    read_offset_ = 0;
    data_ready_ = true;
    
    Serial.printf("[SETTINGS] startRead: addr=0x%04X, size=%d\n", read_addr_, read_size_);
    
    status_ = MonoWheelRegs::Status::DATA_READY;
}

// ============ Сохранение и загрузка ============

bool SettingsManager::saveToFlash() {
    if (!init_) return false;
    
    size_t saved = 0;
    for (const auto& [addr, meta] : meta_) {
        if (meta.persistent) {
            auto it = regs_.find(addr);
            if (it != regs_.end()) {
                char key[8];
                snprintf(key, sizeof(key), "r%04X", addr);
                size_t written = prefs_.putBytes(key, it->second.data(), meta.size);
                if (written == meta.size) {
                    saved++;
                }
            }
        }
    }
    
    Serial.printf("[SETTINGS] Saved %d registers to NVS\n", saved);
    return saved > 0;
}

bool SettingsManager::loadFromFlash() {
    //sif (!init_) return false;
    
    size_t loaded = 0;
    for (const auto& [addr, meta] : meta_) {
        if (meta.persistent) {
            auto it = regs_.find(addr);
            if (it != regs_.end()) {
                char key[8];
                snprintf(key, sizeof(key), "r%04X", addr);
                size_t len = prefs_.getBytes(key, it->second.data(), meta.size);
                if (len == meta.size) {
                    loaded++;
                }
            }
        }
    }
    
    Serial.printf("[SETTINGS] Loaded %d registers from NVS\n", loaded);
    return loaded > 0;
}

bool SettingsManager::resetToDefaults() {
    if (!init_) return false;
    
    size_t reset = 0;
    for (const auto& [addr, meta] : meta_) {
        if (meta.def_val) {
            auto it = regs_.find(addr);
            if (it != regs_.end()) {
                memcpy(it->second.data(), meta.def_val, meta.size);
                reset++;
            }
        }
    }
    
    Serial.printf("[SETTINGS] Reset %d registers to defaults\n", reset);
    return true;
}

// ============ Обработка BLE данных ============

void SettingsManager::handleWriteData(const uint8_t* data, size_t len) {
    if (!init_) return;
    
    write_buf_.insert(write_buf_.end(), data, data + len);
    
    // Ищем маркер начала
    int startIdx = SettingsHelpers::check_marker(write_buf_.data(), SettingsProtocol::WRITE_START_MARKER) ? 0 : -1;
    
    if (startIdx == -1) {
        startIdx = SettingsHelpers::find_marker(write_buf_.data(), write_buf_.size(), 
                                                 SettingsProtocol::WRITE_START_MARKER);
        if (startIdx > 0) {
            write_buf_.erase(write_buf_.begin(), write_buf_.begin() + startIdx);
            startIdx = 0;
        } else if (startIdx == -1) {
            write_buf_.clear();
            return;
        }
    }
    
    // Ищем маркер конца
    int endIdx = SettingsHelpers::find_marker(write_buf_.data() + 4, write_buf_.size() - 4,
                                               SettingsProtocol::WRITE_END_MARKER);
    
    if (endIdx >= 0) {
        endIdx += 4;
        processWritePacket(write_buf_.data() + 4, endIdx - 4);
        write_buf_.erase(write_buf_.begin(), write_buf_.begin() + endIdx + 4);
    }
    
    if (write_buf_.size() > 512) {
        write_buf_.clear();
        status_ = MonoWheelRegs::Status::ERR_SIZE;
    }
}

void SettingsManager::processWritePacket(const uint8_t* data, size_t len) {
    size_t pos = 0;
    
    while (pos + 4 <= len) {
        // Проверяем на маркер конца
        if (SettingsHelpers::check_marker(data + pos, SettingsProtocol::WRITE_END_MARKER)) {
            break;
        }
        
        uint16_t addr = data[pos] | (data[pos + 1] << 8);
        uint16_t size = data[pos + 2] | (data[pos + 3] << 8);
        pos += 4;
        
        if (pos + size > len) {
            status_ = MonoWheelRegs::Status::ERR_SIZE;
            break;
        }
        
        writeRegister(addr, data + pos, size);
        pos += size;
    }
}

size_t SettingsManager::handleReadData(uint8_t* data, size_t max_len) {
    if (!data_ready_ || read_size_ == 0) {
        return 0;
    }
    
    size_t remaining = read_size_ - read_offset_;
    size_t chunkSize = min(max_len - 2, min(remaining, SettingsProtocol::MAX_CHUNK_SIZE));
    
    if (chunkSize == 0) {
        data_ready_ = false;
        status_ = MonoWheelRegs::Status::OK;
        return 0;
    }
    
    // Offset
    data[0] = read_offset_ & 0xFF;
    data[1] = (read_offset_ >> 8) & 0xFF;
    
    // Читаем данные последовательно, учитывая размеры регистров
    size_t bytes_read = 0;
    uint16_t current_addr = read_addr_ + read_offset_;
    
    while (bytes_read < chunkSize && current_addr <= 0xFFFF) {
        auto it = regs_.find(current_addr);
        if (it != regs_.end() && !it->second.empty()) {
            // Определяем, сколько байт можно прочитать из этого регистра
            size_t reg_size = it->second.size();
            size_t offset_in_reg = 0;  // Пока всегда 0, т.к. мы не поддерживаем смещение внутри регистра
            
            // Для упрощения: если запрашивают адрес начала регистра - возвращаем весь регистр
            // Если запрашивают адрес внутри регистра - нужна более сложная логика
            
            // Копируем данные из регистра
            size_t copy_size = min(chunkSize - bytes_read, reg_size);
            memcpy(data + 2 + bytes_read, it->second.data(), copy_size);
            bytes_read += copy_size;
            current_addr += copy_size;
        } else {
            // Регистр не найден - возвращаем 0
            data[2 + bytes_read] = 0;
            bytes_read++;
            current_addr++;
        }
    }
    
    read_offset_ += bytes_read;
    
    if (read_offset_ >= read_size_) {
        data_ready_ = false;
        status_ = MonoWheelRegs::Status::OK;
        Serial.println("[SETTINGS] Stream complete");
    }
    
    Serial.printf("[SETTINGS] handleReadData: returned %d bytes\n", bytes_read);
    
    return bytes_read + 2;
}