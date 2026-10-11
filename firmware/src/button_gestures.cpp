#include "button_gestures.h"

#include "board_gpio.h"
#include "board_pins.h"

void ButtonGestures::begin() {
  board_pin_input_pullup(PIN_BUTTON);
  busy_ = false;
}

GestureEvent ButtonGestures::poll(uint32_t nowMs) {
  (void)nowMs;
  return GestureEvent::None;
}
