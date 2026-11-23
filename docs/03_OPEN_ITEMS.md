# 03 — Open items

| # | Item | Status | Needed by | Owner |
|---|---|---|---|---|
| OI-01 | ~~Display variant~~ → RESOLVED by D-034 (MCUFRIEND RM68140 8-bit parallel shield). Verify the controller ID against the panel at bring-up | CLOSED | — | — |
| OI-02 | Span tempco closure at full coil load. **Closed by analysis for the operating spec (D-071):** standard Sm2Co17 + in-situ span model + auto-adjust at 0.5 K → 0.93 mg at 520 g (k = 2). MET-06 static only after ≥ 10 K of history, a B4 heat test and the coil set ±20 µm (OI-38). TC magnet stays the fallback. Remaining: rig span sweep 15–30 °C (α, its curvature, lag τ, non-tracking fraction) | UNRESOLVED (rig confirmation) | Gate C | Phase 3 rig |
| OI-03 | MCU stock: STM32G491VET6 is Active but scarce. JLC has 17 (C3209873, Extended, ~$7.50; unofficial mirror data), Digi-Key 0 with a 52-week lead time, ST eStore out of stock. **Reserve 4 at JLC now.** Fallback: STM32G474VET6TR/VET3 (50/56 at JLC) after a CubeMX pin re-map. HRTIM question closed by D-069 | UNRESOLVED (stock risk; user action) | Gate B | User + Phase 2 |
| OI-04 | ~~Coil driver: OPA593 out of stock~~ → RESOLVED by D-066: a zero-drift op-amp drives the 10–12.5 mA coil directly; no power stage | CLOSED (PROPOSED design) | — | — |
| OI-05 | Measurand parts:
- ADS1263IPWR: 74 at JLC (C2861295), 2,614 at Digi-Key.
- LTC6655BHMS8-2.5: 0 at JLC, 980 at Digi-Key → buy and consign; the LS8 package has no stock until May 2027.
- OPA2388: 16k at JLC.
- Shunt: **VPG Z201 150 Ω ±0.01 % (Y1453150R000T9L)**, through-hole, or FRSM1206 as the SMD alternative. VSMP/VFCP are end-of-life (PCN FR-005-2024). "TCR ≤ 0.2 ppm/K" is a typical value; the worst case is ±0.6 ppm/K, covered by the B4 heat test and auto-adjust (D-072).

Final part numbers are fixed in Phase 4 | UNRESOLVED | Gate D | Phase 4 |
| OI-06 | ~~TMC2209-LA availability~~ → RESOLVED: the "21 at LCSC" was the tray part number. The reel part TMC2209-LA-T (C2150710) has ~24k at JLC and ~27k at Digi-Key | CLOSED | — | — |
| OI-07 | Internal check mass: 200 g on a receptor kinematic seat, geared-stepper lift (D-052) The lift puts ~5.5 J into the cell zone just before the span reading: mount the motor off the cell base and allow a settle time (power budget). Driver on B2 as decoder code 3 (D-076). | PROPOSED | Gate B | Phase 2 |
| OI-08 | Board count and partition: 4 boards PROPOSED (D-042), B2/B3 merge open | PROPOSED | Gate B | Phase 2 |
| OI-09 | ~~Power budget and IRM derating~~ → RESOLVED by D-076 (`docs/phase2/POWER_BUDGET.md`): IRM-30-12 45 % and IRM-20-5 86 % margin at 45 °C / 108 VAC; fuses and eFuse settings set; B4 option C. Bring-up measurements in OI-41 | CLOSED (PROPOSED design) | — | — |
| OI-10 | Verify Torbal AD520-specific manual specs (only the AD-series manual was reachable; it mentions internal auto-cal) | UNRESOLVED | Gate A note | Phase 2 |
| OI-11 | ~~Masses plan~~ → CONFIRMED D-030 | CLOSED | — | — |
| OI-12 | RM68140 8080-parallel write timing; LVGL buffer sizing on 112 KB SRAM | UNRESOLVED | Gate B | Phase 2 |
| OI-13 | FMEA | RELEASE BLOCKER | Gate F | Phase 6 |
| OI-14 | Trickler mechanics: concept PROPOSED (D-047, `docs/phase2/TRICKLER_CONCEPT.md`); tube speeds rest on an ASSUMED η·φ, to be measured on the Phase 3 rig | PROPOSED | Gate B (concept) / Gate C (measured) | Phase 2/3 |
| OI-15 | Counterweight: option U, tare trim only (D-051) | PROPOSED | Gate B | Phase 2 |
| OI-16 | ~~Eccentric-load target basis~~ → RESOLVED by D-055: MET-09 stays the full-range goal (Phase 7); rig exit uses the dosing case plus OI-25 | CLOSED | — | — |
| OI-17 | Which bought balance to buy for Config S bring-up (see `phase0/SCALE_SEARCH.md`) | UNRESOLVED | Phase 3 | User |
| OI-18 | MCU pin budget: done (`docs/phase2/CONTROLLER_IO.md`). LQFP64 −11 for S+E → LQFP100 PROPOSED (D-040); CubeMX confirmation of the candidate map pending | PROPOSED | Gate B | Phase 2 |
| OI-19 | ~~Merge the v2 restructure PR~~ → DONE by D-061 (PR #1 and PR #2 merged into `main` 2026-11-22) | CLOSED | — | — |
| OI-20 | GraviMetra firmware deltas: 520 g, 3 stages, Nextion → RM68140/LVGL, Config S scale drivers, SmCo multi-sensor compensation | UNRESOLVED | Phase 4 | Phase 4 |
| OI-21 | Display shield: driven by an Arduino Uno (5 V), 2× 74HC245. LCD data now on 5 V-tolerant PE8–PE15 (D-056), so a 5 V read can't damage the MCU. Remaining question is V_IH margin only if the 245s run at 5 V (most such shields run them at 3.3 V to drive the 3.3 V panel). **DMM check before B3 layout:** pin 20 (VCC) of each 245, and what pins 1 (DIR) and 19 (OE) connect to Also measure the shield's current into the 5 V pin and the 3.3 V pin separately with the backlight on, and check for an onboard regulator (AMS1117-class) and a backlight series resistor (power budget: ~1.1 W, up to 2 W). | DEFERRED | Phase 4 (B3 layout) | User |
| OI-22 | ~~Long-kernel tolerance contradiction~~ → RESOLVED by D-048 (per-profile tol + warning < 1.1 × kernel mass) | CLOSED | — | — |
| OI-23 | Powder measurements per powder used: kernel d × L (20 kernels), mass of 100 kernels, bulk density (10 cm³ scoop). Replaces the ASSUMED inputs in `analysis/trickler_sizing.py` | UNRESOLVED | Gate C | User |
| OI-24 | ~~Magnet/coil Km ≥ 1.6~~ → RESOLVED by D-057: Km 4.4 (Recoma 28) / 3.2 (TC16 min Br); 6 mW at FS; r = 15 kept. Confirm on the rig (gap B, Kf, L) | CLOSED (PROPOSED design) | — | — |
| OI-25 | Corner-adjuster lock-in stability at the ~0.1–0.5 µm level after thermal cycling (empirical) | UNRESOLVED | Gate C | Phase 3 rig |
| OI-26 | Shim blade sourcing: the user buys Precision Brand 1095 blue-tempered rolls (D-054). Remaining: thickness check with a micrometer (±5 % drives blade K ±15 %), optional cert with E/UTS, optional photo-etch quote for dog-bone blades | IN PROGRESS | Gate C (rig) | User |
| OI-27 | Temperature-compensated SmCo quote: Ø31.75 × 12.7 and Ø25.4 × 12.7 axial discs in Arnold Recoma 2:17 Stab "0" / EEC 2:17-TC16. Need price, MOQ, lead time, RTC tolerance and Br min. Also confirm the EEC TC table values (extraction was misaligned) | OPTIONAL (fallback per D-062) | Gate C | User (vendor contact; Claude can draft the RFQ) |
| OI-28 | Pot steel: SendCutSend cuts only **A36/A1018 HRP&O** at 3/16 and 1/4 in (1008 cold-rolled stops at 3.43 mm); cut tolerance ±0.127 mm.
- The ASSUMED B-H curve is 1.7–2.4× conservative against Metals Handbook low-carbon curves, and Km is only ~0.25 % sensitive to it → order HRP&O.
- Open: price (needs the user's SendCutSend upload), micrometer each plate, Km re-run with a forgings-curve lower bound.
- Cut tolerance plus the 0.1 mm concentricity target leaves ~0.19 mm clearance on the tight side, not 0.35 mm | PROPOSED | Gate B | Phase 2 / user quote |
| OI-29 | SmCo disc sourcing: neither Ø31.75 × 12.7 nor the Ø25.4 × 12.7 fallback is stocked at the 10 sellers checked.
- Rig: First4Magnets Ø30 × 10 mm ($18.21, in stock, grade not stated; the sweep gives Km 3.98 vs 4.42), with a ~2.7 mm steel spacer. Get the seller's written grade.
- Production size by RFQ, merged with OI-27 | UNRESOLVED | Gate C | Phase 3 (rig purchase) |
| OI-30 | Passive temperature-compensator shunt experiment: Carpenter Temperature Compensator 30 strip across the pot (D-062). Get the datasheet curves (Type 1/2/3) and a small-quantity source, then measure k = d(shunt flux)/dT and the residual tempco on the rig | UNRESOLVED | Gate C | Phase 3 rig |
| OI-31 | Optical position sensor: OPR2100 is Active, datasheet Rev G 06/2022, 770 at Digi-Key ($11.51). It is six separate PIN chips (D / C\|E / B\|F / A), so the fine split is a gap between two chips. Outer cells serve as the coarse channel. Rig drift and noise test against D-067/D-074 (≤ 1 nm/√Hz, ≤ 100 nm per 60 s); the monolithic Hamamatsu S4349 quadrant is the comparison part The LED current is cell heat (~25 mW at 20 mA, as much as the coil and heater): use the lowest DC current that meets ≤ 1 nm/√Hz. | UNRESOLVED (test) | Gate C | Phase 3 rig |
| OI-32 | Garage floor vibration. The 500 g SD reaches 1 mg at 5.7× the assumed spectrum (0.33 s window) or 11× (0.9 s load-adaptive window, D-073). Measure on the rig, using the EMFC at 520 g as the seismometer; beyond ~11×: isolation or geophone feed-forward | UNRESOLVED | Gate C | Phase 3 rig |
| OI-33 | Coil soft-off on SAFE (~0.5 s RC) plus elastomer-faced lever stops: **decision CONFIRMED by D-070**. Remaining: rig impact test (stop arrival speed and peak force ≤ 1 kg-equivalent, MET-11) Add the mains-loss case (D-077) and a mains-pull test at full load. | UNRESOLVED (test only) | Gate C | Phase 3 rig |
| OI-34 | MET-04 linearity path: measure the ADS1263 INL shape on the rig. **9-point linearity adjustment with the D-030 masses is the baseline (D-072)**: ≤ 0.87 mg p95 even for a rough INL at the datasheet max. PWM-DAC duty measurand (M3) stays the fallback | UNRESOLVED | Gate C | Phase 3 rig |
| OI-35 | Coil manufacture: 4 × AWG 37 in hand on the Al bobbin, hand-wound on a **printed winding jig (D-070 CONFIRMED; jig design in `mech/coil_winder/`)**. Rig checks: radial build vs the 3.0 mm window, hairpin joint, residual heater force and coupling at 25 kHz, slit vs solid bobbin (ζ 0 vs 2.6) | IN PROGRESS (jig design) | Gate C | Phase 2 (jig) / Phase 3 rig (coil) |
| OI-36 | Structural modes: coil bracket and vane mount ≥ 500 Hz, Q ≤ 50 (D-067). Design check in Phase 4, swept-sine on the rig | UNRESOLVED | Gate C | Phase 3/4 |
| OI-37 | Auto-adjust policy: `docs/firmware/CALIBRATION.md` says the internal mass is "verification by default", while MET-07, D-062 and D-072 need span auto-adjust enabled (triggers, bounds, audit log, in-situ α learning). Needs the user's decision; then update `CALIBRATION.md` (also its 0–250 g sequence and ADS1262 mentions) | UNRESOLVED | Gate B (decision) | User + Phase 4 |
| OI-38 | MET-06 wording: "without recalibration" is met only after the in-situ model has ≥ 10 K of history (or a deliberate +10 K enclosure soak at commissioning), a B4 heat test and the coil set ±20 µm. Keep as is (recommended) or reword to "with MET-07 active" | UNRESOLVED | Gate B (decision) | User |
| OI-39 | Electrostatic force from charged powder in the grounded pan: 0.1 mg ASSUMED in the dosing budget. Measure on the rig (reading change with tribo-charged powder, with and without an ionizer) | UNRESOLVED | Gate C | Phase 3 rig |
| OI-40 | Mains-loss coil soft-off (D-077): the IRM-30-12 holds 12 V only 0.29–0.39 s against ~1.5 s needed. Choose (a) PWR_FAIL + 6.8 mF hold-up on the B4 branch (recommended) or (b) elastomer stops ≤ 2 kN/m | UNRESOLVED | Gate B (decision) | User |
| OI-41 | Power bring-up measurements: wall power per state, lowest reliable trickler run current, motor case temperature, IRM efficiency, B1 temperature after a soak, USB current, B4 thermal map empty vs 520 g, 2.25 MHz ripple on the B4 analog rail | UNRESOLVED | Gate C | Phase 3 rig |
