# Firmware

PlatformIO project for the nRF52832 on the custom board. The build uses the Adafruit Feather nRF52832 package so the SoftDevice is S132. Pin numbers in `src/board_pins.h` are P0 indexes and are written with the nRF GPIO registers, so they do not follow the Feather silkscreen. Flashing this binary to a Feather wiggles the wrong pins.

## Build

Install [PlatformIO](https://platformio.org/), then from this directory:

```sh
pio run -e charge_counter
pio run -e charge_counter -t upload
pio device monitor
```

Upload for the product is SWD on the test pads in [docs/hardware.md](../docs/hardware.md). USB-C on the board is charge power only.

The current sources configure the button and drive the count LEDs off. They do not recognize gestures, map the count onto the LEDs, advertise, or write flash. `loop()` calls `delay(50)` while idle so the board does not spin at full speed if this tree is flashed as-is.

## Modules

| File | Role |
| --- | --- |
| `src/main.cpp` | Turns gesture events into store, LED, notify, and commit calls. Turns the display off after `DISPLAY_MS`. Sleeps only when idle. |
| `src/config.h` | Timings, defaults, UUIDs, storage constants |
| `src/board_pins.h` | P0 assignments for the E73 module |
| `src/board_gpio.*` | Output-low and input-pull-up on those pins |
| `src/counter_store.*` | Range rules in RAM. `commit()` does not write flash yet |
| `src/button_gestures.*` | P0.13 input pull-up. `poll()` always returns `None` |
| `src/led_display.*` | `show()` records the value and does not light the pattern. `clear()` turns every count LED off |
| `src/ble_link.*` | Holds the max-write callback. No radio yet |
| `src/power_mgmt.*` | `idle()` is a short delay |

Specs: [behavior](../docs/behavior.md), [hardware](../docs/hardware.md), [BLE](../docs/ble.md), [power](../docs/power.md).

Temporary logs are appropriate while a module is being brought up. The field path stays quiet.
