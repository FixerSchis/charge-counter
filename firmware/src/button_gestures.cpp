#include "button_gestures.h"

#include <Arduino.h>

void ButtonGestures::begin() {
  pinMode(D8, INPUT_PULLUP);
  busy_ = false;
}

GestureEvent ButtonGestures::poll(uint32_t nowMs) {
  (void)nowMs;
  return GestureEvent::None;
}
