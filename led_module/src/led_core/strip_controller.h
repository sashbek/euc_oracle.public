#pragma once
#include <FastLED.h>
#include "led_types.h"
#include "led_effect.h"

namespace MonoWheel {

class StripController {
public:
    StripController(int pin, int count, const uint8_t* rows = nullptr, int num_rows = 0);
    
    void begin();
    void update(unsigned long time_ms, const LedMetrics& metrics, Effects::EffectFunction effect);
    void show();
    
    int getCount() const { return count_; }
    CRGB* getLeds() { return leds_; }
    
private:
    int pin_;
    int count_;
    const uint8_t* rows_;
    int num_rows_;
    CRGB* leds_;
    StripConfig config_;
};

} // namespace MonoWheel