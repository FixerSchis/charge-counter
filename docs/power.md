# Power

The device has to stay reachable from a phone and still run for a long time on a small cell. The radio, not the LEDs, is the idle load: the LEDs are off except for a 2.5 second reading.

## Policy

Idle means the gesture recognizer is idle and the LEDs are off. In that state the firmware waits for the next event instead of spinning.

Use system-on sleep with the SoftDevice still scheduled. System OFF stops advertising, and the phone could not monitor the device until somebody pressed the button. Advertising stays on at the 1000 ms interval in [docs/ble.md](ble.md).

`PowerMgmt::idle()` returns on a button edge or a SoftDevice event. `BleLink::poll()` runs before each wait. A connected phone is not a reason to stay awake between connection events.

The onboard red and blue LEDs stay off. The green LED is on only during a reading. USB serial logging stays out of the field firmware; it keeps the USB block awake.

Leave the charger current pin alone, as described in [docs/hardware.md](hardware.md).

## What to measure

The power work records, on a real cell and a real board:

| State | What to capture |
| --- | --- |
| Idle, advertising, LEDs off | Average current |
| Connected, idle, LEDs off | Average current |
| All eight LEDs lit | Current above the idle baseline |
| Charging from USB | Charge current at the default (pin 22 untouched) setting |

Put the measured numbers and the resulting life on the chosen cell back into this file. Until those measurements exist, this project does not claim a runtime.

Battery percentage for the standard Battery Level characteristic uses a short sample of `PIN_VBAT` with `VBAT_ENABLE` asserted only during that sample. The mapping from cell voltage to percent is one function, so it can be replaced after the charge curve is measured. A first mapping can be a straight line from an empty cell to a full cell; label it as uncalibrated in the characteristic if it ships before the measurement.
