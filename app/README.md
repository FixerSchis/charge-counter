# Phone app

One app monitors every Charge Counter in range and edits each device's max. The radio contract is [docs/ble.md](../docs/ble.md). The app is not in the tree yet.

## Stack

Flutter, with `flutter_blue_plus`, so iOS and Android share one codebase and one BLE client. The client speaks the GATT contract directly. A different stack is fine if it implements that same contract.

## Screens

**Device list.** Scanning is filtered to the Charge Counter service UUID. Each row is a device the phone has seen: `CC-XXXX` name, count, max, battery, and last-seen time. A device that is out of range stays in the list with the cached values. Rows are keyed by the stable BLE address, so two devices never share a row.

**Device detail.** Large count, max, battery, and connection state. The count and max update from notifications while connected. Opening the screen connects if needed, reads the three values, and subscribes.

**Max editor.** A value from 1 to 255 for this device. Save writes the Max characteristic. The screen then shows whatever the device reports. A rejected write stays on the editor with the failure visible. Saving on one device does not write to another.

The phone stores the cache (address, name, last count, last max, last battery, last seen). It does not store the authoritative count. After a reconnect, the read values replace the cache.

## Permissions

Android needs the Bluetooth scan and connect permissions for the OS version in use. iOS needs a Bluetooth usage description. The app explains why before the system prompt.
