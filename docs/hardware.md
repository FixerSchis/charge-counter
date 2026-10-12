# Hardware

The product is a custom two-layer PCB. Most of it is assembled on one side by a budget SMT service. The USB-C socket and the button are soldered by hand after the boards come back, so those two extended parts never pay a feeder fee.

The copper drawings are not in the repository yet. Pin numbers below are datasheet numbers. Pad positions come from the LCSC footprint.

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

The nRF52832 has no USB device. A [CH340C](https://jlcpcb.com/partdetail/WCH_Jiangsu_Qin_Heng-CH340C/C84681) (C84681) sits between the USB-C data pins and a UART on the module, so a cable is enough to load firmware. The CH340C needs no crystal. D+ and D− go straight to the socket, A-side tied to B-side.

The chip's TX pin idles high, close to its own supply. A module pin must stay at or below VDD + 0.3 V, so the CH340 is not powered from 5 V. A second XC6206, the same basic part as the module regulator, takes VBUS down to 3.3 V and feeds CH340 VCC. V3 is tied to that same 3.3 V pin, which is the datasheet connection for a 3.3 V supply. 10 µF sits on the regulator output and 100 nF sits at the CH340. Unplugging the cable removes VBUS, so this regulator and the CH340 both go off. The listed CH340 quiescent current is about 50 µA, and it is not drawn from the cell.

DTR# is CH340 pin 13. It goes through 100 nF to module P0.21, which has a 10 kΩ pull-up to the module 3.3 V rail. The uploader pulls DTR# low to reset into the serial bootloader. The capacitor blocks DC, and the pull-up holds reset released while the CH340 is unpowered. Pin 8 (OUT#) stays open. A pulldown there would turn it into a second DTR on some lots. Pin 7 stays open. Pin 15 (R232) stays open so its internal pulldown does not invert RXD.

| Net | Module | CH340C |
| --- | --- | --- |
| UART TX | P0.12, module pin 28 | RXD, pin 3 |
| UART RX | P0.15, module pin 31 | TXD, pin 2 |
| Reset | P0.21, module pin 37 | DTR#, pin 13, through 100 nF |

Fit the cell before plugging in USB. The charger does not run the module on its own, and the VBUS divider would otherwise put voltage on a pin of an unpowered module.

VBUS sense is a second pair of 10 kΩ resistors: VBUS, 10 kΩ, P0.17, 10 kΩ, ground. At 5 V that node is about 2.5 V, which is a valid high for the 3.3 V GPIO. While P0.17 reads low, field firmware leaves P0.12 and P0.15 as disconnected inputs with no pull. A pull-up there would feed the unpowered CH340 through its pin diodes and drain the cell.

The bootloader uses this UART at 115200. SWD pads (SWDIO, SWCLK, VCC, GND) stay on the board for the first bootloader flash and for recovery. Day-to-day firmware updates go over USB-C.

SoftDevice S132. The phone-facing GATT contract does not change.

## Radio

[Ebyte E73-2G4M04S1B](https://jlcpcb.com/partdetail/Chengdu_Ebyte_ElecTech-E732G4M04S1B/C411306), LCSC C411306. It is an nRF52832 with a PCB antenna, and JLCPCB lists it as Extended and eligible for Economic assembly.

The module is 28.7 mm by 17.5 mm. Put the antenna end on the board edge and leave the keep-out the E73 user manual draws, with no copper under the antenna on any layer. VCC is 1.8 V to 3.6 V. A LiPo at 4.2 V must not connect to VCC.

DEC1, DEC2, DEC3, DEC4, and DCC are left open. Ebyte's answer for this module is that LDO mode needs nothing on those pins, and DC/DC mode would only add an inductor. This board uses the internal LDO. The English user manual lists the pins and does not give capacitor values.

## Power

```text
USB-C VBUS (5 V)
  CC1, CC2 each 5.1 kΩ to GND
  TP4056 VCC
  XC6206 U3 input
  CHRG  -- LED -- 1.5 kΩ -- VBUS     charge lamp, off when USB is unplugged
  VBUS -- 10 kΩ -- P0.17 -- 10 kΩ -- GND

U3 3.3 V -- CH340C VCC and V3, with 10 µF and 100 nF to GND

TP4056 BAT -- cell pad BAT+
           -- XC6206 U2 input
U2 3.3 V -- module VCC, with 10 µF and 100 nF to GND

BAT+ -- 1 MΩ -- AIN -- 1 MΩ -- 2N7002 drain
2N7002 source -- GND
2N7002 gate  -- P0.14
```

TP4056 (C16581) charges one cell to 4.2 V. With USB removed, the datasheet battery drain is under 3 µA. PROG is 10 kΩ to ground. The charge-current formula in the TOPPOWER datasheet is `IBAT = (VPROG / RPROG) × 1100` with VPROG at 1 V in constant current, so 10 kΩ is about 110 mA. Use a protected cell of at least 220 mAh so that stays under 0.5C. TEMP is tied to ground, which the datasheet says disables the thermistor input. CE is tied to VCC. The exposed pad is soldered to ground. STDBY is left open so there is no second charge LED.

XC6206P332MR (C5446) is a basic-library 3.3 V LDO with about 1 µA quiescent current. Two of them are fitted. U2 feeds the module from the cell. U3 feeds the CH340 from VBUS and is off when the cable is unplugged. On U2, when the cell sags near 3.3 V the regulator drops out and the module keeps running on the falling cell voltage, down to the module's 1.8 V minimum. The count lamps need the cell itself at 3.7 V or more, so they go dark before the module does. SOT-23 pin order, top view: pin 1 ground, pin 2 output, pin 3 input.

The charge LED is a red KT-0603R on VBUS, so it cannot drain the cell. It is not one of the eight count lamps.

The battery divider is off unless P0.14 is high. The ADC full-scale reading uses the nRF internal reference, not VDD, so the percentage tracks the cell rather than the LDO output. Half of 4.2 V is 2.1 V, inside a 3.6 V full-scale range.

## Count LEDs and the button

Eight addressable RGB lamps, Worldsemi WS2812C-2020-V1/W (C42417600), 2.2 × 2.0 mm. On 12 Oct 2026 JLCPCB listed that part as Extended, and as Economic and Standard. Smaller pixels that were checked the same day, including the 1.5 mm SK6805-EC15 and the 1.3 mm WS2816C-1313, are Standard Only, and one of those would move the whole order off Economic assembly. All eight lamps are the same part, so they are one feeder. The charge LED stays the basic red KT-0603R.

Each lamp has one data input and one data output. P0.02 drives the first lamp through 470 Ω. Each data output feeds the next data input. The eighth output is left open. Lamp 1 is the first to light. The four pads come from the LCSC footprint for C42417600.

The chain does not run from the 3.3 V rail. Its supply is the cell, switched. Left powered, each lamp draws about 0.5 mA while dark, and eight of them would dominate the idle budget. A high-side switch keeps that current off except during a reading:

```
BAT+ -- AO3401A source
AO3401A drain -- LED_VDD -- lamp VDD pins, and 10 µF plus 100 nF to ground at the chain
AO3401A gate -- 1 MΩ -- BAT+
AO3401A gate -- 2N7002 drain
2N7002 source -- GND
2N7002 gate  -- P0.16
```

P0.16 high turns the chain on. AO3401A (C15127) is basic-library, SOT-23, top view pin 1 gate, pin 2 source, pin 3 drain. It is specified on at 2.5 V gate drive; the cell at 4.2 V gives the gate about 4.2 V when P0.16 is high. The second 2N7002 is the same basic part as the divider switch.

The lamp supply range is 3.7 V to 5.3 V, so the chain is powered from the cell and not from a 5 V boost. A 3.3 V data high still clears a high threshold of about 0.7 times the cell voltage, because the cell stays under 4.2 V. Below a 3.7 V cell the lamps are out of spec and may stay dark. The stored count is unchanged.

Firmware holds P0.02 low whenever the chain is off, including at boot, so the data pin cannot feed an unpowered lamp. Turn the switch on, wait for the rail, send one frame, and drop the data pin low again before turning the switch off. Frames are sent with PWM and DMA, or another method a SoftDevice interrupt cannot split. A bit-banged frame will glitch when the radio runs. Color values stay near `LED_LEVEL` (20 out of 255) so eight lamps do not pull the cell at full pixel current.

A 100 nF capacitor sits on VDD at each lamp. One 10 µF sits on LED_VDD.

P0.09 and P0.10 stay free. They are the NFC pins, and using them as GPIO needs a UICR setting the firmware does not rely on.

The button is the hand-soldered tactile switch from P0.13 to ground. The firmware enables the internal pull-up.

The charge LED is not a ninth count lamp. A reading of zero is the blue opening flash described in [docs/behavior.md](behavior.md).

## Cell

A protected single-cell Li-ion or LiPo, soldered to BAT+ and BAT− pads after assembly. Silkscreen the polarity. The cell is not part of the fab order.

## Bring-up

1. P0.13 reads low while the button is held and high when released.
2. With P0.16 high and a cell above 3.7 V, a frame on P0.02 lights the chain in order. With P0.16 low, the chain is dark and is not a load on the cell.
3. With USB plugged in and a cell on the pads, the charge LED follows CHRG and the cell voltage rises toward 4.2 V.
4. Module VCC stays at or below 3.6 V while the cell is at 4.2 V.
5. With P0.14 low, the divider does not present a DC path from the cell to ground.
6. Plugging in USB-C enumerates the CH340, and a serial upload resets the module into the bootloader.
7. With USB unplugged, P0.12 and P0.15 are not pulled, and the cell is not feeding the CH340.
