#include "board_gpio.h"

#include <nrf_gpio.h>

void board_pin_output_low(uint8_t pin) {
  nrf_gpio_pin_clear(pin);
  nrf_gpio_cfg_output(pin);
}

void board_pin_input_pullup(uint8_t pin) {
  nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_PULLUP);
}

void board_pin_write(uint8_t pin, bool high) {
  if (high) {
    nrf_gpio_pin_set(pin);
  } else {
    nrf_gpio_pin_clear(pin);
  }
}

bool board_pin_read(uint8_t pin) {
  return nrf_gpio_pin_read(pin) != 0;
}
