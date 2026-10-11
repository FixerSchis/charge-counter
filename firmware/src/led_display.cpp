#include "led_display.h"

#include "board_gpio.h"
#include "board_pins.h"

void LedDisplay::begin() {
  clear();
}

void LedDisplay::show(uint16_t count) {
  shown_ = count;
  on_ = true;
}

void LedDisplay::clear() {
  for (uint8_t pin : PIN_LED_BIT) {
    board_pin_output_low(pin);
  }
  on_ = false;
  shown_ = 0;
}
