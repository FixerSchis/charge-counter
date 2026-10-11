#pragma once

#include <stdint.h>

// Eight external LEDs plus the onboard green LED.
// Encoding is specified in docs/behavior.md. show() does not drive the
// pattern yet; begin() and clear() force every LED off.
class LedDisplay {
 public:
  void begin();
  void show(uint16_t count);
  void clear();

  bool isOn() const { return on_; }
  uint16_t shown() const { return shown_; }

 private:
  void writeActivity(bool on);
  void writeCountersOff();

  bool on_ = false;
  uint16_t shown_ = 0;
};
