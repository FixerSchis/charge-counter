#pragma once

#include <stdint.h>

// Direct P0 access so the pin numbers in board_pins.h stay correct on a
// custom board, independent of the Arduino variant map used to compile.

void board_pin_output_low(uint8_t pin);
void board_pin_input_pullup(uint8_t pin);
void board_pin_input_nopull(uint8_t pin);
void board_pin_write(uint8_t pin, bool high);
bool board_pin_read(uint8_t pin);
