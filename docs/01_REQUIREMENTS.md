# 01 — Requirements (Phase 1, Gate A approved)

Rev A2 · 2026-11-21 (A0 approved at Gate A; A1 adds D-034 display, bench supply, D-037 note; A2 adds D-048 per-profile tolerance and D-040 MCU package) · Numbers derived in `analysis/req_numbers.py` (output in `analysis/req_numbers.out.txt`).

**Status:** CONFIRMED (approved by user) · CANDIDATE (preferred, still being validated) · PROPOSED (needs Gate A approval) · UNRESOLVED · SUPERSEDED.
**Verification:** T = test · A = analysis · I = inspection · D = demonstration.
**Configurations:** **S** = bought scale over RS-232 · **E** = DIY EMFC scale. A requirement applies to both unless tagged.

---

## 1. Metrology — Config E (AD520-class)

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| MET-01 | Capacity 520 g on the pan (pan/tray excluded via counterweight, see MET-15) | CONFIRMED | Torbal AD520 | T |
| MET-02 | Readability 0.001 g; grain display in 0.02 gr steps (1 mg = 0.0154 gr) | CONFIRMED | AD520; grain conversion | I/T |
| MET-03 | Repeatability SD ≤ 0.001 g (10 loadings each at ~10 g and ~500 g) | CONFIRMED | AD520 | T |
| MET-04 | Linearity ≤ ±0.002 g over 0–520 g (= ±3.85 ppm FS; 520,000 d) | CONFIRMED | AD520 | T (needs CAL-03 masses) |
| MET-05 | Stabilization ≤ 3 s required; **goal ≤ 1.5 s** for dosing throughput | CONFIRMED / goal PROPOSED | AD520 3 s; A&D FX-i about 1 s | T |
| MET-06 | Span temperature drift ≤ ±2 ppm/K over 10–30 °C, after firmware compensation and without recalibration (1.04 mg/K at Max) | PROPOSED | FX-i ±2 ppm/°C; PG-S ±3 ppm/°C | T (garage temperature swing logged against TMP117s) |
| MET-07 | Automatic internal span check/adjust when ΔT ≥ 0.5 K since the last adjust, at power-up after warm-up, and on user request | PROPOSED | Needed for MET-06 at Max (see §9 risk) | D |
| MET-08 | Zero stability ≤ 1 mg over a 60 s dosing cycle at stable temperature | PROPOSED | Dosing re-tares every throw | T |
| MET-09 | Eccentric load ≤ ±0.002 g at ~175 g (1/3 Max), OIML R76 quadrant positions | PROPOSED | Matches linearity; powder is center-loaded | T |
| MET-10 | Warm-up to full spec ≤ 60 min | PROPOSED | — | T |
| MET-11 | Overload: OL indication above Max; mechanical stops protect flexures from ≥ 1 kg static on the pan; zero shift ≤ 2 mg after overload | PROPOSED | Protects SendCutSend flexures | A + T |
| MET-12 | Spec holds at 15–30 °C, ≤ 85 % RH non-condensing | CONFIRMED (temperature) / PROPOSED (RH) | AD520 15–30 °C; FX-i 85 % RH | T |
| MET-13 | Weight output to dosing firmware ≥ 10 Hz, with a stable flag | PROPOSED | A&D streams at 5/10/20 Hz | T |
| MET-14 | SmCo magnets; magnet, coil and ambient temperatures measured; span compensation in firmware | CONFIRMED | User | I/T |
| MET-15 | Counterweight carries the pan/tray tare, so the coil carries mostly the net load | CONFIRMED (Q1) | User | I/A |
| MET-16 | No printed parts in the cell or anything supporting it. Flat metal from SendCutSend, with alignment from cut dowel holes. No CNC | CONFIRMED | User | I |

## 2. Calibration

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| CAL-01 | External calibration with user masses, using a documented procedure. Traceability is claimed only as far as the masses' certificates support it | CONFIRMED | Handoff §13 | D |
| CAL-02 | Internal check mass for span check/adjust. Value and actuator are chosen in Phase 2. Not described as traceable | CONFIRMED | Handoff §13 | D |
| CAL-03 | Masses for verifying MET-04 need MPE ≤ 0.67 mg (1/3 of ±2 mg). F1 qualifies ≤ 100 g. At 200 g only E2 qualifies. At 500 g even E2 (0.75 mg) needs its **certified value** used, not the nominal | PROPOSED (Gate A #1) | OIML R111-1 MPE; OIML calls for E2 above 100,000 d (this is 520,000 d) | A |
| CAL-04 | Temperature-compensation coefficients characterized per unit and stored in non-volatile memory | PROPOSED | MET-06 | T |

## 3. Bought-scale interface — Config S

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| SCS-01 | Driver priority: A&D FX-i/FZ-i/GF, then Mettler MT-SICS, then Shimadzu UW/UX | CONFIRMED (Q3) | `SCALE_SUPPORT_research.md` | T per model |
| SCS-02 | Minimum 5 Hz stream with a stable flag. Otherwise a firmware stability-detection mode, flagged "degraded" on the HMI | PROPOSED | Torbal, Sartorius and G&G frames lack a stable flag | T (scale simulator) |
| SCS-03 | DE-9 RS-232 port wired DTE (straight cable mates to A&D DCE). DB-25 adapter cable for the GF series. MAX3232-class transceiver with ESD protection | PROPOSED | A&D pinout; BOM audit | I/T |
| SCS-04 | Per-model printed adapter positions the trickler outlets over that scale's pan | CONFIRMED | Prompt | I |
| SCS-05 | Maintain a scale support matrix (`docs/SCALE_SUPPORT.md`) | CONFIRMED | Prompt | I |
| SCS-06 | The dispenser works end-to-end with an A&D balance before the EMFC module is finished | CONFIRMED (Q3) | De-risking | D |

## 4. Dispensing

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| DSP-01 | Three channels (coarse, medium, fine), each a belt-driven stepper turning a tube/shaft. Only one motor runs at a time | CONFIRMED | Handoff §7 | I/T |
| DSP-02 | Powder forms: spherical, extruded (including long-kernel), flake | CONFIRMED (Q4) | User | T per form |
| DSP-03 | Charge range 5–150 gr (0.324–9.72 g) | CONFIRMED (Q4) | User | T |
| DSP-04 | Accept a charge when the stable reading is within [target, target + tol]. Default tol = +0.02 gr / −0, **set per powder profile**. The HMI warns when tol < 1.1 × the profile's measured mean kernel mass (D-048). True-mass uncertainty is bounded by the scale (3σ about 3 mg ≈ 0.046 gr for AD520-class) | CONFIRMED (D-031, D-048) | Reloader "never under" practice; kernel quantization (OI-22) | T |
| DSP-05 | Overthrow (above target + tol): stop, alarm and ask the user to dump the pan. Goal ≤ 5 % of throws for spherical, ≤ 10 % for extruded, **applying only when tol ≥ 1.1 × mean kernel mass** (D-048) | PROPOSED | Single-kernel overshoot model, `analysis/trickler_sizing.py` | T (≥ 50 throws per form) |
| DSP-06 | Cycle time ≤ 30 s per charge | CONFIRMED (Q4) | User | T |
| DSP-07 | Motors-off settle window before final acceptance. Trickler frame mechanically isolated from the scale | CONFIRMED | Prompt | T/I |
| DSP-08 | One hopper feeding all three channels, ≤ 1 lb (453.6 g) resident, quick-empty | CONFIRMED (Q5) | User | I |
| DSP-09 | Removable conductive pan with presence detection and auto-tare | CONFIRMED (Q6) | User | D |
| DSP-10 | Powder-contact parts conductive or static-dissipative and bonded to PE | CONFIRMED | Prompt | I/T (resistance to PE) |

## 5. Power

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| PWR-01 | Fused, switched IEC C14 inlet feeding the user's Mean Well IRM-30-12 + IRM-20-5, on a mains board in a grounded metal SendCutSend compartment | CONFIRMED (Q7) | User | I |
| PWR-02 | 120 VAC 60 Hz (US). Universal input if the module datasheets allow | PROPOSED | IRM datasheet check in Phase 2 | I |
| PWR-03 | No battery on Rev A; the battery connector/footprint is kept for Rev B. *Supersedes the handoff's "battery CONFIRMED desired" for Rev A only* | CONFIRMED (Q8) | User | I |
| PWR-04 | Power budget per rail with ≥ 25 % margin on each module; modules derated per datasheet | UNRESOLVED (Phase 2) | — | A |

## 6. Environment, size, test equipment

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| ENV-01 | Garage bench operation, 15–30 °C. Outside that range it still works, but the MET spec isn't guaranteed and the HMI shows a warning | CONFIRMED (Q15) | User | T |
| ENV-02 | Footprint ≤ 450 × 350 mm | CONFIRMED (Q15) | User | I |
| ENV-03 | Draft shield enclosing the pan | CONFIRMED | Prompt | I |
| ENV-04 | Test equipment on hand: Siglent SDS1204X-E with AWG and 16-ch logic probe (Bode plots for servo characterization), Keysight 33210A, Fluke 287, soldering iron, current-limited bench supply. Masses per D-030 | CONFIRMED (Q12, Gate A) | User | — |

## 7. Interfaces, HMI, firmware

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| IF-01 | USB-C service port (CDC) | CANDIDATE | — | T |
| IF-02 | HMI: user's 3.5" 320×480 MCUFRIEND-style shield TFT (ID 0x6814, RM68140), 8-bit parallel, 4-wire resistive touch; LVGL with partial buffers on the MCU | CONFIRMED (D-034) | Proven in Kaboom-Konveyor; fully codable | D |
| IF-03 | CAN and RS-485 footprints present but not populated | CONFIRMED (Q14) | User | I |
| IF-04 | SWD debug header with SWO | PROPOSED | Bring-up | I |
| FW-01 | Headless build + CI with PlatformIO (`native` Unity tests + `gravimetra_g491` target), STM32Cube HAL/LL, ST-Link V3 | CONFIRMED (Q14, D-037) | User; GraviMetra repo | D (CI build) |
| FW-02 | Original code, no GPL-derived code (OpenTrickler used for ideas only) | CONFIRMED (Q14) | User | I |
| FW-03 | Scale-driver abstraction, scale simulator, host unit tests | CONFIRMED | Prompt | T |
| FW-04 | MCU STM32G491**VET6** (LQFP100); SUPERSEDES RET6 (LQFP64 is −11 pins for Config S+E). G474VET6 stays an alternative until OI-03 closes | CONFIRMED package (D-040) / part CANDIDATE | `analysis/pin_budget.out.txt`; stock unverified (OI-03) | A |

## 8. Fabrication

| ID | Requirement | Status | Basis | Verify |
|---|---|---|---|---|
| FAB-01 | PCBs by JLCPCB/PCBWay. JLC places all SMT; the user hand-solders THT and connectors | CONFIRMED (Q11) | User | I |
| FAB-02 | Hand-sourced parts kept to a short, listed set | PROPOSED | BOM audit | I |
| FAB-03 | Printed parts fit the X2D envelope (256 × 256 × 260 mm). PETG/ASA, ESD-safe where in powder contact | CONFIRMED (Q10) | User | I |
| FAB-04 | Deliverables: STEP, STL/3MF with Bambu print notes, DXF + SendCutSend order list, dimensioned PDF drawings | CONFIRMED | Prompt | I |
| FAB-05 | Build 1–2 units; report cost per configuration and per prototype round | CONFIRMED (Q9) | User | A |

## 9. Safety (hardware-enforced)

| ID | Requirement | Status | Verify |
|---|---|---|---|
| SAF-01 | E-stop and lid interlock cut motor power or force driver enable off without going through the MCU. Restart needs a deliberate action | CONFIRMED | T |
| SAF-02 | Coil current limited in hardware; coil disabled on fault | CONFIRMED | T |
| SAF-03 | Safe pin states through power-on, reset, brown-out and MCU crash. External watchdog disables motors and coil | CONFIRMED | T |
| SAF-04 | One motor at a time in firmware. Hardware enforcement evaluated | CONFIRMED / hardware PROPOSED | T |
| SAF-05 | Mains: fused, switched inlet; PE bond to chassis and powder metal; isolated modules; barrier on the PCB and in the harness | CONFIRMED | I/T (earth-bond test) |
| SAF-06 | Powder zone free of exposed switching nodes, arcing contacts, brushed motors and hot surfaces. Electronics sealed from the powder path. Resident powder minimized | CONFIRMED | I |
| SAF-07 | Motor and driver temperature monitoring, fault log, safe recovery | CONFIRMED | T |
| SAF-08 | No hazardous-location compliance claims | CONFIRMED | I |
| SAF-09 | FMEA completed before any fabrication release | RELEASE BLOCKER | A |

---

## Top technical risk carried into Phase 2

At 520 g on the coil, SmCo's raw tempco (−0.03 to −0.05 %/K) gives 156–260 mg/K. Meeting 2 ppm/K by temperature compensation alone would require knowing the magnet temperature to within about 4–6 mK, which isn't realistic. Phase 2 must close this with a combination of:
- the counterweight, possibly preloaded so the coil current is bipolar and carries at most half the range;
- temperature-compensated SmCo grades (Gd/Er-doped "LTC"/"Stab");
- automatic internal span adjust on temperature change (MET-07);
- per-unit characterization.

For powder charges (≤ 10 g net on the coil), the same tempco is only 3–5 mg/K, and a 200–333 mK sensing error keeps it under 1 mg, so dosing is far less sensitive than full-range spec.
