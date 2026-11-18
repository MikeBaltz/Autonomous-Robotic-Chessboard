# TMC2209 UART / StallGuard test

Checks the UART wiring to both drivers and helps choose a StallGuard
threshold for sensorless homing.

## Wiring

Power everything off before wiring. Never unplug a motor while 12 V is on.

**BigTreeTech TMC2209 V1.3** (the modules in use): the pin labelled `RX`
is the driver's single-wire UART pin. `TX` is not connected to it (no
continuity between them) and stays unconnected. GPIO6 (ESP32 TX) and GPIO5
(ESP32 RX) are joined on one row; the firmware runs TX as open-drain with
pull-ups, so no resistor is needed (confirmed working on the Y driver):

```
GPIO6 (TX) ──┐
             ├── X RX, Y RX
GPIO5 (RX) ──┘
```

Leave `TX` and `CLK` unconnected.

DIAG is optional: the sketch also detects stalls by reading `SG_RESULT`
over UART.

**Modules with a single `PDN_UART` pin** instead:

| From | To | Notes |
|---|---|---|
| GPIO5 (TX) | 1 kΩ resistor → shared UART line | Resistor required |
| GPIO6 (RX) | shared UART line | Direct |
| Shared UART line | X `PDN_UART` and Y `PDN_UART` | |

**Both module types:**

| From | To | Notes |
|---|---|---|
| X `MS1`, X `MS2` | GND | X address 0 |
| Y `MS1` | 3.3 V | Y address 1 |
| Y `MS2` | GND | |
| X `DIAG` | GPIO7 | HIGH on stall |
| Y `DIAG` | GPIO15 | HIGH on stall |

STEP/DIR/EN, VM, VDD, GND and the 100 µF capacitor are unchanged.

Tying Y's `MS1` high changes its microstepping to 1/32 until firmware sets
it over UART. This sketch sets both drivers to 1/8. The `movement` sketch
does not configure UART yet, so don't use it for the Y axis until it does.

## Run

```sh
pio run -t upload -t monitor -d sketches/tmc_uart_test
```

## Steps

1. **Connection.** At startup it prints both drivers. Expect
   `OK, version 0x21` for X and Y.
   - "all 1s": the ESP32 gets no reply. Check TX/resistor/PDN_UART.
   - "all 0s": check RX, the MS1/MS2 address wiring, VDD and GND.
   - Only Y fails: check Y's `MS1` is on 3.3 V.
2. **Free running.** With the carriage near the middle and the threshold at 0
   (off), send `x+` then `x-`. Jogs stop after 2 s. Note the `SG_RESULT`
   values. Repeat for `y+` / `y-`.
3. **Resisting.** Jog again and gently push against the carriage. `SG_RESULT`
   should drop.
4. **Threshold.** A stall triggers when `SG_RESULT <= 2 × SGTHRS`. Start with
   `SGTHRS` at about a quarter of the free-running value (e.g. free-running
   ~200 → `t 50`), then adjust:
   - stops while running freely → lower it
   - doesn't stop when you hold the carriage → raise it
5. **End stop.** Jog slowly toward one end (`m 15000` to allow a longer jog)
   and check it stops on contact. Try a couple of speeds with `f N`; the
   speed that detects stalls most reliably is the one homing will use.

Record the final speed, SGTHRS and free-running SG_RESULT for each axis.
The homing routine needs them.
