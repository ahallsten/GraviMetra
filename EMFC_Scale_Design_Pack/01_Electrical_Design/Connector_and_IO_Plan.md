# Connector and I/O Plan

## Controller board external connectors

- USB-C: USB2 device, service/logging/firmware.
- Nextion: 4-pin keyed header, +5V/GND/TX/RX.
- CAN: 3-position pluggable terminal, CANH/CANL/GND; optional shield/chassis terminal nearby.
- RS-485: 3-position pluggable terminal, A/B/GND; optional shield/chassis terminal nearby.
- Four motor outputs: four 4-position locking/pluggable connectors for A1/A2/B1/B2.
- Four TMC2209 module sockets.
- E-stop loop input and monitored contact input.
- Rear power-control/status connections as needed.
- Battery connector: protected 2S pack, keyed and current-rated.
- Precision-board interconnect: shielded digital/power connector.
- Calibration actuator expansion connector.
- Calibration limit/home sensor connector.
- SWD header.

## Precision-board interconnect

Send digital data, not microvolt analog measurement signals, between boards.

Suggested signals:

- clean/raw power input(s) as finalized,
- 3V3 logic,
- ground returns,
- SPI SCK/MOSI/MISO/CS for ADS1262,
- ADS1262 DRDY,
- ADC RESET/START if controlled remotely,
- I2C for TMP117 sensors if MCU owns the bus,
- OPA593 thermal/current-limit status,
- analog-servo enable/reset control,
- optical diagnostic ADC signals only if they are digitized on the controller board (prefer digitizing locally if possible).

Use a shielded cable and chassis-terminate the shield according to EMC testing. Keep cable length roughly 6-18 inches as planned.

## UART allocation concept - STM32G491

Exact peripheral/pin mapping must be finalized in STM32CubeMX before schematic capture.

Required serial channels:

1. TMC2209 #1 UART
2. TMC2209 #2 UART
3. TMC2209 #3 UART
4. TMC2209 #4 UART
5. Nextion UART
6. RS-485 UART

CAN uses FDCAN, not a UART. USB uses the USB peripheral. ADS1262 uses SPI. TMP117 uses I2C.

Use hardware timers/GPIO for STEP pulses rather than bit-banged delays.
