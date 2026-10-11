#include "power_mgmt.h"

#include <Arduino.h>

void PowerMgmt::begin() {
  // Leave PIN_CHARGING_CURRENT (Arduino pin 22, P0.13) untouched.
  // High-impedance selects the board's ~50 mA charge current.
  // Driving the pin changes that current. See docs/hardware.md.
}

void PowerMgmt::idle() {
  // Placeholder so a flashed framework does not spin at 64 MHz.
  // The real wait is a SoftDevice-aware system-on sleep.
  delay(50);
}
