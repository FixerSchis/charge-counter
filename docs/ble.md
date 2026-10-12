# BLE contract

The device is a peripheral. The phone is a central. Several devices are several peripherals; each one stores its own count and max. This document is the contract both sides implement.

SoftDevice S132 on the nRF52832 module. Custom service UUIDs are in `firmware/src/config.h`. Firmware updates go over USB-C through the CH340C UART in [docs/hardware.md](hardware.md).

## Identity

Advertise a stable address (the chip's public address or a static random address). A rotating private address would make one device look like a new device to the phone.

| Field | Value |
| --- | --- |
| Advertising | Flags (general discoverable, BR/EDR not supported) and the 128-bit service UUID |
| Scan response | Complete local name `CC-XXXX` |
| Scan response, pairing window only | Service data for that same UUID, one byte `01` |
| `XXXX` | Lower 16 bits of the device address, four uppercase hex digits |

The 128-bit UUID fills most of the advertising packet, so the name goes in the scan response. The pairing byte is added there only while the bonding window is open, and removed when the window closes. The advertising interval target is `BLE_ADV_INTERVAL_MS` (1000 ms), which is 1600 units of 0.625 ms.

Preferred connection parameters, as a starting point for the power work to measure and adjust:

| Parameter | Value |
| --- | --- |
| Minimum interval | 80 ms |
| Maximum interval | 150 ms |
| Peripheral latency | 4 |
| Supervision timeout | 4 s |

## Service

UUID `8addf38b-7eab-4cec-a983-6597a8cebcc0`.

Integers are unsigned 16-bit, little-endian. Count 5 is the bytes `05 00`. Max 8 is `08 00`. Reads and notifications are exactly 2 bytes. Writes that are not exactly 2 bytes are rejected.

| Characteristic | UUID | Properties | Value |
| --- | --- | --- | --- |
| Count | `3f037c73-e2c7-4a6f-ae69-e30ad2bc3c05` | Read, Notify | count |
| Max | `ddc46a1f-005c-43b2-bbbe-328a76ce631b` | Read, Write, Notify | max |
| Name | `f4af48c7-fe47-4658-b580-155a69a8fcea` | Read, Write, Notify | UTF-8 label, 0 to 32 bytes |
| Locate | `64da8c3f-04ac-4f61-8fc3-5a089c800558` | Write | one byte, `01` starts the chase |

Also expose:

| Service | Characteristic | Contents |
| --- | --- | --- |
| Battery, `0x180F` | Battery Level, `0x2A19` | 0–100 percent, from a short VBAT sample |
| Device Information, `0x180A` | Model Number, `0x2A24` | `Charge Counter` |
| Device Information, `0x180A` | Firmware Revision, `0x2A26` | `FW_VERSION` from `config.h` |

## When values change

| Cause | Notify |
| --- | --- |
| Add or subtract changed the count | Count |
| Add or subtract clamped and the count stayed put | Nothing |
| Phone wrote a new in-range max | Max, and Count as well if the count was clamped |
| Phone wrote the max the device already has | Nothing. The write still succeeds |
| Phone wrote 0, a value above 16, or a length other than 2 | Reject the write. Nothing is stored or notified |
| Phone wrote a new name | Name |
| Phone wrote the name the device already has | Nothing. The write still succeeds |
| Phone wrote a name longer than 32 bytes, or bytes that are not UTF-8 | Reject the write. Nothing is stored or notified |
| Phone wrote `01` to Locate | Nothing. The chase runs |

A max write does not turn the LEDs on. If the LEDs are already showing a reading and the count was clamped, the pattern updates. A name write does not turn the LEDs on. An empty name is a legal write and clears the label.

Notifications are sent when a central is connected and subscribed. The value is still stored when nobody is listening. A later read returns the stored value.

## Security

Count, max, name, and battery are readable only over an encrypted link from a phone this device has bonded with. Locate is writable only on that same link. A phone that has not bonded cannot connect, read, or write. The advertising name stays `CC-XXXX`. The label the owner types is the Name characteristic, so an advertisement does not reveal it.

The device keeps advertising while it is idle, so a generic Bluetooth scanner can still see the service UUID and `CC-XXXX`. The Charge Counter app does not. It lists a counter only when this phone has bonded it, or when the scan response contains the pairing byte. A counter that is nearby, not bonded, and not in the pairing window is ignored.

Bonding is LE Secure Connections, Just Works, and only while the device has opened a pairing window. The window opens from the hold in [docs/behavior.md](behavior.md): hold until the count is at max, then keep holding for three seconds. No cable. With the default max of 2, that is a short hold if the count is already 2, and a longer one if it has to climb. The eight lamps then blink blue together for `PAIR_OPEN_MS` (60 s) and the device accepts one new bond. The window closes when that bond completes, when the user short-presses, or when the minute ends.

A phone that is already bonded reconnects with no special hold. A second phone needs its own hold-to-max. Each phone's app lists only the counters it has bonded. Someone else's phone keeps its own list and cannot read this counter.

While the pairing lamps are flashing, a hold of three seconds, begun after the finger has lifted, forgets every bonded phone and leaves the window open. That is how a lost phone loses access.

The Name value is stored on the device with the count and max. An empty name means the app shows `CC-XXXX`. Locate writes `01` and the lamps chase in blue, first through eighth, four times. The count does not change.

## Phone behavior

The app filters scans to this service UUID, then applies a second filter. The main list is the counters this install has bonded, keyed by the stable address. Each row shows the stored name, or `CC-XXXX` when the name is empty, plus count, max, battery, and time seen. A bonded device that is out of range stays in the list with the cached values. A scan result that is not one of those addresses does not appear on this list, even when the name is `CC-XXXX`.

Adding a counter is a separate screen. It lists only advertisements whose scan response contains the pairing byte, so several counters can be in that window at once and the phone can bond to the ones the user picks. A nearby counter that is not bonded and is not advertising that byte is not shown. One bond per window. Connecting reads count, max, name, and battery, then subscribes to count, max, and name. Editing max writes the Max characteristic and then displays the values the device notifies or returns on read. Editing the name writes the Name characteristic. Locate writes `01` and the matching counter chases its lamps. Writing device A's max, name, or locate does not change device B. Removing a counter from the app drops that phone's bond on the device while it is connected.
