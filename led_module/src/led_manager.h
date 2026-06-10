#pragma once
#include <Arduino.h>
#include "led_core/strip_controller.h"
#include "led_effects/effects.h"

namespace MonoWheel {

class LedManager {
public:
    static LedManager& getInstance() {
        static LedManager instance;
        return instance;
    }
    
    bool begin();
    void update(const LedMetrics& metrics);
    void setEffect(int mode);
    void setSpeed(uint8_t speed);
    void setColors(const uint32_t* colors, int count);
    
    bool enabled_ = true;
private:
    LedManager() = default;
    
    // 2 вертикальные полосы спереди по 5 светодиодов
    StripController front_left_{0, 5};
    StripController front_right_{1, 5};
    
    // 2 ёлочки по бокам — каждая одна лента из 27 светодиодов
    // Ряды: 3+4+5+7+8 = 27 (только для информации)
    uint8_t tree_rows_[5] = {3, 4, 5, 7, 8};
    StripController side_left_{3, 27, tree_rows_, 5};
    StripController side_right_{4, 27, tree_rows_, 5};

    Effects::EffectFunction current_effect_ = Effects::gradient3;
    
    unsigned long last_settings_load_ = 0;
};

} // namespace MonoWheel