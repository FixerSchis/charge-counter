#pragma once

#include <stdint.h>

// Eight addressable RGB lamps. Encoding is in docs/behavior.md.
// show() does not send a frame yet. begin() and clear() leave the chain unpowered.
class LedDisplay {
 public:
  void begin();
  void show(uint16_t count);
  void clear();

  bool isOn() const { return on_; }
  uint16_t shown() const { return shown_; }

 private:
  bool on_ = false;
  uint16_t shown_ = 0;
};
