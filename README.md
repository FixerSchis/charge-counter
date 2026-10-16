# Charge Counter

A small rechargeable counter. One button, eight LEDs, and a number from 0 to a per-device maximum that survives power loss. A phone app talks to any number of these devices over Bluetooth Low Energy and can read each count or set each device's maximum.

The behavior, wiring, radio contract, and power policy are specified. The firmware implements those contracts. The phone app in [app/](app/) lists the counters that phone has bonded and edits each name and max.

## What the button does

| Gesture | Result |
| --- | --- |
| Single press | Show the current number on the LEDs, then turn them off. The number does not change. |
| Double press | Subtract one. Further presses in the same rhythm subtract one more each. |
| Long press | Add one when the hold crosses the long-press time, then keep adding while the hold continues. |
| Hold on after the count reaches max | Three more seconds opens pairing. All eight lamps blink blue, and a phone can bond. |

The number stays inside 0 and the device's max. Max is an integer from 1 to 16, stored on the device, defaulting to 2, and changed from the phone. The full timing and edge cases are in [docs/behavior.md](docs/behavior.md).

The same gestures work from an external button on a two-pin socket. It is wired in parallel with the on-board button, so the device still has one input.

The eight RGB lamps count one step at a time. The first pass is red: one lamp is 1, eight lamps is 8. The second pass turns those lamps green, so 9 is one green lamp and seven red, and 16 is all green. Each reading starts with a short blue flash of all eight, so a count of zero is visible and then goes dark. Blue is also the pairing blink and the locate chase.

## How the pieces fit

```mermaid
flowchart LR
  button[Button] --> gestures[Gesture recognizer]
  gestures --> store[Count and max]
  store --> leds[Eight LEDs]
  store --> flash[Flash record]
  store --> ble[BLE peripheral]
  phone[Phone app] --> ble
  ble --> store
```

The phone is a central. Each device is its own peripheral with its own count, max, and name. The app lists the counters that phone has bonded, and, on the add screen, counters that are in the pairing window. A counter that is neither bonded nor pairing is not shown. The app shows the latest count and writes a new max or name to a bonded device only. Bonding takes a hold until the count is at max, then three more seconds, with no cable. Another person's phone cannot read or change a counter it has not bonded. Locate makes that one counter chase its eight lamps in blue so it can be picked out of a group.

## Repository

| Path | Contents |
| --- | --- |
| [docs/behavior.md](docs/behavior.md) | Button state machine, LED encoding, flash record |
| [docs/hardware.md](docs/hardware.md) | Custom PCB, charging, and assembly rules |
| [hardware/bom.csv](hardware/bom.csv) | JLCPCB part numbers |
| [docs/ble.md](docs/ble.md) | Advertising and GATT contract |
| [docs/power.md](docs/power.md) | Sleep and battery-life policy |
| [firmware/](firmware/) | PlatformIO project for the nRF52832 module |
| [app/](app/) | Flutter app for bonded counters |

The board is a custom two-layer PCB built around an Ebyte E73-2G4M04S1B (nRF52832). Firmware loads over USB-C through a CH340C that is only powered while the cable is plugged in. JLCPCB Economic assembly places one side. The USB-C socket, the button, the cell socket, and the external-button socket are hand soldered. The machine-placed extended parts are the module, the charger, the CH340C, and the eight addressable lamps (one feeder).

## Firmware status

| Module | What is in the tree |
| --- | --- |
| `main.cpp` | Connects gestures, storage, LEDs, BLE, and sleep |
| `counter_store` | RAM clamping plus `/counter.bin` load and commit |
| `button_gestures` | View, subtract chain, hold-to-add, hold-at-max pairing |
| `led_display` | Blue ACK flash, red-then-green count, pairing blink, locate |
| `ble_link` | Advertising, GATT, bonding window, battery sample |
| `power_mgmt` | System-on idle; UART pins float while USB is unplugged |

Build instructions are in [firmware/README.md](firmware/README.md). Idle current still needs measuring on the assembled board for [#6](https://github.com/FixerSchis/charge-counter/issues/6).

## Work remaining

| Issue | Feature |
| --- | --- |
| [#1](https://github.com/FixerSchis/charge-counter/issues/1) | Custom PCB laid out for cheap SMT assembly |
| [#6](https://github.com/FixerSchis/charge-counter/issues/6) | Measured battery life on the finished board |
| [#7](https://github.com/FixerSchis/charge-counter/issues/7) | Phone app that monitors several devices |
| [#8](https://github.com/FixerSchis/charge-counter/issues/8) | Per-device max edited from the phone |

## License

[MIT](LICENSE).
