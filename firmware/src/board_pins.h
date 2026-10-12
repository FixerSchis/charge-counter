#pragma once

#include <stdint.h>

// P0 pin numbers on the Ebyte E73-2G4M04S1B. The map is in docs/hardware.md.
// These are Nordic GPIO indexes, not Arduino variant pin numbers.
// UART and VBUS sense are the USB-C programming path through the CH340C.

// Count lamps are a WS2812-style chain. P0.02 is the data pin, held low
// whenever the chain is unpowered. P0.16 high connects the cell to the chain.
// Lamp order and colors are in docs/behavior.md and docs/hardware.md.
static constexpr uint8_t PIN_LED_DATA = 2;  // P0.02
static constexpr uint8_t PIN_LED_EN = 16;   // P0.16, high turns the chain on

static constexpr uint8_t PIN_BUTTON = 13;      // P0.13, active low
static constexpr uint8_t PIN_VBAT_EN = 14;     // P0.14, high enables the divider
static constexpr uint8_t PIN_VBAT_AIN = 28;    // P0.28 / AIN4
static constexpr uint8_t PIN_UART_TX = 12;     // P0.12 to CH340 RXD
static constexpr uint8_t PIN_UART_RX = 15;     // P0.15 from CH340 TXD
static constexpr uint8_t PIN_VBUS_SENSE = 17;  // P0.17, high while USB-C is plugged in
// P0.21 is the hardware reset from CH340 DTR. Firmware does not drive it.
