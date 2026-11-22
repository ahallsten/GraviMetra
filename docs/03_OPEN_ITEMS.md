# 03 — Open items

| # | Item | Status | Needed by | Owner |
|---|---|---|---|---|
| OI-01 | ~~Display variant~~ → RESOLVED by D-034 (MCUFRIEND RM68140 8-bit parallel shield). Verify the controller ID against the panel at bring-up | CLOSED | — | — |
| OI-02 | Span tempco closure strategy at full coil load (counterweight preload, compensated SmCo grade, auto-adjust, characterization) | UNRESOLVED · top risk | Gate B | Phase 2 |
| OI-03 | MCU: STM32G491RET6 stock (0 at Digi-Key, 53 at LCSC); evaluate G474 (HRTIM) for the PWM-measurand option | UNRESOLVED | Gate B | Phase 2 |
| OI-04 | Coil driver: OPA593 out of stock everywhere; choose an alternate architecture | UNRESOLVED | Gate B | Phase 2 |
| OI-05 | Reference (LTC6655LNB-2.5 candidate) and shunt (VCS1625Z drop-in, stock unknown) | UNRESOLVED | Gate D | Phase 2/4 |
| OI-06 | TMC2209-LA availability (21 at LCSC) | UNRESOLVED | Gate D | Phase 4 |
| OI-07 | Internal check-mass value and actuator | UNRESOLVED | Gate B | Phase 2 |
| OI-08 | Board count and partition (mains / controller+LV power / EMFC module) | UNRESOLVED | Gate B | Phase 2 |
| OI-09 | Power budget and Mean Well IRM derating; verify the IRM datasheets | UNRESOLVED | Gate B | Phase 2 |
| OI-10 | Verify Torbal AD520-specific manual specs (only the AD-series manual was reachable; it mentions internal auto-cal) | UNRESOLVED | Gate A note | Phase 2 |
| OI-11 | ~~Masses plan~~ → CONFIRMED D-030 | CLOSED | — | — |
| OI-12 | RM68140 8080-parallel write timing; LVGL buffer sizing on 112 KB SRAM | UNRESOLVED | Gate B | Phase 2 |
| OI-13 | FMEA | RELEASE BLOCKER | Gate F | Phase 6 |
| OI-14 | Trickler mechanics for long-kernel extruded powder (tube ID, internal features, speed range) | UNRESOLVED | Gate B | Phase 2 |
| OI-15 | Counterweight sizing against the 520 g range: fixed tare vs mid-range preload with bipolar coil | UNRESOLVED | Gate B | Phase 2 |
| OI-16 | Eccentric-load target basis (MET-09 currently equals linearity) | PROPOSED | Gate B | Phase 2 |
| OI-17 | Which bought balance to buy for Config S bring-up (see `phase0/SCALE_SEARCH.md`) | UNRESOLVED | Phase 3 | User |
| OI-18 | MCU pin budget with an 8-bit parallel display (~13 GPIO + 2 ADC) plus 3 TMC2209 UARTs, RS-232, SPI (ADS1262), I2C, USB, SWD, safety I/O; LQFP64 vs a larger package | UNRESOLVED | Gate B | Phase 2 |
| OI-19 | Home repo migration: branch `v2-restructure` prepared; needs push + PR merge (GitHub not yet linked to Claude) | IN PROGRESS | Before Phase 2 work lands | User/Claude |
| OI-20 | GraviMetra firmware deltas: 520 g, 3 stages, Nextion → RM68140/LVGL, Config S scale drivers, SmCo multi-sensor compensation | UNRESOLVED | Phase 4 | Phase 4 |
