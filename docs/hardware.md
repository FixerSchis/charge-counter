# Hardware

The first device is a [Seeed Studio XIAO nRF52840](https://wiki.seeedstudio.com/XIAO_BLE/), not the Sense variant. The Sense adds an IMU and a microphone that this product does not use. The module already has a Nordic nRF52840 (BLE 5, SoftDevice S140), a USB-C connector, a BQ25101 Li-ion charger, a user RGB LED, and a charge LED.

Pin numbers below are the Adafruit nRF52 core names from [`variants/Seeed_XIAO_nRF52840/variant.h`](https://github.com/Seeed-Studio/Adafruit_nRF52_Arduino/blob/master/variants/Seeed_XIAO_nRF52840/variant.h). D0 through D10 match the silkscreen.

## Bill of materials

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | Seeed Studio XIAO nRF52840 | Non-Sense |
| 1 | Momentary pushbutton | Normally open |
| 8 | LED | Red or yellow-green. Blue and white often have a forward voltage too close to 3.3 V |
| 8 | Series resistor | Sized for about 1 mA. See below |
| 1 | Protected single-cell Li-ion or LiPo | Soldered to the BAT+ and BAT− pads on the back of the module |

Resistor starting point for a red LED with a forward drop around 2.0 V, at 1 mA, from a 3.3 V GPIO:

```text
R = (3.3 V − 2.0 V) / 0.001 A = 1.3 kΩ
```

1.2 kΩ or 1.5 kΩ is a fine first build. Eight LEDs at 1 mA is about 8 mA, and only while a reading is showing. Measure the actual forward voltage of the LEDs you buy and resize from the same formula.

## Wiring

| XIAO pin | Arduino | nRF pin | Connects to |
| --- | --- | --- | --- |
| D0 | 0 | P0.02 | LED bit 0 (value 1), through its resistor |
| D1 | 1 | P0.03 | LED bit 1 (value 2) |
| D2 | 2 | P0.28 | LED bit 2 (value 4) |
| D3 | 3 | P0.29 | LED bit 3 (value 8) |
| D4 | 4 | P0.04 | LED bit 4 (value 16) |
| D5 | 5 | P0.05 | LED bit 5 (value 32) |
| D6 | 6 | P1.11 | LED bit 6 (value 64) |
| D7 | 7 | P1.12 | LED bit 7 (value 128) |
| D8 | 8 | P1.13 | Button, other side to GND |
| BAT+ / BAT− | — | — | Protected cell. Confirm polarity against the [Seeed wiki](https://wiki.seeedstudio.com/XIAO_BLE/) before soldering |

Each external LED is active high: GPIO, resistor, LED anode, LED cathode, GND. The firmware's internal pull-up holds the button line high; a press shorts it to ground. No external pull-up is required.

D6 and D7 are the hardware UART on this module. Debug output uses the USB serial port, so those pads are free for LEDs. D4 and D5 are the primary I2C pads and are used as LEDs as well.

D9 and D10 stay free.

The onboard green LED is the reading-active indicator. Leave the red and blue channels off. The separate charge LED is driven by the charger, not by firmware.

## Charging

USB-C powers the board and charges the cell through the BQ25101.

Leave Arduino pin 22 (`PIN_CHARGING_CURRENT`, P0.13) as a high-impedance input. That is the board's approximately 50 mA charge setting, which is the right default for a small cell. Driving the pin low selects approximately 100 mA. Driving it high has been reported to select 0 mA. Firmware must not configure this pin unless a chosen cell is rated for the higher current.

The onboard charge LED follows the charger. Seeed documents the charge-current pins in the [battery charging current](https://wiki.seeedstudio.com/XIAO_BLE/) section of the XIAO nRF52840 wiki.

## Battery measurement

The core names the divider-enable pin `VBAT_ENABLE` (Arduino 14, P0.14) and the sense pin `PIN_VBAT` (Arduino 32, P0.31). Drive the enable low only during a short sample, then release it. Seeed's wiki (Q3 on the same page) warns that mishandling P0.14 can put P0.31 above its input limit and damage the pad. The battery percentage characteristic is part of the BLE work; the sample routine has to respect that warning.

## Bring-up checks

1. Button between D8 and GND reads low while pressed and high while released.
2. Each of D0–D7 lights its own LED and no other.
3. A protected cell on BAT+ / BAT− charges from USB, and the charge LED follows the charger.
4. Pin 22 is still an input after the firmware boots.
