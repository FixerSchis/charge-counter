# Power

The device has to stay reachable from a phone and still run for a long time on a small cell. The idle load is the radio, the XC6206 (about 1 µA), and the TP4056's sleep drain (under 3 µA with USB unplugged). The eight count lamps are an addressable chain on a switched cell rail, so their standby current is disconnected except during a reading. Left connected, each one would draw about 0.5 mA while dark. The charge LED is powered from USB, so it is dark when the device is on the cell alone.

The supply circuit is in [docs/hardware.md](hardware.md). Module VCC is the LDO output, never the cell.

## Policy

Idle means the gesture recognizer is idle and the count LEDs are off. In that state the firmware waits for the next event instead of spinning.

Use system-on sleep with the SoftDevice still scheduled. System OFF stops advertising, and the phone could not monitor the device until somebody pressed the button. Advertising stays on at the 1000 ms interval in [docs/ble.md](ble.md).

`PowerMgmt::idle()` returns on a button edge or a SoftDevice event. `BleLink::poll()` runs before each wait. A connected phone is not a reason to stay awake between connection events.

P0.14 stays low while the divider is not being sampled, so the two 1 MΩ resistors are not a permanent load. While P0.17 reads low (USB unplugged), P0.12 and P0.15 stay disconnected inputs with no pull, so the unpowered CH340 cannot draw from the cell. Debug logging stays out of the field firmware. The serial bootloader may use that UART, because it only runs while USB is plugged in.

Charge current is the PROG resistor, about 110 mA with 10 kΩ. Firmware does not change it.

## What to measure

The power work records, on a finished board and a stated cell:

| State | What to capture |
| --- | --- |
| Idle, advertising, count LEDs off, USB unplugged | Average current from the cell |
| Connected, idle, count LEDs off | Average current from the cell |
| All eight count LEDs lit | Current above the idle baseline |
| USB plugged in, cell charging | Charge current into the cell |

Put the measured numbers and the resulting life on that cell back into this file. Until those measurements exist, this project does not claim a runtime.

Battery percentage uses one short sample: drive P0.14 high, read AIN4 on P0.28 against the internal reference, then drive P0.14 low. The mapping from cell voltage to percent is one function, so it can be replaced after the charge curve is measured. A first mapping can be a straight line from an empty cell to a full cell.
