# 51 单片机多外设联动控制系统

> An 8051 course project that connects light sensing, motor control, traffic signals, flowing LEDs, RGB lighting and UART interaction.

## What it actually does

This is **not** a temperature-detection project. The firmware reads analog light intensity via a PCF8591 ADC and uses the value to change the direction of a four-phase stepper motor. In parallel, the 51 MCU runs timed traffic-light states, configurable flowing LEDs, a button-controlled RGB indicator and a serial command interface.

## Features

- PCF8591 I²C analog acquisition on AIN0
- Light-threshold motor-direction control
- Four-state traffic-light timing sequence
- Four UART modes for LED direction and speed (`0`–`3`)
- RGB color cycling with a local push button
- 1 ms Timer0 scheduler for concurrent periodic tasks
- Proteus project for circuit-level simulation

## Hardware map

| Peripheral | Interface / pins |
| --- | --- |
| PCF8591 | Software I²C on P0.6 / P0.7 |
| Stepper motor | P0.0–P0.3 |
| Traffic LEDs | P1.0–P1.5 |
| Flow LEDs | P2.0–P2.7 |
| RGB LED | P3.5–P3.7 |
| RGB button | P3.2 |
| Serial | 9600 bps @ 11.0592 MHz |

## Run it

1. Open `firmware/TEST14.uvproj` with Keil µVision for 8051.
2. Build and flash the target (or load the produced HEX into a compatible simulator).
3. Open `proteus/controller.pdsprj` in Proteus to inspect or simulate the circuit.
4. Send ASCII `0`, `1`, `2`, or `3` via serial to choose a flow-LED behavior.

## Layout

```text
firmware/main.c          8051 application firmware
firmware/TEST14.uvproj   Keil project
proteus/controller.pdsprj  Proteus schematic/simulation project
```

## Course-project note

This repository is an educational hardware project. Pin assignments, thresholds and timing values are intentionally simple and can be adapted for a different development board.
