# BLE contract

The device is a peripheral. The phone is a central. Several devices are several peripherals; each one stores its own count and max. This document is the contract both sides implement.

SoftDevice S140 on the XIAO nRF52840. Custom service UUIDs are in `firmware/src/config.h`.

## Identity

Advertise a stable address (the chip's public address or a static random address). A rotating private address would make one device look like a new device to the phone.

| Field | Value |
| --- | --- |
| Advertising | Flags (general discoverable, BR/EDR not supported) and the 128-bit service UUID |
| Scan response | Complete local name `CC-XXXX` |
| `XXXX` | Lower 16 bits of the device address, four uppercase hex digits |

The 128-bit UUID fills most of the advertising packet, so the name goes in the scan response. The advertising interval target is `BLE_ADV_INTERVAL_MS` (1000 ms), which is 1600 units of 0.625 ms.

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
| Phone wrote 0, a value above 255, or a length other than 2 | Reject the write. Nothing is stored or notified |

A max write does not turn the LEDs on. If the LEDs are already showing a reading and the count was clamped, the pattern updates.

Notifications are sent when a central is connected and subscribed. The value is still stored when nobody is listening. A later read returns the stored value.

## Security

The first version is open: no pairing. Anyone in radio range who can connect can read the count and write max. Pairing can be added later without changing the characteristic layout.

## Phone behavior

The app filters scans to this service UUID. It remembers devices by the stable address and shows the last count, max, battery, and time seen when a device is out of range. Connecting reads count, max, and battery, then subscribes to count and max. Editing max writes this characteristic and then displays the values the device notifies or returns on read. Writing device A's max does not change device B.
