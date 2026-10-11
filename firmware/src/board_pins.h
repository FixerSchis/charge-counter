#pragma once

#include <stdint.h>

// P0 pin numbers on the Ebyte E73-2G4M04S1B. The map is in docs/hardware.md.
// These are Nordic GPIO indexes, not Arduino variant pin numbers.
// UART and VBUS sense are the USB-C programming path through the CH340C.

static constexpr uint8_t PIN_LED_BIT[8] = {
    2,   // P0.02  value 1
    3,   // P0.03  value 2
    4,   // P0.04  value 4
    5,   // P0.05  value 8
    6,   // P0.06  value 16
    7,   // P0.07  value 32
    8,   // P0.08  value 64
    11,  // P0.11  value 128
};

static constexpr uint8_t PIN_BUTTON = 13;      // P0.13, active low
static constexpr uint8_t PIN_VBAT_EN = 14;     // P0.14, high enables the divider
static constexpr uint8_t PIN_VBAT_AIN = 28;    // P0.28 / AIN4
static constexpr uint8_t PIN_UART_TX = 12;     // P0.12 to CH340 RXD
static constexpr uint8_t PIN_UART_RX = 15;     // P0.15 from CH340 TXD
static constexpr uint8_t PIN_VBUS_SENSE = 17;  // P0.17, high while USB-C is plugged in
// P0.21 is the hardware reset from CH340 DTR. Firmware does not drive it.
