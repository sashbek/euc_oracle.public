#include "led_manager.h"
#include "settings_manager.h"
#include "settings_registers.h"
#include "log_defines.h"

namespace MonoWheel {

bool LedManager::begin() {
    LOG_LN("[LED] Initializing strips...");
    
    // Централизованная инициализация всех лент
    FastLED.addLeds<WS2812, 0, GRB>(front_left_.getLeds(), front_left_.getCount());
    FastLED.addLeds<WS2812, 1, GRB>(front_right_.getLeds(), front_right_.getCount());
    FastLED.addLeds<WS2812, 3, GRB>(side_left_.getLeds(), side_left_.getCount());
    FastLED.addLeds<WS2812, 4, GRB>(side_right_.getLeds(), side_right_.getCount());
    
    FastLED.setBrightness(128);
    FastLED.clear();
    //FastLED.show();
    
    LOG_LN("[LED] Strips initialized");
    return true;
}

void LedManager::update(const LedMetrics& metrics) {
    if (!enabled_) return;
    
    unsigned long now = millis();
    
    // Обновляем настройки раз в секунду
    if (now - last_settings_load_ > 1000) {
        last_settings_load_ = now;
        auto& s = SettingsManager::getInstance();
        
        // Режим
        int mode = s.getUint8(MonoWheelRegs::SettingIndex::LED_EFFECT_MODE);
        setEffect(mode);
        
        // Скорость
        uint8_t base_speed = s.getUint8(MonoWheelRegs::SettingIndex::LED_BASE_SPEED);
        
        // Speed boost
        if (s.getBool(MonoWheelRegs::SettingIndex::LED_SPECIAL_SPEED_BOOST)) {
            uint16_t thresh1 = s.getUint16(MonoWheelRegs::SettingIndex::LED_SPEED_THRESHOLD_1);
            uint16_t boost1 = s.getUint16(MonoWheelRegs::SettingIndex::LED_SPEED_BOOST_1);
            uint16_t thresh2 = s.getUint16(MonoWheelRegs::SettingIndex::LED_SPEED_THRESHOLD_2);
            uint16_t boost2 = s.getUint16(MonoWheelRegs::SettingIndex::LED_SPEED_BOOST_2);
            
            float speed_kmh = metrics.speed;
            if (speed_kmh >= thresh2 / 10.0f) {
                base_speed = boost2;
            } else if (speed_kmh >= thresh1 / 10.0f) {
                base_speed = boost1;
            }
        }
        
        Effects::g_effectParams.speed = base_speed;
        
        // Цвета
        // ... чтение LED_COLOR_1..5 из регистров
    }
    
    // Применяем эффект ко всем лентам
    front_left_.update(now, metrics, current_effect_);
    front_right_.update(now, metrics, current_effect_);
    side_left_.update(now, metrics, current_effect_);
    side_right_.update(now, metrics, current_effect_);
    
    FastLED.show();
}

void LedManager::setEffect(int mode) {
    static const Effects::EffectFunction effects[] = {
        Effects::staticColor,   // 0
        nullptr,                // 1 (breathing — не реализован)
        Effects::gradient3,     // 2
        Effects::gradient5,     // 3
        Effects::whiteShift,    // 4
        Effects::blink2,        // 5
        Effects::blink3,        // 6
        Effects::blink5,        // 7
        Effects::flash1_1,      // 8
        Effects::flash1_3,      // 9
        Effects::flash1_5,      // 10
        Effects::flash2_1,      // 11
        Effects::flash2_3,      // 12
        Effects::flash2_5,      // 13
        Effects::flash3_1,      // 14
        Effects::flash3_3,      // 15
        Effects::flash3_5,      // 16
        Effects::cloneDrl,      // 17
        Effects::police,        // 18
    };
    
    if (mode >= 0 && mode < 19 && effects[mode]) {
        current_effect_ = effects[mode];
        LOG_F("[LED] Effect: %d\n", mode);
    }
}

void LedManager::setSpeed(uint8_t speed) {
    Effects::g_effectParams.speed = speed;
}

void LedManager::setColors(const uint32_t* colors, int count) {
    for (int i = 0; i < count && i < 5; i++) {
        Effects::g_effectParams.colors[i] = colors[i];
    }
    Effects::g_effectParams.num_colors = count;
}

} // namespace MonoWheel