# Phone app

One app monitors the Charge Counters this phone has bonded with, and edits each of those devices' max and name. Counters bonded to someone else's phone are not listed and cannot be read or changed from here. The radio contract is [docs/ble.md](../docs/ble.md). The app is not in the tree yet.

## Stack

Flutter, with `flutter_blue_plus`, so iOS and Android share one codebase and one BLE client. The client speaks the GATT contract directly. A different stack is fine if it implements that same contract.

## Screens

**Device list.** Rows are only counters this install has bonded. Scanning is filtered to the Charge Counter service UUID, and a device in range that is not bonded does not appear, including one whose advertisement name is `CC-XXXX`. Each row shows the name stored on the device, or `CC-XXXX` when that name is empty, plus count, max, battery, and last-seen time. A bonded device that is out of range stays in the list with the cached values. Rows are keyed by the stable BLE address, so two devices never share a row.

**Add a counter.** The screen tells the user to hold the button until the count reaches max, then keep holding for three seconds, until all eight lamps blink blue. It lists only counters whose scan response contains the pairing byte, and bonds to the ones the user picks. A counter that is nearby but not in that window is not listed. One bond per window. Cancelling, or leaving the screen, does not remove counters already bonded.

**Device detail.** Large count, the stored name, max, battery, and connection state. The count, name, and max update from notifications while connected. Opening the screen connects if needed, reads those values, and subscribes. Locate starts the LED chase on this counter only. Remove drops this phone's bond on the device, then drops the row.

**Name editor.** A label up to 32 bytes of UTF-8 for this device. Save writes the Name characteristic. An empty save clears the label and the list falls back to `CC-XXXX`. The screen then shows whatever the device reports. A rejected write stays on the editor with the failure visible.

**Max editor.** A value from 1 to 16 for this device. Save writes the Max characteristic. The screen then shows whatever the device reports. A rejected write stays on the editor with the failure visible. Saving on one device does not write to another.

The phone stores the cache (address, name, last count, last max, last battery, last seen). The device is the source of truth for the count, the max, and the name. After a reconnect, the read values replace the cache.

## Permissions

Android needs the Bluetooth scan and connect permissions for the OS version in use. iOS needs a Bluetooth usage description. The app explains why before the system prompt.
