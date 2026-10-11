#include "power_mgmt.h"

#include <Arduino.h>

void PowerMgmt::begin() {
  // Charge current is set by the PROG resistor on the PCB, not by a GPIO.
}

void PowerMgmt::idle() {
  // Placeholder so a flashed framework does not spin at 64 MHz.
  // The real wait is a SoftDevice-aware system-on sleep.
  delay(50);
}
