#include "power_mgmt.h"

#include <Arduino.h>
#include <nrf_soc.h>

#include "board_gpio.h"
#include "board_pins.h"

namespace {

// Empty ISR: the interrupt itself exits waitForEvent().
void buttonWakeIsr() {}

}  // namespace

void PowerMgmt::begin() {
  // Charge current is set by the PROG resistor on the PCB, not by a GPIO.
  board_pin_output_low(PIN_VBAT_EN);
  board_pin_input_nopull(PIN_VBUS_SENSE);

  // While USB is unplugged, leave the UART pins floating so the unpowered
  // CH340 cannot draw from the cell through its pin diodes.
  if (!board_pin_read(PIN_VBUS_SENSE)) {
    board_pin_input_nopull(PIN_UART_TX);
    board_pin_input_nopull(PIN_UART_RX);
  }

  // GPIOTE IN event wakes System ON idle. Sense alone only wakes System OFF.
  attachInterrupt(PIN_BUTTON, buttonWakeIsr, FALLING);
}

void PowerMgmt::idle() {
  if (!board_pin_read(PIN_VBUS_SENSE)) {
    board_pin_input_nopull(PIN_UART_TX);
    board_pin_input_nopull(PIN_UART_RX);
  }

  sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
  waitForEvent();
}
