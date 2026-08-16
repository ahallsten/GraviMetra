# STM32CubeMX Project Setup Guide - Gravimetra AGD-250

## Purpose

This guide creates the STM32CubeMX reference project for the **Gravimetra AGD-250 controller board** using an **STM32G491RET6 in LQFP64**.

Use the generated `.ioc` file as the authoritative source for MCU pin assignments before completing the Altium schematic. The firmware project may later be built in PlatformIO, but the CubeMX project should remain under source control so pin, clock, and peripheral changes are documented.

This baseline was checked against:

- STM32CubeMX 6.18 documentation
- STM32G491RE datasheet, revision 4
- STM32G4 reference manual RM0440

Official references:

- https://www.st.com/en/development-tools/stm32cubemx.html
- https://www.st.com/en/microcontrollers-microprocessors/stm32g491re.html
- https://www.st.com/resource/en/datasheet/stm32g491re.pdf
- https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

---

## 1. Install the correct ST software

1. Install the current **STM32CubeMX**, not STM32CubeMX2. The STM32G4 family uses the HAL1/CubeMX workflow.
2. Open CubeMX.
3. Select **Help -> Manage embedded software packages**.
4. Install the current **STM32Cube MCU Package for STM32G4 Series**.
5. Restart CubeMX after installation.

---

## 2. Create the MCU project

1. Select **File -> New Project**.
2. Open the **MCU Selector**.
3. Search for:

   `STM32G491RET6`

4. Verify:
   - Family: STM32G4
   - Flash: 512 KB
   - Package: LQFP64
   - Maximum CPU clock: 170 MHz
5. Select the part and click **Start Project**.
6. Save the project immediately as:

   `Gravimetra_AGD250_Controller.ioc`

Recommended location:

```text
Gravimetra_AGD250/
  Firmware/
    CubeMX/
      Gravimetra_AGD250_Controller.ioc
```

---

## 3. Configure system and debug

### SYS

Open **System Core -> SYS**:

- Debug: **Serial Wire**
- Timebase source: **SysTick**

This preserves:

- PA13 = SWDIO
- PA14 = SWCLK
- PG10 = NRST

Using Serial Wire instead of full JTAG frees PA15, PB3, and PB4 for normal GPIO.

### RCC

Open **System Core -> RCC**:

- HSE: **Disabled**
- LSE: **Disabled** for Revision A
- HSI16: enabled by the clock configuration
- HSI48: enabled for USB

The first board revision uses the internal oscillators. Leave optional crystal footprints in Altium if desired, but do not assign PF0/PF1 or PC14/PC15 to oscillators in this CubeMX baseline.

### BOOT0 warning

PB8 is also the BOOT0 pin on the LQFP64 device. Do **not** use PB8 for CAN or another externally driven signal.

In the schematic:

- add a BOOT0 pull-down,
- provide a test pad or jumper,
- keep PB8 reserved unless its reset-time behavior is fully controlled.

---

## 4. Configure the 170 MHz clock tree

Open **Clock Configuration** and set:

| Setting | Value |
|---|---:|
| PLL source | HSI16 |
| PLLM | /4 |
| PLLN | x85 |
| PLLR | /2 |
| SYSCLK source | PLLCLK |
| SYSCLK | 170 MHz |
| HCLK | 170 MHz |
| APB1 | 170 MHz |
| APB2 | 170 MHz |
| USB clock | HSI48, 48 MHz |
| FDCAN clock | PCLK1, 170 MHz |

CubeMX should automatically select the required voltage-scaling/boost setting and flash latency.

Enable **System Core -> CRS**:

- Synchronization source: **USB SOF**
- Frequency error counter source: **HSI48**
- Leave the remaining values at CubeMX defaults initially.

The intended clock arrangement is:

```text
HSI16 -> PLL -> 170 MHz system clock
HSI48 -> USB FS clock
USB SOF -> CRS trimming of HSI48
```

---

## 5. Assign the primary peripheral pins

Use this as the Revision-A pin map. In CubeMX, click the required pin if the automatic assignment selects a different alternate location.

### Communications and scale interface

| Function | Peripheral | Pin(s) | CubeMX mode |
|---|---|---|---|
| Nextion HMI | USART1 | PB6 TX, PB7 RX | Asynchronous |
| TMC2209 channel 1 UART | USART2 | PA2 | Single Wire / Half-Duplex |
| TMC2209 channel 2 UART | USART3 | PB10 | Single Wire / Half-Duplex |
| TMC2209 channel 3 UART | UART4 | PC10 | Single Wire / Half-Duplex |
| TMC2209 channel 4 UART | UART5 | PC12 | Single Wire / Half-Duplex |
| RS-485 | LPUART1 | PC1 TX, PC0 RX | Asynchronous |
| RS-485 driver enable | GPIO | PC2 | Output, initially low |
| CAN | FDCAN2 | PB13 TX, PB12 RX | FDCAN2 |
| USB service port | USB FS | PA12 D+, PA11 D- | Device only |
| USB VBUS detect | GPIO | PA9 | Input, no pull |
| ADS1262 SPI | SPI1 | PA5 SCK, PA6 MISO, PA7 MOSI | Full-duplex master |
| ADS1262 chip select | GPIO | PA4 | Output, initially high |
| ADS1262 DRDY | EXTI GPIO | PC7 | Falling-edge interrupt |
| ADS1262 RESET | GPIO | PC11 | Output |
| ADS1262 START | GPIO | PD2 | Output |
| TMP117 / power-management bus | I2C3 | PC8 SCL, PC9 SDA | I2C |

### Four independent step outputs

| Motor | Timer channel | Pin |
|---|---|---|
| Auger 1 STEP | TIM2_CH1 | PA0 |
| Auger 2 STEP | TIM3_CH1 | PC6 |
| Auger 3 STEP | TIM15_CH1 | PB14 |
| Auger 4 STEP | TIM20_CH1 | PB2 |

### Direction and individual enable GPIOs

| Motor | DIR pin | EN pin |
|---|---|---|
| Auger 1 | PA1 | PA3 |
| Auger 2 | PA10 | PA15 |
| Auger 3 | PB0 | PB1 |
| Auger 4 | PB3 | PB4 |

TMC2209 EN is normally active-low. Configure all four EN pins to power up **high**, so every driver is disabled until firmware deliberately enables one.

### Safety, calibration, and diagnostics

| Signal | Suggested pin | Mode |
|---|---|---|
| ESTOP_SENSE | PA8 | Input, no pull; hardware circuit defines the level |
| GLOBAL_MOTION_ENABLE | PB5 | Output, initially inactive |
| BATTERY_SENSE | PC3 / ADC1_IN9 | Analog ADC input |
| CAL_ACTUATOR_ENABLE | PC4 | Output, initially inactive |
| CAL_HOME | PC5 | Input with EXTI if needed |
| CAL_END | PB11 | Input with EXTI if needed |
| EMFC_SERVO_RESET | PF0 | Output; active level to match schematic |
| OPA593_CURRENT_LIMIT | PF1 | Input |
| OPA593_THERMAL_WARNING | PC13 | Input |
| CHARGER_INTERRUPT | PB15 | Input / EXTI if supported by charger |

Do not assign PB8. Reserve PC14 and PC15 for a future 32.768 kHz crystal if desired.

---

## 6. Configure each peripheral

### SPI1 - ADS1262

Set:

- Mode: **Full-Duplex Master**
- Frame format: Motorola
- Data size: 8 bits
- First bit: MSB first
- Clock polarity: Low
- Clock phase: **Second edge**
- NSS: Software
- Baud-rate prescaler: **32** initially

This gives an SPI clock of about 5.3 MHz at a 170 MHz peripheral clock and implements SPI mode 1, which the ADS1262 uses.

### I2C3 - TMP117 and power-management devices

Set:

- I2C speed: **400 kHz Fast Mode**
- Addressing: 7-bit
- Analog filter: enabled
- Digital filter: 0 initially

The physical schematic must include the required pull-up resistors to 3.3 V. CubeMX does not add them.

### USART1 - Nextion

Set:

- Asynchronous
- 8 data bits
- No parity
- 1 stop bit
- No hardware flow control
- 115200 baud target

A factory-default Nextion may initially use 9600 baud. Either configure the display project for 115200 or temporarily change USART1 to 9600 during first bring-up.

### USART2, USART3, UART4, UART5 - TMC2209 modules

For each peripheral:

- Mode: **Single Wire / Half-Duplex**
- 8 data bits
- No parity
- 1 stop bit
- 115200 baud
- No hardware flow control

Each driver has its own UART peripheral, so all four modules may use the same TMC UART address. The carrier-board schematic still needs the correct BTT-module PDN_UART connection and any required series resistor.

### LPUART1 - RS-485

Set:

- Asynchronous
- 8 data bits
- No parity
- 1 stop bit
- 115200 baud initially
- No hardware flow control

PC2 is the separate DE/RE control output for the external THVD1450-class transceiver. Initialize PC2 low so the transceiver starts in receive mode.

### FDCAN2 - CAN bring-up configuration

Use Classic CAN first:

- Mode: Normal
- Frame format: Classic CAN
- Nominal prescaler: 20
- Nominal time segment 1: 13
- Nominal time segment 2: 3
- Nominal sync jump width: 3

At a 170 MHz FDCAN kernel clock, this produces 500 kbit/s with an approximately 82.4% sample point.

Enable the FDCAN2 interrupt. Remember that the STM32 contains the FDCAN controller only; the board still requires the external CAN transceiver and bus termination strategy.

### USB FS and USB Device middleware

1. Enable **USB -> Device Only**.
2. Enable **Middleware -> USB_DEVICE**.
3. Select **Communication Device Class (CDC)**.
4. Select HSI48 as the USB clock source.
5. Keep hardware VBUS sensing disabled in the USB block if CubeMX presents that option.
6. Use PA9 as the external `USB_VBUS_SENSE` GPIO through the schematic's resistor divider.

The machine is self-powered. USB VBUS is for host-presence detection only; it must not power the machine.

### Step timers

For TIM2, TIM3, TIM15, and TIM20:

- Clock source: Internal Clock
- Channel 1: PWM Generation CH1
- Prescaler: 169
- Counter period: 999 initially
- Pulse: 0 initially
- Polarity: High

This creates a 1 MHz timer tick. Firmware will change the period and pulse values for the requested step rate. Do not start PWM automatically during initialization.

### ADC1 - battery monitoring

Enable the PC3 analog channel:

- Channel: ADC1_IN9
- Resolution: 12 bit
- Conversion mode: single conversion initially
- Trigger: software
- Sampling time: choose a long sampling interval, such as 247.5 cycles, because the battery-divider source impedance may be relatively high.

### CRC

Enable the hardware CRC peripheral for configuration/calibration-data integrity checks.

### Watchdog

Leave IWDG disabled during the very first hardware bring-up. Enable it later, after the firmware health-supervisor path is operating and can refresh it safely.

### RTOS

Do not enable FreeRTOS in Revision A. The planned firmware uses interrupt-driven drivers plus deterministic state machines unless a later design review selects an RTOS.

---

## 7. Configure GPIO startup safety

In **System Core -> GPIO**, verify these initial output states:

| Signal group | Startup state |
|---|---|
| TMC1_EN through TMC4_EN | High - all drivers disabled |
| TMC DIR pins | Low |
| ADS1262 CS | High - deselected |
| RS485_DE | Low - receive mode |
| GLOBAL_MOTION_ENABLE | Inactive |
| CAL_ACTUATOR_ENABLE | Inactive |
| Step timer outputs | Timers not started |

For `EMFC_SERVO_RESET`, use the safe startup level defined by the final analog schematic. Do not guess its active polarity in firmware.

Set unused GPIOs to analog/no-pull only after all reserved future signals have been labeled.

---

## 8. Configure interrupts

Enable at minimum:

- ADS1262 DRDY EXTI interrupt
- USB interrupt
- FDCAN2 interrupt
- USART1 interrupt or DMA for Nextion receive
- LPUART1 interrupt for RS-485 receive
- I2C3 error interrupt
- calibration-sensor EXTI interrupts if used
- charger interrupt EXTI if used

Suggested priority order, from highest importance to lowest:

1. ADS1262 DRDY
2. active step-timer service
3. E-stop and hard fault inputs
4. USB
5. FDCAN
6. Nextion and RS-485
7. I2C and background diagnostics

Final numerical priorities belong in the firmware design review. The hardware E-stop must remove motion independently of all interrupt priorities and firmware execution.

---

## 9. Configure code generation

Open **Project Manager**.

### Project

- Project name: `Gravimetra_AGD250_Controller`
- Toolchain / IDE: **STM32CubeIDE**
- Firmware package: current installed STM32CubeG4 version

STM32CubeIDE is used as the official generated reference project. Codex may later integrate the generated HAL initialization into the PlatformIO target.

### Code Generator

Enable:

- Keep user code when re-generating
- Generate peripheral initialization as separate `.c/.h` files
- Copy only the necessary library files
- Generate a backup before re-generating if CubeMX offers the option

Do not place substantial application logic inside generated files. Keep application code in separate source modules.

---

## 10. Generate and verify

1. Click **Generate Code**.
2. Resolve every red error and yellow pin/clock conflict before continuing.
3. Confirm CubeMX shows all six serial peripherals simultaneously:
   - USART1
   - USART2
   - USART3
   - UART4
   - UART5
   - LPUART1
4. Confirm USB remains on PA11/PA12.
5. Confirm CAN is FDCAN2 on PB12/PB13, not PB8/PB9.
6. Confirm PB8 remains reserved for BOOT0.
7. Confirm SWD remains on PA13/PA14.
8. Save the `.ioc` file.
9. Commit the `.ioc` file to source control before copying the pin assignments into Altium.

The Altium MCU sheet and firmware pin header should both be generated from this same pin map. Any later CubeMX pin change must be treated as an electrical-design change and reviewed against the PCB schematic before code is regenerated.

---

## Revision-A completion checklist

- [ ] Correct MCU: STM32G491RET6, LQFP64
- [ ] SYS debug set to Serial Wire
- [ ] 170 MHz system clock from HSI16 + PLL
- [ ] HSI48 + CRS configured for USB
- [ ] USB CDC enabled
- [ ] ADS1262 SPI1 configured for mode 1
- [ ] I2C3 configured for TMP117/power devices
- [ ] Four TMC UARTs configured in half-duplex mode
- [ ] Nextion USART configured
- [ ] RS-485 LPUART configured
- [ ] FDCAN2 configured on PB12/PB13
- [ ] Four independent STEP timer outputs configured
- [ ] All TMC EN pins default high/disabled
- [ ] PB8 reserved for BOOT0
- [ ] SWD and NRST preserved
- [ ] No pin or clock conflicts remain
- [ ] `.ioc` file saved and committed
