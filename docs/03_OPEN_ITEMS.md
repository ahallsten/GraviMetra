# 03 — Open items

| # | Item | Status | Needed by | Owner |
|---|---|---|---|---|
| OI-01 | ~~Display variant~~ → RESOLVED by D-034 (MCUFRIEND RM68140 8-bit parallel shield). Verify the controller ID against the panel at bring-up | CLOSED | — | — |
| OI-02 | Span tempco closure at full coil load. Preload doesn't help (D-051). Magnet/coil study: standard Sm2Co17 is −336 ppm/K net → needs α to 0.6 %, a lag-compensated magnet temperature (~3.5 ppm lag at 2 K/h), constant coil power (D-058) and auto-adjust at ΔT ≥ 0.5 K. A TC grade (≈ +4 ppm/K) relaxes this 50× (D-059, OI-27) → error-budget study | UNRESOLVED · top risk | Gate B | Phase 2 |
| OI-03 | MCU: STM32G491RET6 stock (0 at Digi-Key, 53 at LCSC); evaluate G474 (HRTIM) for the PWM-measurand option | UNRESOLVED | Gate B | Phase 2 |
| OI-04 | Coil driver: OPA593 out of stock everywhere; choose an alternate architecture | UNRESOLVED | Gate B | Phase 2 |
| OI-05 | Reference (LTC6655LNB-2.5 candidate) and shunt (VCS1625Z drop-in, stock unknown) | UNRESOLVED | Gate D | Phase 2/4 |
| OI-06 | TMC2209-LA availability (21 at LCSC) | UNRESOLVED | Gate D | Phase 4 |
| OI-07 | Internal check mass: 200 g on a receptor kinematic seat, geared-stepper lift (D-052) | PROPOSED | Gate B | Phase 2 |
| OI-08 | Board count and partition: 4 boards PROPOSED (D-042), B2/B3 merge open | PROPOSED | Gate B | Phase 2 |
| OI-09 | Power budget and Mean Well IRM derating; verify the IRM datasheets | UNRESOLVED | Gate B | Phase 2 |
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
| OI-21 | Display shield: driven by an Arduino Uno (5 V), 2× 74HC245. LCD data now on 5 V-tolerant PE8–PE15 (D-056), so a 5 V read can't damage the MCU. Remaining question is V_IH margin only if the 245s run at 5 V (most such shields run them at 3.3 V to drive the 3.3 V panel). **DMM check before B3 layout:** pin 20 (VCC) of each 245, and what pins 1 (DIR) and 19 (OE) connect to | DEFERRED | Phase 4 (B3 layout) | User |
| OI-22 | ~~Long-kernel tolerance contradiction~~ → RESOLVED by D-048 (per-profile tol + warning < 1.1 × kernel mass) | CLOSED | — | — |
| OI-23 | Powder measurements per powder used: kernel d × L (20 kernels), mass of 100 kernels, bulk density (10 cm³ scoop). Replaces the ASSUMED inputs in `analysis/trickler_sizing.py` | UNRESOLVED | Gate C | User |
| OI-24 | ~~Magnet/coil Km ≥ 1.6~~ → RESOLVED by D-057: Km 4.4 (Recoma 28) / 3.2 (TC16 min Br); 6 mW at FS; r = 15 kept. Confirm on the rig (gap B, Kf, L) | CLOSED (PROPOSED design) | — | — |
| OI-25 | Corner-adjuster lock-in stability at the ~0.1–0.5 µm level after thermal cycling (empirical) | UNRESOLVED | Gate C | Phase 3 rig |
| OI-26 | Shim blade sourcing: the user buys Precision Brand 1095 blue-tempered rolls (D-054). Remaining: thickness check with a micrometer (±5 % drives blade K ±15 %), optional cert with E/UTS, optional photo-etch quote for dog-bone blades | IN PROGRESS | Gate C (rig) | User |
| OI-27 | Temperature-compensated SmCo quote: Ø31.75 × 12.7 and Ø25.4 × 12.7 axial discs in Arnold Recoma 2:17 Stab "0" / EEC 2:17-TC16. Need price, MOQ, lead time, RTC tolerance and Br min. Also confirm the EEC TC table values (extraction was misaligned) | OPTIONAL (fallback per D-062) | Gate C | User (vendor contact; Claude can draft the RFQ) |
| OI-28 | Pot steel: SendCutSend mild-steel grade (CR 1008/1010 vs A36), thicknesses 3/16 and 1/4 in, cut-diameter tolerance, B-H data or cert. Sets the clearance and the B-H assumption | UNRESOLVED | Gate B | Phase 2 (sourcing) |
| OI-29 | SmCo disc sourcing, Ø31.75 × 12.7 mm (1¼ × ½ in) Sm2Co17 axial (fallback Ø25.4 × 12.7): stock and price | UNRESOLVED | Gate C | Phase 3 (rig purchase) |
| OI-30 | Passive temperature-compensator shunt experiment: Carpenter Temperature Compensator 30 strip across the pot (D-062). Get the datasheet curves (Type 1/2/3) and a small-quantity source, then measure k = d(shunt flux)/dT and the residual tempco on the rig | UNRESOLVED | Gate C | Phase 3 rig |
