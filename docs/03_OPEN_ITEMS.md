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
| OI-19 | Home repo migration: `v2-restructure` pushed; Claude opens the PR to `main` (D-049); **user merges** | IN PROGRESS | Before Phase 2 work lands | User |
| OI-20 | GraviMetra firmware deltas: 520 g, 3 stages, Nextion → RM68140/LVGL, Config S scale drivers, SmCo multi-sensor compensation | UNRESOLVED | Phase 4 | Phase 4 |
| OI-21 | Display shield: user reports it was driven by an **Arduino Uno (5 V)** and carries **2× 74HC245**. If the 245s run on 5 V, 3.3 V drive is below HC V_IH (≈3.5 V at 5 V) and a read puts 5 V on the bus. PE7 (LCD_D7 in the candidate map) is TT_a (3.6 V max). **Next check (user, DMM, shield on the Uno):** voltage on pin 20 (VCC) of each 245; what pin 1 (DIR) and pin 19 (OE) connect to. If VCC = 5 V, B3 gets a level translator with OE for the touch phase (Phase 4) | IN PROGRESS | Gate B | User |
| OI-22 | ~~Long-kernel tolerance contradiction~~ → RESOLVED by D-048 (per-profile tol + warning < 1.1 × kernel mass) | CLOSED | — | — |
| OI-23 | Powder measurements per powder used: kernel d × L (20 kernels), mass of 100 kernels, bulk density (10 cm³ scoop). Replaces the ASSUMED inputs in `analysis/trickler_sizing.py` | UNRESOLVED | Gate C | User |
