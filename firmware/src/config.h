#pragma once

#include <stdint.h>

// Timing values are starting points. The gesture and power work can tune
// them once a unit is measured. See docs/behavior.md and docs/power.md.

static constexpr uint32_t DEBOUNCE_MS = 25;
static constexpr uint32_t PRESS_GAP_MS = 400;
static constexpr uint32_t LONG_PRESS_MS = 600;
static constexpr uint32_t ADD_REPEAT_MS = 400;
static constexpr uint32_t DISPLAY_MS = 2500;
static constexpr uint32_t ACK_FLASH_MS = 150;

// Once a hold has the count at max, this much more holding opens pairing.
// The open window is how long a new phone may bond. A hold of the same
// length while the pairing lamps are flashing forgets every bond.
static constexpr uint32_t PAIR_AT_MAX_MS = 3000;
static constexpr uint32_t PAIR_OPEN_MS = 60000;
static constexpr uint32_t PAIR_FORGET_MS = 3000;

// Locate chase: one lamp at a time, in display order, repeated, in blue.
static constexpr uint32_t LOCATE_STEP_MS = 150;
static constexpr uint8_t LOCATE_CYCLES = 4;

// Two passes of the eight lamps: red, then green. All green is 16.
static constexpr uint16_t COUNT_HARD_CAP = 16;

// Pixel channel value, out of 255. Full scale would pull the cell hard.
static constexpr uint8_t LED_LEVEL = 20;
static constexpr uint16_t DEFAULT_MAX = 2;
static constexpr uint8_t NAME_MAX_BYTES = 32;
static constexpr uint16_t DEFAULT_COUNT = 0;

// Connectable advertising interval. Bluefruit wants units of 0.625 ms;
// 1000 ms is 1600 of those units.
static constexpr uint16_t BLE_ADV_INTERVAL_MS = 1000;

static constexpr char FW_VERSION[] = "0.1.0";
static constexpr char MODEL_NAME[] = "Charge Counter";

// Custom GATT UUIDs. Layout and endianness are in docs/ble.md.
static constexpr char BLE_SERVICE_UUID[] = "8addf38b-7eab-4cec-a983-6597a8cebcc0";
static constexpr char BLE_COUNT_UUID[] = "3f037c73-e2c7-4a6f-ae69-e30ad2bc3c05";
static constexpr char BLE_MAX_UUID[] = "ddc46a1f-005c-43b2-bbbe-328a76ce631b";
static constexpr char BLE_NAME_UUID[] = "f4af48c7-fe47-4658-b580-155a69a8fcea";
static constexpr char BLE_LOCATE_UUID[] = "64da8c3f-04ac-4f61-8fc3-5a089c800558";

// On-disk record. See the persistence section of docs/behavior.md.
static constexpr char STORAGE_MAGIC[4] = {'C', 'C', '0', '2'};
static constexpr char STORAGE_PATH[] = "/counter.bin";
