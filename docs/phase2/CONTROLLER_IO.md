# Controller I/O list and MCU pin budget (Phase 2, OI-18)

Rev 0 · 2026-11-21 · Source of numbers: `analysis/pin_budget.py` → `analysis/pin_budget.out.txt`; pin data extracted from DS13122 Rev 4, Tables 12–13 (`analysis/tools/extract_g4_pins.py`).

## Result

| Package (STM32G491) | Usable I/O* | Config S only (39) | S + E (54) | S + E + Rev-B reserved (61) |
|---|---|---|---|---|
| LQFP64 (RET6, current FW-04 candidate) | 43 | fits, **+4** | **−11** | −18 |
| LQFP80 (MET6) | 57 | +18 | +3 | −4 |
| LQFP100 (VET6) | 77 | +38 | +23 | **+16** (solved map below) |

\*After reserving USB (PA11/12), SWD (PA13/14), SWO (PB3), HSE crystal (PF0/1), BOOT0 (PB8) and NRST.

**Contradiction:** FW-04 lists STM32G491**RET6** (LQFP64), but LQFP64 can't carry Config E's interface alongside the parallel display (D-034) without an I/O expander. Either the MCU package changes or the I/O does.

**Recommendation (PROPOSED for Gate B): move to LQFP100, STM32G491VET6.** It uses the same die, flash, SRAM and firmware, and leaves 16 spare pins after every Rev-B net. Why: the display alone takes 13 pins, and an I/O expander would put safety and actuator lines behind an I²C bus, which conflicts with SAF-03 (defined pin states through reset or a crash). G474VET6 stays open as the HRTIM alternative until the measurand study closes OI-03. Pin compatibility between G474 and G491 in LQFP100 is [U] until checked in CubeMX.
- Rejected: LQFP80 has only +3 spare pins for Config E and can't carry the Rev-B nets.
- Rejected: LQFP64 plus an expander.

**Stock: UNVERIFIED.** On 2026-11-21 LCSC and JLC returned no readable data (JS pages, or "0 found" even for parts LCSC stocks), and Octopart returned 403. The Digi-Key search cards showed prices but no stock:

| MPN | Price on the card (summary only) |
|---|---|
| G491VET6 | $10.95 |
| G491RET6 | $9.89 |
| G474VET6 | $12.13 |
| G491MET6 | $10.84, stock 999 |

The sourcing check (OI-03) has to be redone by a person or with the sourcing agent before Gate B.

## Design rules used (each is checked by the script)

| Rule | Why | Source |
|---|---|---|
| LCD D0–D7 on 8 consecutive bits of one port (PE0–PE7) | One BSRR write per byte. LVGL flush speed (OI-12) | Design choice |
| LCD WR, RS and CS on ADC pins | 4-wire touch shares WR and RS (analog) with D6/D7 or D0/D1. CS covers shield variants | MCUFRIEND touch convention; verify on the shield |
| Each STEP and buzzer on its own **timer instance** | Channels of one timer share its period, which would couple the frequencies | RM0440 timer architecture |
| STEP from timer outputs only | CLAUDE.md firmware invariant | — |
| Actuator outputs not on PA13/PA14/PA15/PB4 | These have pull-up or pull-down during and after reset, before firmware runs | RM0440 §9.3.1 ("debug pins … PA15 JTDI pull-up, PA14 pull-down, PA13 pull-up, PB4 NJTRST pull-up") |
| No plain GPIO or outputs on PB4/PB6. Firmware sets `PWR_CR3.UCPD1_DBDIS` first thing | UCPD dead-battery Rd (5.1 kΩ pull-down) is active on CC1/CC2 after reset, gated by DBCC pins PA9/PA10 | RM0440 PWR_CR3 bit 14; §46.4.6 |
| PC13–PC15 only for LED or slow inputs | Supplied through the backup power switch, limited sink current | DS13122 Table 12 note 2 |
| PB8 kept as a BOOT0 strap (pull-down + test pad) | Allows ROM-bootloader recovery | RM0440 §2.6 |
| Interrupt inputs on distinct EXTI line numbers | EXTI line n is shared by every PxN | RM0440 EXTI |
| Every output has an external pull to its safe level | All GPIO are high-Z (analog) through reset | RM0440 §9.3.1; SAF-03 |

## Function list (tiers: 1 = Rev A Config S, 2 = Config E module, 3 = Rev-B reserved/DNP)

| Group | Signals | Tier | Notes |
|---|---|---|---|
| HMI display | D0–D7, RD, WR*, RS*, CS*, RST (*ADC) | 1 | D-034 shield; touch read through WR/RS ADC |
| Steppers | STEP1–3 (timer), DIR (shared), SEL0, SEL1, EN_REQ, TMC_UART (1 bus), TMC_DIAG (3 OR-ed) | 1 | See "Motor channel scheme" |
| Scale RS-232 | TX, RX, RTS, CTS (one U(S)ART) | 1 | RTS/CTS fitted but unused for A&D (no handshake needed) |
| USB device | DM, DP (fixed), VBUS_SENSE | 1 | CC pins get 5.1 kΩ Rd resistors; UCPD not used |
| Temperature | I2C SCL/SDA | 1 | TMP117 ambient (controller) plus magnet/coil (EMFC module) on one bus, 4 addresses available |
| Safety | ESTOP_MON, LID_MON, SAFE_CHAIN_OK (EXTI); WDG_KICK; VM_SENSE (ADC) | 1 | Monitoring only; hardware does the cutting (SAF-01) |
| Housekeeping | V12, V5, BOARD_NTC (ADC); BUZZER (timer); STATUS_LED | 1 | 3V3 via VREFINT |
| ADS1262 | SPI SCK/MISO/MOSI, CS, DRDY (EXTI), START, RESET | 2 | Digital only crosses to the EMFC module |
| EMFC control | COIL_EN, COIL_FAULT (EXTI), SERVO_MODE, POS_MON (ADC), EMFC_DAC, CM_STEP (timer), CM_DIR, CM_HOME (EXTI) | 2 | **Placeholder until the servo study**; check-mass actuator per OI-07 |
| Optional | PAN_DET | 3 | Pan presence is detected from weight by default (both configs) |
| Rev-B reserved | CAN TX/RX, RS-485 TX/RX/DE, BATT_SENSE | 3 | IF-03: footprints only. No protocol (CLAUDE.md) |

### Motor channel scheme (PROPOSED, closes the hardware part of SAF-04)

- **EN path.** `EN_REQ` and the hardware `SAFE_CHAIN_OK` go into a 2-to-4 decoder (74LVC1G139 class) enable. `SEL1:SEL0` picks the channel, so **at most one TMC2209 ENN can be low**, whatever the firmware does. Code 3 selects no driver. Why: hardware one-motor-at-a-time costs the same three pins as three separate EN lines.
- **DIR.** One DIR line is shared. Only one motor moves at a time, so a separate DIR per channel buys nothing.
- **STEP.** STEP stays per channel on separate timers. Why: it keeps the firmware's per-channel `StepPulseTimer` model, and a disabled driver never sees steps.
- **UART.** One single-wire UART bus (USART half-duplex, 1 kΩ to each PDN_UART). Addresses 0, 1, 2 are set by MS1/MS2 straps. Source: TMC2209 DS Rev 1.08 §4, "slave address … MS1 (bit 0) and MS2 (bit 1) in the range 0 to 3".
- **DIAG.** DIAG is a push-pull, active-high output that clears when ENN goes high (DS pin table). The three outputs are OR-ed (74LVC1G332 class) into one EXTI. The firmware reads `DRV_STATUS` over UART to see which channel and what fault.

## Candidate LQFP100 map (CANDIDATE, solver output; confirm in CubeMX before schematic capture)

Peripherals: USART2 = scale RS-232, USART3 = TMC bus, I2C1, SPI1 = ADS1262, USART1 = RS-485 (DNP), FDCAN1 (DNP). Timers: TIM1/TIM2/TIM3 = STEP1–3, TIM8 = buzzer, TIM20 = check-mass step. The full table is in `analysis/pin_budget.out.txt`. Spare pins: PB1 PB2 PB6 PB14 PB15 PC0–PC3 PC15 PD12–PD14 PE9 PE11 PE13.

Points the CubeMX pass must check:
- DMA request mapping (DMAMUX makes this flexible).
- That the TIM20 and TIM8 choices don't collide with the clock plan.
- That PA9/PA10 (RS-485, DNP) don't wake UCPD dead-battery behaviour before `DBDIS`.

## Open points raised here

1. **Shield logic level [UNRESOLVED, user].** MCUFRIEND-style shields differ. Some have no buffer, some a 74HC245 on 5 V, which is marginal at 3.3 V drive (V_IH ≈ 3.5 V at 5 V), and some an LVC245 or a 3.3 V regulator. Kaboom-Konveyor isn't on this machine. **Ask:** the chip markings on the back of the shield, and which MCU board Kaboom-Konveyor drove it with. Fallback: a 74LVC8T245 level-translator footprint on the display carrier.
2. **OI-03 stock** for G491VET6 and G474VET6: re-check before Gate B.
3. **EMFC control lines** are placeholders until the servo study fixes analog / digital / hybrid.
