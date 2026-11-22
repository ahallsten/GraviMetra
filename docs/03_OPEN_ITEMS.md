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
| OI-08 | Board count and partition: 4 boards PROPOSED (D-042), B2/B3 merge open | PROPOSED | Gate B | Phase 2 |
| OI-09 | Power budget and Mean Well IRM derating; verify the IRM datasheets | UNRESOLVED | Gate B | Phase 2 |
| OI-10 | Verify Torbal AD520-specific manual specs (only the AD-series manual was reachable; it mentions internal auto-cal) | UNRESOLVED | Gate A note | Phase 2 |
| OI-11 | ~~Masses plan~~ → CONFIRMED D-030 | CLOSED | — | — |
| OI-12 | RM68140 8080-parallel write timing; LVGL buffer sizing on 112 KB SRAM | UNRESOLVED | Gate B | Phase 2 |
| OI-13 | FMEA | RELEASE BLOCKER | Gate F | Phase 6 |
| OI-14 | Trickler mechanics: concept PROPOSED (D-047, `docs/phase2/TRICKLER_CONCEPT.md`); tube speeds rest on an ASSUMED η·φ, to be measured on the Phase 3 rig | PROPOSED | Gate B (concept) / Gate C (measured) | Phase 2/3 |
| OI-15 | Counterweight sizing against the 520 g range: fixed tare vs mid-range preload with bipolar coil | UNRESOLVED | Gate B | Phase 2 |
| OI-16 | Eccentric-load target basis (MET-09 currently equals linearity) | PROPOSED | Gate B | Phase 2 |
| OI-17 | Which bought balance to buy for Config S bring-up (see `phase0/SCALE_SEARCH.md`) | UNRESOLVED | Phase 3 | User |
| OI-18 | MCU pin budget: done (`docs/phase2/CONTROLLER_IO.md`). LQFP64 −11 for S+E → LQFP100 PROPOSED (D-040); CubeMX confirmation of the candidate map pending | PROPOSED | Gate B | Phase 2 |
| OI-19 | Home repo migration: branch `v2-restructure` prepared; needs push + PR merge (GitHub not yet linked to Claude) | IN PROGRESS | Before Phase 2 work lands | User/Claude |
| OI-20 | GraviMetra firmware deltas: 520 g, 3 stages, Nextion → RM68140/LVGL, Config S scale drivers, SmCo multi-sensor compensation | UNRESOLVED | Phase 4 | Phase 4 |
| OI-21 | Display shield logic level and touch pin mapping: buffer chip on the shield (none / 74HC245 at 5 V / LVC245), and which MCU drove it in Kaboom-Konveyor. Decides whether the display carrier needs a level translator | UNRESOLVED | Gate B | User |
| OI-22 | **Contradiction:** a long extruded kernel (≈1.6–2.5 mg, ASSUMED) exceeds the +0.02 gr window, so DSP-04's default tol and DSP-05's ≤10 % extruded overthrow can't both hold (P(in window) 52–81 %). Recommended: per-profile tol with a warning below 1.1 × kernel mass | UNRESOLVED | Gate B | User |
| OI-23 | Powder measurements per powder used: kernel d × L (20 kernels), mass of 100 kernels, bulk density (10 cm³ scoop). Replaces the ASSUMED inputs in `analysis/trickler_sizing.py` | UNRESOLVED | Gate C | User |
