# Firmware

PlatformIO project for the nRF52832 on the custom board. The build uses the Adafruit Feather nRF52832 package so the SoftDevice is S132. Pin numbers in `src/board_pins.h` are P0 indexes and are written with the nRF GPIO registers, so they do not follow the Feather silkscreen. Flashing this binary to a Feather wiggles the wrong pins.

## Build

Install [PlatformIO](https://platformio.org/), then from this directory:

```sh
pio run -e charge_counter
pio run -e charge_counter -t upload
pio device monitor
```

Day-to-day upload is USB-C. The CH340C turns that into UART on P0.12 (TX) and P0.15 (RX) at 115200, and DTR resets the module through P0.21. The serial bootloader has to be built for those pins. SWD pads are for the first bootloader flash and for recovery. See [docs/hardware.md](../docs/hardware.md).

The current sources recognize button gestures, write `/counter.bin`, and drive the count LEDs off. They do not map the count onto the LEDs or advertise. `loop()` calls `delay(50)` while idle so the board does not spin at full speed if this tree is flashed as-is.

## Modules

| File | Role |
| --- | --- |
| `src/main.cpp` | Turns gesture events into store, LED, notify, and commit calls. Turns the display off after `DISPLAY_MS`. Sleeps only when idle. |
| `src/config.h` | Timings, defaults, UUIDs, storage constants |
| `src/board_pins.h` | P0 assignments, including the CH340 UART and VBUS sense |
| `src/board_gpio.*` | Output-low and input-pull-up on those pins |
| `src/counter_store.*` | Range rules in RAM. `begin()` / `commit()` use `/counter.bin` with CRC-16 |
| `src/button_gestures.*` | View, subtract chain, hold-to-add, and hold-at-max pairing |
| `src/led_display.*` | `show()` records the value and does not light the pattern. `clear()` turns every count LED off |
| `src/ble_link.*` | Holds the max-write callback. No radio yet |
| `src/power_mgmt.*` | `idle()` is a short delay |

Specs: [behavior](../docs/behavior.md), [hardware](../docs/hardware.md), [BLE](../docs/ble.md), [power](../docs/power.md).

Temporary logs are appropriate while a module is being brought up. The field path stays quiet.
