#include "strip_controller.h"

namespace MonoWheel {

StripController::StripController(int pin, int count, const uint8_t* rows, int num_rows)
    : pin_(pin), count_(count), rows_(rows), num_rows_(num_rows) {
    leds_ = new CRGB[count];
    config_.count = count;
    config_.rows = rows;
    config_.num_rows = num_rows;
}

void StripController::begin() {
    // FastLED с динамическим пином — используем define для каждого пина
    // Все ленты инициализируются в LedManager централизованно
}

void StripController::update(unsigned long time_ms, const LedMetrics& metrics, Effects::EffectFunction effect) {
    if (!effect || !leds_) return;
    
    for (int i = 0; i < count_; i++) {
        uint32_t color = effect(i, time_ms, metrics, config_);
        leds_[i] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }
}

} // namespace MonoWheel