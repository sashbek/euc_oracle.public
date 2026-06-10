#include <Arduino.h>
#include "ble_manager.h"
#include "extension_server_handler.h"

#define DEVICE_NAME "[EXT1]TestDevice"

MonoWheel::BleManager& ble = MonoWheel::BleManager::getInstance();
MonoWheel::ExtensionServerHandler& extServer = MonoWheel::ExtensionServerHandler::getInstance();

void onDataReceived(const MonoWheel::ExtensionPacket& packet) {
    uint32_t t = packet.ride_time;
    int hours = t / 3600;
    int mins = (t % 3600) / 60;
    int secs = t % 60;
    
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║           EXTENSION DEVICE DATA             ║");
    Serial.println("╚══════════════════════════════════════════════╝");
    
    Serial.printf("  Speed:       %.1f km/h\n", packet.speed / 100.0f);
    Serial.printf("  Battery:     %d%%\n", packet.battery);
    Serial.printf("  PWM:         %.1f%%\n", packet.pwm / 10.0f);
    Serial.printf("  Temperature: %.1f°C\n", packet.temperature / 10.0f);
    Serial.printf("  Voltage:     %.2f V\n", packet.voltage / 100.0f);
    Serial.printf("  Current:     %.2f A\n", packet.current / 100.0f);
    Serial.printf("  Power:       %.1f W\n", packet.power / 10.0f);
    Serial.printf("  Distance:    %.1f km\n", packet.distance / 10.0f);
    Serial.printf("  Ride time:   %02d:%02d:%02d\n", hours, mins, secs);
    
    Serial.print("  Flags:       ");
    if (packet.flags & MonoWheel::ExtensionFlags::EXT_FLAG_CHARGING) Serial.print("CHARGING ");
    if (packet.flags & MonoWheel::ExtensionFlags::EXT_FLAG_CONNECTED) Serial.print("CONNECTED ");
    if (packet.flags & MonoWheel::ExtensionFlags::EXT_FLAG_ALERT) Serial.print("ALERT ");
    if (packet.flags == 0) Serial.print("NONE");
    Serial.println();
    
    Serial.println("═══════════════════════════════════════════════");
    
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void onConnectionChanged(bool connected) {
    if (connected) {
        Serial.println("\n┌──────────────────────────────────────────────┐");
        Serial.println("│           WHEEL CONNECTED                    │");
        Serial.println("└──────────────────────────────────────────────┘");
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        Serial.println("\n┌──────────────────────────────────────────────┐");
        Serial.println("│           WHEEL DISCONNECTED                 │");
        Serial.println("└──────────────────────────────────────────────┘");
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════╗");
    Serial.println("║       EXTENSION DEVICE TEST v1.0             ║");
    Serial.println("╚══════════════════════════════════════════════╝");
    
    // Настраиваем обработчик
    extServer.setDeviceName(DEVICE_NAME);
    extServer.onDataReceived(onDataReceived);
    extServer.onConnectionChanged(onConnectionChanged);
    
    // Регистрируем в BleManager
    ble.registerServerHandler(&extServer);
    
    // Запускаем сервер
    if (!ble.beginServer(DEVICE_NAME)) {
        Serial.println("[FATAL] Failed to start server");
        return;
    }
    
    Serial.printf("  Device name: %s\n", DEVICE_NAME);
    Serial.println("  Waiting for wheel connection...\n");
}

void loop() {
    ble.loop();
    
    // Альтернативный способ получения данных (опрос)
    if (extServer.isDataUpdated()) {
        const auto& data = extServer.getData();
        // Данные доступны через getData()
    }
    
    static unsigned long last_status = 0;
    if (millis() - last_status > 10000) {
        last_status = millis();
        Serial.printf("[STATUS] Connected: %s, Packets: %lu\n",
                      extServer.isConnected() ? "YES" : "NO",
                      extServer.getPacketCount());
    }
    
    delay(10);
}