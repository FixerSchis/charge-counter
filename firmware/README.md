# Firmware

PlatformIO project for the Seeed Studio XIAO nRF52840, using Seeed's Adafruit nRF52 board definition `seeed-xiao-afruitnrf52-nrf52840`.

## Build

Install [PlatformIO](https://platformio.org/), then from this directory:

```sh
pio run -e seeed_xiao_nrf52840
pio run -e seeed_xiao_nrf52840 -t upload
pio device monitor
```

The current sources configure the button and drive the LEDs off. They do not recognize gestures, map the count onto the LEDs, advertise, or write flash. `loop()` calls `delay(50)` while idle so the board does not spin at full speed if this tree is flashed as-is.

## Modules

| File | Role |
| --- | --- |
| `src/main.cpp` | Turns gesture events into store, LED, notify, and commit calls. Turns the display off after `DISPLAY_MS`. Sleeps only when idle. |
| `src/config.h` | Timings, defaults, UUIDs, storage constants |
| `src/counter_store.*` | Range rules in RAM. `commit()` does not write flash yet |
| `src/button_gestures.*` | `D8` input pull-up. `poll()` always returns `None` |
| `src/led_display.*` | `show()` records the value and does not light the pattern. `clear()` turns every LED off |
| `src/ble_link.*` | Holds the max-write callback. No radio yet |
| `src/power_mgmt.*` | Does not touch the charge-current pin. `idle()` is a short delay |

Specs: [behavior](../docs/behavior.md), [hardware](../docs/hardware.md), [BLE](../docs/ble.md), [power](../docs/power.md).

Temporary USB serial logs are appropriate while a module is being brought up. The field path stays quiet so USB does not hold the regulator awake.
