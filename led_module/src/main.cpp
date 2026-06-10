#include <Arduino.h>
#include <Preferences.h>
#include "log_defines.h"
#include "settings_manager.h"
#include "settings_registers.h"
#include "settings_register_map.h"
#include "ble_manager.h"
#include "settings_server_handler.h"
#include "extension_server_handler.h"
#include "led_manager.h"

// ============================================================================
// Конфигурация
// ============================================================================
constexpr unsigned long CONFIG_TIMEOUT_MS = 5000;

// ============================================================================
// Глобальные переменные
// ============================================================================
MonoWheel::BleManager& ble = MonoWheel::BleManager::getInstance();
MonoWheel::ExtensionServerHandler& extServer = MonoWheel::ExtensionServerHandler::getInstance();
MonoWheel::LedManager& ledManager = MonoWheel::LedManager::getInstance();
MonoWheel::SettingsServerHandler settingsServerHandler;
SettingsManager& settings = SettingsManager::getInstance();

MonoWheel::LedMetrics currentMetrics;

// ============================================================================
// Обработчики данных расширений
// ============================================================================

void onDataReceived(const MonoWheel::ExtensionPacket& packet) {
    currentMetrics.speed = packet.speed / 100.0f;
    currentMetrics.battery = packet.battery;
    currentMetrics.pwm = packet.pwm;
    currentMetrics.temperature = packet.temperature / 10.0f;
    currentMetrics.voltage = packet.voltage / 100.0f;
    currentMetrics.is_charging = packet.flags & MonoWheel::EXT_FLAG_CHARGING;
    currentMetrics.is_connected = packet.flags & MonoWheel::EXT_FLAG_CONNECTED;
    
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void onConnectionChanged(bool connected) {
    if (connected) {
        LOG_LN("[APP] Wheel connected");
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        LOG_LN("[APP] Wheel disconnected");
        digitalWrite(LED_BUILTIN, LOW);
    }
}

// ============================================================================
// Обработчики BLE
// ============================================================================

void onBleWrite(const uint8_t* data, size_t len) {
    LOG_F("[APP] BLE write: %d bytes\n", len);
}

void onBleRead(uint8_t* buffer, size_t* len) {
    if (settings.isDataReady()) {
        *len = settings.handleReadData(buffer, 22);
    } else {
        *len = 1;
        buffer[0] = (uint8_t)settings.getStatus();
    }
}

// ============================================================================
// Режим настройки
// ============================================================================

bool waitForConfig(unsigned long timeout_ms) {
    LOG_LN("\n[CONFIG] Starting configuration mode...");
    
    char adv_name[20];
    settings.getString(MonoWheelRegs::SettingIndex::DEVICE_NAME, adv_name, 20, "ORACLE-L-1");
    
    ble.registerServerHandler(&settingsServerHandler);
    
    settingsServerHandler.onClientConnected([]() { LOG_LN("[CONFIG] Client connected"); });
    settingsServerHandler.onClientDisconnected([](int r) { LOG_F("[CONFIG] Client disconnected: %d\n", r); });
    settingsServerHandler.onDataWritten(onBleWrite);
    settingsServerHandler.onDataRead(onBleRead);
    
    ble.beginServer(adv_name);
    
    LOG_F("[CONFIG] Advertising as: %s\n", adv_name);
    LOG_LN("[CONFIG] Waiting for connection...");
    
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
    
    if (configured) {
        settings.saveToFlash();
        LOG_LN("[CONFIG] Settings saved");
    } else {
        LOG_LN("[CONFIG] No configuration received");
    }
    
    return configured;
}

// ============================================================================
// Основной режим
// ============================================================================

void startOperationalMode() {
    LOG_LN("\n[MAIN] Starting operational mode...");
    
    char ext_name[8];
    settings.getString(MonoWheelRegs::SettingIndex::EXT_PROXY_PEER_GP, ext_name, sizeof(ext_name), "EXT1");
    
    char full_name[32];
    snprintf(full_name, sizeof(full_name), "[%s]LED", ext_name);
    
    extServer.setDeviceName(full_name);
    extServer.onDataReceived(onDataReceived);
    extServer.onConnectionChanged(onConnectionChanged);
    
    ble.registerServerHandler(&extServer);
    
    if (!ble.beginServer(full_name)) {
        LOG_LN("[FATAL] Failed to start operational server");
        return;
    }
    
    LOG_F("[MAIN] Operational server: %s\n", full_name);
}

// ============================================================================
// Main
// ============================================================================

void setup() {
    LOG_INIT();
    delay(1000);
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    LOG_LN("\n╔══════════════════════════════════════════════╗");
    LOG_LN("║           LED MODULE v1.0                    ║");
    LOG_LN("╚══════════════════════════════════════════════╝");
    
    if (!settings.begin(MonoWheelRegs::DeviceType::LED)) {
        LOG_LN("[FATAL] Failed to initialize SettingsManager");
        return;
    }
    
    settings.incBootCount();
    LOG_F("[BOOT] Boot count: %d\n", settings.getBootCount());

    // Инициализация LED
    if (!ledManager.begin()) {
        LOG_LN("[FATAL] Failed to initialize LED manager");
        return;
    }
    
    // Режим настройки
    waitForConfig(CONFIG_TIMEOUT_MS);
    
    delay(500);
    
    // Перезапуск в основном режиме
    startOperationalMode();
    
    LOG_LN("\n╔══════════════════════════════════════════════╗");
    LOG_LN("║              READY                           ║");
    LOG_LN("╚══════════════════════════════════════════════╝\n");
}

void loop() {
    ble.loop();
    
    // Обновление LED эффектов
    ledManager.update(currentMetrics);
    
    static unsigned long last_status = 0;
    if (millis() - last_status > 10000) {
        last_status = millis();
        LOG_F("[STATUS] Connected: %s, Packets: %lu\n",
              extServer.isConnected() ? "YES" : "NO",
              extServer.getPacketCount());
    }
    
    delay(1);
}