#include "led_display.h"

#include <Arduino.h>

namespace {

const uint8_t kCounterPins[8] = {D0, D1, D2, D3, D4, D5, D6, D7};

// LED_STATE_ON comes from the XIAO variant header (1 on this core).
// Off is the other level, so a future core change does not hard-code polarity.
constexpr uint8_t kActivityOff = LED_STATE_ON ? LOW : HIGH;
constexpr uint8_t kActivityOn = LED_STATE_ON ? HIGH : LOW;

}  // namespace

void LedDisplay::writeActivity(bool on) {
  digitalWrite(LED_GREEN, on ? kActivityOn : kActivityOff);
}

void LedDisplay::writeCountersOff() {
  for (uint8_t pin : kCounterPins) {
    digitalWrite(pin, LOW);
  }
}

void LedDisplay::begin() {
  for (uint8_t pin : kCounterPins) {
    pinMode(pin, OUTPUT);
  }
  pinMode(LED_GREEN, OUTPUT);
  clear();
}

void LedDisplay::show(uint16_t count) {
  shown_ = count;
  on_ = true;
}

void LedDisplay::clear() {
  writeCountersOff();
  writeActivity(false);
  on_ = false;
  shown_ = 0;
}
