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
  board_pin_output_low(PIN_LED_DATA);
  board_pin_output_low(PIN_LED_EN);
  on_ = false;
  shown_ = 0;
}
