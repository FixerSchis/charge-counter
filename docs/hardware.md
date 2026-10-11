# Hardware

The product is a custom two-layer PCB. Most of it is assembled on one side by a budget SMT service. The USB-C socket and the button are soldered by hand after the boards come back, so those two extended parts never pay a feeder fee.

The machine-placed list is chosen for [JLCPCB Economic assembly](https://jlcpcb.com/help/article/pcb-assembly-faqs). Basic parts have no feeder-loading fee. Extended parts cost about $3 each. PCBWay can fabricate the same Gerbers; their loading fees are different, so a PCBWay quote should be checked against this same single-sided SMT set.

Library types below were read from JLCPCB part pages on 11 Oct 2026. JLCPCB moves parts between Basic and Extended, so check the extended lines again at order time. If a Preferred Extended substitute is in stock, use it: Preferred parts also have no feeder fee on Economic assembly.

## What gets assembled, and what does not

| Who places it | Parts |
| --- | --- |
| The fab, from the Machine rows in [hardware/bom.csv](../hardware/bom.csv) | Module, charger, USB-serial chip, LDO, LEDs, passives, divider MOSFET |
| Hand, after the boards arrive | USB-C socket, tactile button, and the lithium cell |

The button is four SMT pads. Flux and a normal iron are enough.

The USB-C socket is [TYPE-C-31-M-12](https://jlcpcb.com/partdetail/Korean_HropartsElec-TYPE_C_31_M12/C165948) (C165948), a right-angle part whose signal pins stick out behind the shell. Drag-solder those pins with flux, then wick any bridges. The shell tabs need a bit more heat. A mid-mount socket with hidden pins would be a poor hand-solder, so the footprint stays this one. Leave both parts off the fab BOM. Ordering them as "do not place" still risks a feeder fee; buy them separately and solder them yourself.

Rules that keep the fab order on the cheap tier:

- One side only. Parts on the back, or a second assembly pass, move the order to Standard assembly, which charges a loading fee for every part.
- Nothing through-hole on the fab order.
- No part whose page says Standard Only. That excludes the Ebyte E73-2G4M08S1C and the Raytac MDBT50Q, both nRF52840 modules with native USB. Putting either on the board forces the whole order off Economic assembly (setup about $26, plus a loading fee on every part, plus about $1.64 of X-ray per board). That is more than one extra extended part.
- 0603 and larger passives.
- Standard stackup: 2 layers, 1.6 mm, the fab's default solder mask and surface finish.
- Three fiducials on the assembled side.

The machine-placed extended lines are the nRF52832 module, the TP4056, and the CH340C. That is about $9 of feeder fees on top of the Economic setup charge (about $8 plus a stencil of about $1.50). The LEDs, resistors, capacitors, LDO, and divider MOSFET are Basic.

## Programming

The nRF52832 has no USB device. A [CH340C](https://jlcpcb.com/partdetail/WCH_Jiangsu_Qin_Heng-CH340C/C84681) (C84681) sits between the USB-C data pins and a UART on the module, so a cable is enough to load firmware. The CH340C needs no crystal. Follow its crystal-less reference circuit: VCC from VBUS, 100 nF on V3, D+ and D− straight to the socket.

The chip's listed quiescent current is about 50 µA, so it is powered only from VBUS. Unplugging the cable removes its power. DTR goes through 100 nF to P0.21, which has a 10 kΩ pull-up to 3.3 V, so the uploader can reset into the serial bootloader. The capacitor blocks DC, and the pull-up holds reset released while the CH340 is unpowered.

| Net | Module pin | CH340C |
| --- | --- | --- |
| UART TX | P0.12 | RXD |
| UART RX | P0.15 | TXD |
| Reset | P0.21 | DTR, through 100 nF |

VBUS sense is a second pair of 10 kΩ resistors: VBUS, 10 kΩ, P0.17, 10 kΩ, ground. At 5 V that node is about 2.5 V, which is a valid high for the 3.3 V GPIO. While P0.17 reads low, field firmware leaves P0.12 and P0.15 as disconnected inputs with no pull. A pull-up there would feed the unpowered CH340 through its pin diodes and drain the cell.

The bootloader uses this UART at 115200. SWD pads (SWDIO, SWCLK, VCC, GND) stay on the board for the first bootloader flash and for recovery. Day-to-day firmware updates go over USB-C.

SoftDevice S132. The phone-facing GATT contract does not change.

## Radio

[Ebyte E73-2G4M04S1B](https://jlcpcb.com/partdetail/Chengdu_Ebyte_ElecTech-E732G4M04S1B/C411306), LCSC C411306. It is an nRF52832 with a PCB antenna, and JLCPCB lists it as Extended and eligible for Economic assembly.

The module is 28.7 mm by 17.5 mm. Put the antenna end on the board edge and leave the keep-out the E73 user manual draws, with no copper under the antenna on any layer. VCC is 1.8 V to 3.6 V. A LiPo at 4.2 V must not connect to VCC.

DEC1, DEC2, DEC3, DEC4, and DCC get the capacitors from the E73 user-manual reference circuit. Use basic-library 0603 ceramics of those values. They are extra lines on the BOM once the manual's values are copied.

## Power

```text
USB-C VBUS (5 V)
  CC1, CC2 each 5.1 kΩ to GND
  TP4056 VCC, and CH340C VCC
  CHRG  -- LED -- 1.5 kΩ -- VBUS     charge lamp, off when USB is unplugged
  VBUS -- 10 kΩ -- P0.17 -- 10 kΩ -- GND

TP4056 BAT -- cell pad BAT+
           -- XC6206 input
XC6206 3.3 V -- module VCC, with 10 µF and 100 nF to GND

BAT+ -- 1 MΩ -- AIN -- 1 MΩ -- 2N7002 drain
2N7002 source -- GND
2N7002 gate  -- P0.14
```

TP4056 (C16581) charges one cell to 4.2 V. With USB removed, the datasheet battery drain is under 3 µA. PROG is 10 kΩ to ground. The charge-current formula in the TOPPOWER datasheet is `IBAT = (VPROG / RPROG) × 1100` with VPROG at 1 V in constant current, so 10 kΩ is about 110 mA. Use a protected cell of at least 220 mAh so that stays under 0.5C. TEMP is tied to ground, which the datasheet says disables the thermistor input. CE is tied to VCC. The exposed pad is soldered to ground. STDBY is left open so there is no second charge LED.

XC6206P332MR (C5446) is a basic-library 3.3 V LDO with about 1 µA quiescent current. It feeds the module. When the cell sags near 3.3 V the regulator drops out and the module keeps running on the falling cell voltage, down to the module's 1.8 V minimum. Red LEDs still light there; they get dimmer.

The charge LED is the same red LED as the count LEDs, from VBUS, so it cannot drain the cell. It is not one of the eight count LEDs.

The battery divider is off unless P0.14 is high. The ADC full-scale reading uses the nRF internal reference, not VDD, so the percentage tracks the cell rather than the LDO output. Half of 4.2 V is 2.1 V, inside a 3.6 V full-scale range.

## Count LEDs and the button

Eight red LEDs, KT-0603R (C2286), basic library. Forward voltage is about 1.8 V to 2.4 V, so they still light from 3.3 V. Each is active high: GPIO, 1.5 kΩ, LED anode, cathode to ground. At a 2.0 V drop that is about 0.9 mA, and eight of them are about 7 mA, only while a reading is showing.

| Bit | Value | Module pin |
| --- | --- | --- |
| 0 | 1 | P0.02 |
| 1 | 2 | P0.03 |
| 2 | 4 | P0.04 |
| 3 | 8 | P0.05 |
| 4 | 16 | P0.06 |
| 5 | 32 | P0.07 |
| 6 | 64 | P0.08 |
| 7 | 128 | P0.11 |

P0.09 and P0.10 stay free. They are the NFC pins, and using them as GPIO needs a UICR setting the firmware does not rely on.

The button is the hand-soldered tactile switch from P0.13 to ground. The firmware enables the internal pull-up.

There is no ninth count LED. A reading of zero is the opening flash described in [docs/behavior.md](behavior.md).

## Cell

A protected single-cell Li-ion or LiPo, soldered to BAT+ and BAT− pads after assembly. Silkscreen the polarity. The cell is not part of the fab order.

## Bring-up

1. P0.13 reads low while the button is held and high when released.
2. Each of P0.02, P0.03, P0.04, P0.05, P0.06, P0.07, P0.08, and P0.11 lights only its own LED.
3. With USB plugged in and a cell on the pads, the charge LED follows CHRG and the cell voltage rises toward 4.2 V.
4. Module VCC stays at or below 3.6 V while the cell is at 4.2 V.
5. With P0.14 low, the divider does not present a DC path from the cell to ground.
6. Plugging in USB-C enumerates the CH340, and a serial upload resets the module into the bootloader.
7. With USB unplugged, P0.12 and P0.15 are not pulled, and the cell is not feeding the CH340.
