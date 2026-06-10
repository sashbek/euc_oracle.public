#define LOG_ENABLE

#include <Arduino.h>
#include <Preferences.h>
#include "settings_manager.h"
#include "settings_registers.h"
#include "uart_parsers/veteran/veteran_uart_parser.h"
#include "uart_parsers/mock/mock_uart_parser.h"

#include "log_defines.h"
#include "ble_manager.h"
#include "settings_server_handler.h"
#include "third_eye_client_handler/third_eye_client_handler.h"
#include "extension_client_handler/extension_client_handler.h"


// ============================================================================
// Конфигурация
// ============================================================================
constexpr unsigned long CONFIG_TIMEOUT_MS = 5000;
constexpr int8_t RX_DATA_GPIO = 20; 
constexpr unsigned long SCAN_TIMEOUT_MS = 10000;

// ============================================================================
// Глобальные переменные
// ============================================================================

MonoWheel::BleManager& ble = MonoWheel::BleManager::getInstance();
MonoWheel::SettingsServerHandler settingsServerHandler;

// Глобальный объект
MonoWheel::ExtensionClientHandler& extensions = MonoWheel::ExtensionClientHandler::getInstance();

MonoWheel::UartParser* uartParser = nullptr;
MonoWheel::WheelData currentWheelData;

SettingsManager& settings = SettingsManager::getInstance();

MonoWheel::ThirdEyeClientHandler& thirdEye = MonoWheel::ThirdEyeClientHandler::getInstance();

unsigned long last_scan_begin_ms = 0;

// ============================================================================
// Обработчики BLE
// ============================================================================

void onBleWrite(const uint8_t* data, size_t len) {
    LOG_F("[APP] BLE write processed: %d bytes\n", len);
}

void onBleRead(uint8_t* buffer, size_t* len) {
    if (settings.isDataReady()) {
        *len = settings.handleReadData(buffer, 22);
        LOG_F("[APP] BLE read: returning %d bytes\n", *len);
    } else {
        *len = 1;
        buffer[0] = (uint8_t)settings.getStatus();
        LOG_F("[APP] BLE read: returning status 0x%02X\n", buffer[0]);
    }
}

void onBleConnect() {
    LOG_LN("[APP] BLE client connected, extending config timeout");
}

void onBleDisconnect(int reason) {
    LOG_F("[APP] BLE client disconnected (reason: 0x%02X)\n", reason);
}

// ============================================================================
// Колбэк состояния ThirdEye
// ============================================================================

void onThirdEyeStateChanged(MonoWheelRegs::ThirdEyeConnState state) {
    const char* state_names[] = {"DISCONNECTED", "SCANNING", "CONNECTING", "CONNECTED", "ERROR"};
    int idx = (state == MonoWheelRegs::ThirdEyeConnState::ERROR) ? 4 : (int)state;
    LOG_F("[APP] ThirdEye state changed: %s\n", state_names[idx]);
}

// ============================================================================
// Режим настройки
// ============================================================================

bool waitForConfig(unsigned long timeout_ms) {
    LOG_LN("[CONFIG] Starting configuration mode...");
    
    char adv_name[20];
    settings.getString(MonoWheelRegs::SettingIndex::DEVICE_NAME, adv_name, 20, "ORACLE-W-1");
    
    // Регистрируем серверный handler
    ble.registerServerHandler(&settingsServerHandler);
    
    // Колбэки
    settingsServerHandler.onClientConnected([]() { LOG_LN("[APP] Config client connected"); });
    settingsServerHandler.onClientDisconnected([](int r) { LOG_F("[APP] Config client disconnected: %d\n", r); });
    settingsServerHandler.onDataWritten(onBleWrite);
    settingsServerHandler.onDataRead(onBleRead);
    
    ble.beginServer(adv_name);
    
    unsigned long start = millis();
    bool configured = false;
    
    while (millis() - start < timeout_ms) {
        if (settingsServerHandler.isClientConnected()) {
            configured = true;
            start = millis();
        }
        delay(100);
    }
    
    ble.stopServer();
    
    return configured;
}

// ============================================================================
// Колбэк при получении новых данных от парсера
// ============================================================================

void onWheelDataUpdated(const MonoWheel::WheelData& data) {
    currentWheelData = data;
    
    // Отправляем в ThirdEye
    if (thirdEye.isEnabled() && thirdEye.isConnected()) {
        MonoWheel::ThirdEyeData teData;
        teData.speed = data.speed;
        teData.pwm = data.pwm;
        teData.voltage = data.voltage;
        teData.current = data.current;
        teData.power = data.power;
        teData.battery = data.battery_level;
        teData.temp = data.temperature;
        teData.distance = data.distance;
        teData.ride_time = data.ride_time;
        
        thirdEye.update(teData);
    }

    extensions.broadcast(data);
}

// ============================================================================
// Инициализация парсера UART
// ============================================================================

bool initUartParser() {
    uint8_t parser_type = settings.getUint8(MonoWheelRegs::SettingIndex::UART_PROTOCOL);
    
    LOG_F("[MAIN] UART parser type: %d\n", parser_type);
    
    // Выбираем парсер согласно настройкам
    if (parser_type == 0) {  // MOCK
        uartParser = new MonoWheel::MockUartParser();
    } else if (parser_type == 1) {  // VETERAN
        uartParser = new MonoWheel::VeteranUartParser(Serial1);
    } else {
        LOG_F("[MAIN] Unknown parser type %d, using MOCK\n", parser_type);
        uartParser = new MonoWheel::MockUartParser();
    }
    
    if (uartParser == nullptr) {
        LOG_LN("[MAIN] Failed to create UART parser");
        return false;
    }
    
    if (!uartParser->begin(RX_DATA_GPIO)) {
        LOG_F("[MAIN] Failed to initialize %s parser\n", uartParser->getName());
        return false;
    }

    uartParser->onDataUpdated(onWheelDataUpdated);
    
    LOG_F("[MAIN] %s parser initialized successfully\n", uartParser->getName());
    
    return true;
}

// ============================================================================
// Инициализация подсистем
// ============================================================================

void logThirdEyeSettings() {
    LOG_LN("\n┌──────────────────────────────────────────────┐");
    LOG_LN("│              THIRD EYE SETTINGS              │");
    LOG_LN("└──────────────────────────────────────────────┘");
    LOG_F("  Enabled: %s\n", 
                  settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_ENABLED) ? "YES" : "NO");
    LOG_F("  Top column: %d\n", 
                  settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_TOP_COLUMN));
    LOG_F("  Bottom column: %d\n", 
                  settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_BOTTOM_COLUMN));
    LOG_F("  Show names: %s\n", 
                  settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_SHOW_NAMES) ? "YES" : "NO");
    LOG_F("  Brightness: %d\n", 
                  settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_BRIGHTNESS));
    LOG_F("  Auto connect: %s\n", 
                  settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_AUTO_CONNECT) ? "YES" : "NO");
    
    char target_name[17] = {0};
    settings.getString(MonoWheelRegs::SettingIndex::THIRD_EYE_TARGET_NAME, target_name, sizeof(target_name));
    LOG_F("  Target name: %s\n", target_name);
    LOG_LN("──────────────────────────────────────────────");
}

bool initThirdEye() {
    if (!settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_ENABLED)) {
        LOG_LN("[THIRD_EYE] Disabled, skipping");
        return false;
    }
    
    thirdEye.onStateChanged(onThirdEyeStateChanged);
    
    char target_name_[17] = {0};
    settings.getString(MonoWheelRegs::SettingIndex::THIRD_EYE_TARGET_NAME, target_name_, 17);

    if (!thirdEye.begin(
        settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_ENABLED),
        (MonoWheel::ThirdEyeColumn)settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_TOP_COLUMN),
        (MonoWheel::ThirdEyeColumn)settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_BOTTOM_COLUMN),
        settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_SHOW_NAMES),
        settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_BRIGHTNESS),
        settings.getBool(MonoWheelRegs::SettingIndex::THIRD_EYE_AUTO_CONNECT),
        settings.getUint8(MonoWheelRegs::SettingIndex::THIRD_EYE_RECONN_INTERVAL),
        target_name_,
        17
    )) {
        LOG_LN("[THIRD_EYE] Failed to initialize");
        return false;
    }
    
    // Регистрируем ThirdEye как обработчик со стороны клиента
    ble.registerClientHandler(&thirdEye);
    
    LOG_LN("[THIRD_EYE] Initialized successfully");
    return true;
}

bool initExtentions() {
    if (!settings.getBool(MonoWheelRegs::SettingIndex::EXT_PROXY_ENABLE)) {
        LOG_LN("[EXT] Disabled");
        return true;
    }

    char buffer[5];
    settings.getString(MonoWheelRegs::SettingIndex::EXT_PROXY_PEER_GP, buffer, 5);
    buffer[4] = '\0';

    uint8_t gp_cap = settings.getUint8(MonoWheelRegs::SettingIndex::EXT_PROXY_GP_CAP);
    if (!extensions.begin(buffer, gp_cap)) {
        LOG_LN("[EXT] Failed to initialize");
        return false;
    }

    // Регистрируем Extentions как обработчик со стороны клиента
    ble.registerClientHandler(&extensions);
    
    LOG_LN("[EXT] Initialized successfully");
    return true;
}

void logConfiguration() {
    LOG_LN("\n");
    LOG_LN("╔══════════════════════════════════════════════╗");
    LOG_LN("║           MONOWHEEL MODULE v1.0              ║");
    LOG_LN("╚══════════════════════════════════════════════╝");
    
    if (!settings.begin(MonoWheelRegs::DeviceType::WHEEL)) {
        LOG_LN("[FATAL] Failed to initialize SettingsManager");
        return;
    }

    settings.incBootCount();
    LOG_F("[BOOT] Boot count: %d\n", settings.getBootCount());

    logThirdEyeSettings();
}

// ============================================================================
// Main
// ============================================================================

void setup() {
    LOG_INIT();

    logConfiguration();

    waitForConfig(CONFIG_TIMEOUT_MS); // Запускаем сервер с конфигуратором для подключения с телефона

    initThirdEye();   // Запускаем обработчик третьего глаза, регистрируем его в ble manager
    initExtentions(); // Запускаем обработчик дополнений, регистрируем его в ble manager
    
    initUartParser(); // Запускаем парсер протокола (по настройкам)
    
    ble.beginScan();   // начинаем сканирование, ищем устройства которые "нравятся" handler-ам
    ble.requestScan();
    last_scan_begin_ms = millis();
    bool status = true;
    while(status) { status = ble.isScanning(); delay(1); }
    ble.connectPendingDevices();

    ble.beginClient(); // подключаем все устройства, которые понтравились handler-ам
}

void loop() {
    ble.loop();
    thirdEye.loop();

    if (uartParser != nullptr) {
        uartParser->loop();
    }

    if (thirdEye.needsScan() && last_scan_begin_ms + SCAN_TIMEOUT_MS < millis()) {
        ble.requestScan();
        last_scan_begin_ms = millis();
    }

    if (ble.getPendingsCount() > 0 && !ble.isScanning()) {
        ble.connectPendingDevices();
    }
    
    delay(1);
}