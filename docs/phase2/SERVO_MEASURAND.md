# EMFC servo loop and measurand (Phase 2)

Rev 0 · 2026-11-22
- Numbers: `analysis/servo_measurand.py` → `analysis/servo_measurand.out.txt` (§ numbers below match it). Pins: `analysis/pin_budget.py` → `analysis/pin_budget.out.txt`.
- Methods (outside methods, standard control engineering; not from the user's mechanical textbook):
  - loop shaping, with margins taken from exact complex frequency responses;
  - non-linear time-domain simulation:
    - 2-DOF plant: the lever plus the coil on its bracket;
    - 10 kHz discrete controller;
    - 12-bit DAC with 2nd-order sigma-delta;
    - actuator limits, stops and sensor saturation;
  - noise PSDs propagated through the closed loop and the readout filter.
- Plant inputs come from `EMFC_MECHANICS.md` §10 and `MAGNET_COIL.md`. Everything marked ASSUMED is measured on the Phase 3 rig.
- The checklist is the one in the Nov-2026 handoff (`docs/archive/2026-11_handoff/…Design_Audit.md` §5), which the user asked to have audited rather than assumed:
  - sensor noise;
  - ADC/DAC choice;
  - latency;
  - resonance;
  - bandwidth;
  - anti-windup;
  - null acquisition;
  - current limits;
  - crash and watchdog behaviour;
  - hybrid options.

## 1. Bottom line (all PROPOSED for Gate B)

- **Digital position loop in the STM32G491 at 10 kHz.** PID + lead + low-pass, with a notch slot.
  - 25 Hz crossover, 46° phase margin, 15 dB gain margin. The non-collocated coil-bracket mode sits 44 dB down.
  - Built analog, the same law gains only 1.3° of phase. The digital loop wins on everything that matters for a rig-first DIY build: retune, notch, acquisition, anti-windup, host tests.
  - **The user's preference holds up on the numbers.** (D-063)
- **Measurand: ADS1263 across a 150 Ω foil shunt, pure DC.** (D-064)
  - Noise is negligible after filtering.
  - The open risk is **MET-04 linearity**. The ADC's INL (3 ppm FSR typical, 12 max) is 9.6 / 38 ppm of our span, against 3.85 ppm allowed. That needs a ≥ 3-point linearity adjustment with CAL-03 masses.
  - Fallback is a PWM-DAC duty measurand (M3), tested on the rig.
  - ±I0 PWM in the coil (M2) is rejected: a ±0.42 N square-wave force, audible, and it needs a ~21-turn, 350 mA power coil.
- **Constant power (D-058) by winding the coil 4 wires in hand.** Two wires in series carry the force; two form a non-inductive hairpin heater driven at ≥ 25 kHz. Total 18 mW (35 mW with a TC magnet), Km 3.1 (D-065).
- **No power amplifier.** At 10–12.5 mA a zero-drift op-amp drives the coil directly, over −20 … +120 % of full scale, so removing the pan keeps the null. **OI-04 closes.** (D-066)
- **No HRTIM needed** → the G491 stays. OI-03 shrinks to the stock check. (D-069)
- **Readout profiles** (D-068):

  | Profile | Filter | Latency / settling | SD |
  |---|---|---|---|
  | Dispensing | sinc² × 100 ms | 0.1 s latency, 20 Hz output | 0.24 mg at 10 g |
  | Settling | sinc³ × 0.33 s | settles 1.14 s after the load lands (MET-05 goal 1.5 s) | 0.03 mg at 10 g, 0.17 mg at 500 g (MET-03 ≤ 1 mg) |

- **New requirements on other parts of the design** (D-067):
  - a coarse position channel over the full stop range;
  - non-collocated structural modes ≥ 500 Hz;
  - **compliant (elastomer) lever stops plus a hardware soft-off of the coil**. A hard coil cut at full load hits the stop with an 8.5 kg-equivalent peak on a stiff stop.

## 2. Plant and coil drive (output §1–2)

| Item | Value |
|---|---|
| Effective mass at the coil | 59.6 g. Rigid-body f_n 1.07 Hz (blades 2.62 N/m) |
| Open-loop damping ζ | 0.005 with a slit bobbin; 2.57 with a solid Al bobbin (eddy currents) |
| Coil-bracket mode | 915 Hz coupled (bracket alone 800 Hz, ASSUMED requirement ≥ 500 Hz) |
| Winding | 4 × AWG 37 in hand, 292 turns, 56.8 Ω each |
| Force winding (A+B in series) | 584 turns, 114 Ω, Kf 33.3 N/A, ~27 mH (estimate), Km 3.13 |
| Force-winding currents and power | I_FS 10.4 mA; −2.1 … +12.5 mA over −20 … +120 %; 12.3 mW at FS, 17.7 mW at max |
| Heater (C+D hairpin, non-inductive) | 114 Ω. P0 = 18 mW (Recoma 28) / 35 mW (TC16 min Br); ≤ 1.43 / 1.99 V rms |
| All-series option (no heater) | Km 4.42, kept so the rig can A/B linear vs constant-power drive on the same coil |
| Shunt | 150 Ω foil: −0.31 / +1.56 / +1.87 V at −20 % / FS / +120 %. 23 mW at max → PCR 0.47 ppm (∝ load²) |
| Driver compliance | 3.3 V (+ L di/dt) from a ~1.0 V return node on 5 V; swing 0.45 … 4.29 V |

## 3. Loop design (output §3)

Controller: C = Kp (1 + ω_i/s)(1 + s/ω_z)/(1 + s/ω_p) · LP2(ω_f).
- **Electronics (ASSUMED):** TIA 5 kHz, DAC reconstruction 1.5 kHz, current loop 3 kHz.
- **Digital delay:** 1.5 samples.
- **Selected values (f_c = 25 Hz):**
  - Kp 335 N/m (coil force per vane metre);
  - lead 6.2 / 100 Hz;
  - integral 2.5 Hz;
  - low-pass 300 Hz.
- **Derived values, not constants.** These come from the measured J, K, Kf and sensor slope at commissioning, never hard-coded (firmware invariant: unmeasured → servo refuses to start).

| f_c target | 10 Hz | 20 Hz | **25 Hz** | 30 Hz | 40 Hz |
|---|---|---|---|---|---|
| PM (empty, slit) | 48° | 47° | **46°** | 45° | 44° |
| GM | 16.7 dB | 15.3 dB | **14.7 dB** | 14.2 dB | 13.1 dB |
| \|S\|max | 1.44 | 1.49 | **1.51** | 1.54 | 1.59 |
| Loop gain at the bracket mode | −76 dB | −52 dB | **−44 dB** | −38 dB | −28 dB |

Why 25 Hz:
- Faster crossovers shrink the structural margin.
- Slower ones lengthen settling and the kernel-impact excursion.
- Full load and the solid bobbin change PM by < +13° and nothing else.

Delay cost of a digital loop at 25 Hz:

| Control rate | Phase cost |
|---|---|
| 10 kHz | 1.3° |
| 5 kHz | 2.7° |
| 2 kHz | 6.8° |

## 4. Analog vs digital vs hybrid (output §9)

| | Analog PID (Aug-2026 candidate) | **Digital in the STM32 (chosen)** | Hybrid (analog PD + digital I) |
|---|---|---|---|
| Phase margin at 25 Hz | 47° | 46° | 47° |
| Quantisation | none | 12-bit DAC + 2nd-order sigma-delta at 50 kHz by DMA: 0.009 mg residue | small |
| Retune / notch for a mode found on the rig | change parts | parameter | parts + parameter |
| Acquisition, anti-windup, gain scheduling | analog switches + MCU | code | mixed |
| Firmware hang | loop keeps running | DAC holds → external watchdog → SAFE → coil soft-off | loop keeps running |
| Host-testable (`pio test -e native`) | no (SPICE) | yes, against this plant model | partly |
| Parts on B4 | ~8 op-amp stages + switches | TIAs + driver | ~5 stages + switches |

On a firmware hang the digital loop is not worse in practice. The DAC holds the last force, which stays balanced until the load changes, and the external windowed watchdog (D-043) ends it within its window.

**The ADS126x is not used inside the loop.** Its sinc latency and its noise at high data rates rule it out. Position uses the MCU's own ADCs:
- ADC1 + ADC2 in dual simultaneous mode give a ratiometric (A−B)/(A+B);
- the MCU ADC noise is ~0.07 nm/√Hz equivalent at the chosen sensor gain, far below the sensor.

## 5. Time-domain behaviour (output §4)

Settling is measured as the ADS126x sees it (sinc4 at 1200 SPS + 20 ms average).

| Event | Peak vane excursion | Settles to ±0.5 mg | Stop contact |
|---|---|---|---|
| 10 g placed | 12 µm | 0.49 s | no |
| Pan placed (+90 g from pan-absent) | 104 µm (coarse channel takes over) | 0.64 s | no |
| Pan removed (−90 g; null held at −15 % FS) | 141 µm | 0.64 s | no |
| 520 g set down by hand (0.2 s) | 287 µm | 0.68 s | no |
| 520 g dropped (instant step) | 488 µm | 0.77 s | brief touch (stop is the protection) |
| One 2.5 mg kernel from 40 mm | 0.02 µm | 84 ms to ±0.1 mg | no |
| Acquisition from the stop, empty pan / 520 g on the pan | — | null in 0.25 / 0.61 s, overshoot ≤ 10 µm | no |

- **Kernel impacts:** the measured force impulse matches the applied 0.148 µN·s at the coil within 0.5 % (momentum check).
- **A steady falling stream** reads high by its impact force, which equals the weight of the powder still in flight (90 ms fall from 40 mm; 71 mg for the coarse stage). So during steady flow **the reading equals the mass already released**. The dosing planner needs no in-flight correction, only a stop-transient model.
- **Crash** (firmware freezes at full load, 10 g added, watchdog cuts the coil):

  | Coil shut-off | Arrival at the stop (pan) | Peak stop force (stiff stop model) |
  |---|---|---|
  | Hard cut | 3.9 mm/s | 8.5 kg-equivalent |
  | Hardware soft-off, RC 0.5 s | 1.0 mm/s | 2.1 kg-equivalent |

  To keep the peak ≤ 1 kg-equivalent (MET-11), the stop stiffness at the coil radius must be ≤ 2 kN/m for a hard cut (0.31 mm compression), or ≤ 33 kN/m with the soft-off (0.02 mm).
  - **Proposal:** elastomer-faced stops plus a hardware RC soft-off of the coil command when SAFE drops.
  - **Why:** the coil (12–35 mW) is no hazard to people, while the motors still cut hard.

## 6. Noise and readout profiles (output §5–6)

| Source (mg/√Hz) | 0.1 Hz | 1 Hz | 10 Hz |
|---|---|---|---|
| Fine sensor, 1 nm/√Hz, through the loop | 0.005 | 0.0005 | 0.44 |
| ADS126x 0.10 µV/√Hz on 150 Ω | 0.034 | 0.034 | 0.034 |
| Reference noise at 500 g | 0.006 | 0.006 | 0.006 |
| Floor vibration at 10 g / 500 g [ASSUMED quiet slab] | 0.006 / 0.16 | 0.006 / 0.16 | 0.28 / 6.9 |

Driver and DAC noise enter as actuator noise and are suppressed in band by 1/(1+L).

| Readout filter | Latency | SD 10 g | SD 500 g | SD 500 g, 10× vibration | Settles (10 g step) |
|---|---|---|---|---|---|
| sinc² × 50 ms | 0.05 s | 0.84 mg | 10.8 mg | 108 mg | 0.53 s |
| **sinc² × 100 ms (dispensing)** | 0.10 s | 0.24 mg | 3.9 mg | 39 mg | 0.59 s |
| sinc³ × 200 ms | 0.30 s | 0.044 mg | 0.44 mg | 4.4 mg | 0.84 s |
| **sinc³ × 330 ms (settling)** | 0.50 s | 0.032 mg | 0.17 mg | 1.7 mg | 1.14 s |
| sinc³ × 500 ms | 0.75 s | 0.026 mg | 0.12 mg | 1.2 mg | 1.57 s |

- **Dispensing profile:** latency 0.1 s against the ≤ 0.5 s the fine stage allows (`TRICKLER_CONCEPT`: flow × latency < m_k/2 at 2.5 mg/s). Output at 20 Hz.
- **Settling profile:** MET-03 and the MET-05 goal are met at 10 g and at 500 g in the assumed environment.
- **Floor-limited at full load:** with 10× the assumed floor vibration, the 500 g SD becomes 1.7 mg. Full-load repeatability is then set by the floor, not the electronics. Measure the garage floor on the rig (OI-32); at high loads, use a longer window or isolation feet.
- **Stable flag:** |slope| < 0.5 d/s and SD of the last 1 s < 0.5 d in the settling profile (d = 1 mg).

## 7. Measurand options (output §7)

| Option | Linearity | Noise | Other | Verdict |
|---|---|---|---|---|
| **M1: ADS1263 ADC1 across a 150 Ω foil shunt** | INL 3 ppm FSR typ / 12 max = 9.6 / 38 ppm of span → **3-point linearity adjustment needed** for MET-04 | 0.034 mg/√Hz, negligible | Electronics span tempco up to ~2.5 ppm/K typ. This is ADC gain 0.5 typ / 4 max, LTC6655B 2 max, shunt 0.05 typ. It is compensated against a B4 TMP117, separately from the magnet. LTC6655 thermal hysteresis is caught by auto-adjust | **Primary** |
| M2: ±I0 PWM through the coil, duty measurand | Inherent | — | ±0.42 N square-wave force: 225 nm at 1 kHz, 9 nm at 5 kHz, audible below ~18 kHz. Reversal through 27 mH takes 67 µs. An inaudible 20 kHz version needs ~21 turns at ~350 mA | **Rejected** |
| M3: precision PWM-DAC duty (reference switched into an RC filter, then the V-I driver) | Linear by construction; R_on mismatch 1 Ω → 2.5 ppm bow at 100 kΩ, 0.25 ppm at 1 MΩ | fine | G491 timer 118 ppm per 20 kHz period, sigma-delta dithered. **No HRTIM needed.** Sensitive to switch charge injection and R_on drift | **Rig fallback** for MET-04 |

**ADS1263, not ADS1262.** It is pin and function compatible [DS SBAS661C], and its auxiliary 24-bit ADC2 measures the coil voltage in the background (§10) without interrupting ADC1.

**Shunt.** The archived VCS1625ZP (0.3–10 Ω current-sense part) is **superseded** for this coil current, because 10 mA needs ~150 Ω. It becomes a VPG foil part, ~150 Ω, TCR ≤ 0.2 ppm/K, with the specific part and stock open (OI-05).

## 8. Constant power (D-058 → D-065)

The firmware sets **P_heater = P0 − I²·R_coil** every 10 ms.
- **Heater drive:** the C+D hairpin is driven bipolar (3-level) at ≥ 25 kHz by a small H-bridge on the 3.3–5 V rail. TIM15 CH1/CH2 set the duty, P = D·V²/R_h.
- **Same heat path:** the heater shares the force winding's copper volume, so the heat goes in at the same place.
- **No DC force:** residual hairpin imbalance (ASSUMED ~1 % of a turn per pair) makes only an AC force at the drive frequency. That is no DC error and inaudible.
- **Accuracy needed:** holding power to 2 % (0.36 mW) costs ~0.4 mg at 520 g with Recoma 28.

## 9. Coil driver and fault behaviour (D-066)

- **Topology:** a zero-drift op-amp (OPA2388 class, ±60 mA short-circuit) drives the coil. Coil and shunt return to a ~1.0 V node, so a single 5 V analog rail covers −20 … +120 %.
- **Slew:** with 0.5 V headroom the current slews 19 A/s through 27 mH, so a full-scale change takes 0.56 ms. The loop needs ~10 ms.
- **Protection:** hardware current limit, COIL_EN gated by SAFE, and COIL_FAULT (compliance/limit) to an EXTI.
- **On SAFE:** the command input switches to an RC that ramps the current to zero over ~0.5 s (soft-off, §5).

## 10. Coil temperature by resistance (D-060 implementation)

R_coil = R_s · V_coil / V_shunt. This ratio is independent of the reference: ADC2 measures V_coil (Kelvin taps), ADC1 measures V_shunt.

| ADC2 at 10 SPS | V_coil | Resolution per reading |
|---|---|---|
| Full scale (gain 1) | 1.18 V | 6.2 ppm = 1.6 mK |
| Empty pan (gain 32) | 24 mV | 12 ppm = 3.0 mK |

The heater winding's resistance (from the heater drive's V and I) is a coarse second estimate.

## 11. Position-sensor requirements (D-067; OPR2100 stays the candidate detector)

| Requirement | Value | Why |
|---|---|---|
| Fine channel linear range at the vane | ≥ ±50 µm, saturating monotonically beyond | Weighing transients stay inside it except pan and full-load steps |
| Fine channel noise | ≤ 1 nm/√Hz above 0.1 Hz | 0.0005 mg/√Hz at 1 Hz; shot-noise and ADC estimates are far below this |
| Coarse channel | Monotonic over the full stop range (±0.6 mm at the vane); ±5 % scale is fine. E.g. the OPR2100 outer cells on POS_COARSE | Acquisition from the stop and recovery from large steps |
| Bandwidth | ≥ 5 kHz (TIA + anti-alias) | Loop phase |
| Drift | 100 nm per weighing cycle is the mechanics allocation (D-050). Ratiometric readout removes LED intensity | Zero error |
| Non-collocated modes (coil bracket, vane mount) | ≥ 500 Hz, Q ≤ 50 | Loop gain < −20 dB at the mode including the peak. A firmware notch covers surprises |

## 12. Firmware structure (feeds `docs/firmware/ARCHITECTURE.md`)

- **Servo ISR, 10 kHz, TIM-triggered dual ADC:** the ratio passes through a mode-dependent controller to the sigma-delta, then to DAC1 by DMA at 50 kHz.
  - Modes: OFF / ACQUIRE (coarse channel, ramped setpoint) / TRACK (fine, coarse takeover beyond ±75 µm) / FAULT.
  - Load: ~400 cycles = 2.4 % of the core.
  - Hardware triggering keeps ISR jitter off the plant.
- **ADS1263 DRDY at 1200 SPS:**
  - readout profiles;
  - temperature and span model (magnet α and electronics α, both learned in situ, D-062);
  - `EmfcScaleSource` (the same `ScaleSource` interface as Config S, so the dosing logic doesn't care which balance it has).
- **10–100 Hz tasks:** heater power, coil R, health checks. Health checks cover:
  - sensor in range;
  - DRDY fresh;
  - DAC vs ADC current agreement within 1 % (a driver-fault check);
  - stop contact longer than X ms.

  Any failure → coil soft-off, FAULT.
- **Parameters:** controller parameters are computed from the measured plant parameters, which are calibration data and invalid until measured.

## 13. Pin changes (CONTROLLER_IO.md, solver rules A10/A12)

- **Removed:** SERVO_MODE, POS_MON.
- **Added:**

  | Signal | Pin | Function |
  |---|---|---|
  | POS_A | PB12 | ADC1_IN11 |
  | POS_B | PC4 | ADC2_IN5 (dual simultaneous with POS_A) |
  | POS_COARSE | PA2 | ADC1_IN3 |
  | LED_EN | PE2 | |
  | HEAT_IN1 / HEAT_IN2 | PF9 / PF10 | TIM15_CH1 / CH2 |
  | HEAT_ISENSE | PA3 | ADC1_IN4 |
- **Kept:** EMFC_DAC on PA4 (DAC1_OUT1).
- **Counts:** Config S+E needs 59 pins, all tiers 66. LQFP100 has **11 spare**; LQFP80 can no longer carry Config E (−2).

## 14. Contradictions and changes to log

1. **D-057's winding is superseded.** It was one AWG 34 winding, Km 4.43. It becomes 4 × AWG 37 in hand, Km 3.13 in the heater configuration (D-065). Km stays ≥ 2.25 even with the TC magnet at minimum Br.
2. **OI-05's shunt candidate (VCS1625Z, 1.5 Ω) is superseded** by a ~150 Ω foil part.
3. **CLAUDE.md firmware invariant** ("Config E measurand candidate: ADS1262 across a 4-terminal shunt"): ADS1263, the pin-compatible sibling with an extra ADC, is now proposed. The shunt stays Kelvin-connected.
4. **SYSTEM_ARCHITECTURE.md says only digital signals cross to B4.** Changed: the precision path (shunt, reference, ADS1263) stays on B4. The loop's position and command signals cross as buffered differential analog; they sit inside the loop and aren't precision.
5. **SAF-02/03 say the coil is "disabled on fault".** This study proposes a 0.5 s hardware soft-off instead of an instant cut, to protect the flexures. It changes how a CONFIRMED safety requirement is met → **needs the user's confirmation**.
6. **MET-03 at ~500 g in a garage may be floor-limited** (1.7 mg at 10× the assumed vibration). It is not contradicted yet; measure first (OI-32).

## 15. Rig checks (Phase 3)

1. Sensor:
   - noise spectrum and drift;
   - coarse/fine hand-over;
   - LED and ratiometric behaviour.
2. Plant:
   - Bode by swept-sine injection (J, K, damping solid vs slit, bracket and lever modes);
   - set the notch if needed.
3. Loop:
   - margins from a closed-loop sweep;
   - step and pan tests against §5;
   - acquisition from both stops.
4. ADS1263: noise floor with the coil open, INL over a 0–2 V sweep, and the 3-point linearity adjustment with E2/F1 masses.
5. Constant power: linear vs constant-power drive on the same coil (all-series vs A+B / C+D), magnet temperature vs load.
6. Floor vibration: the EMFC at 520 g is the seismometer (OI-32).
7. Crash and SAFE soft-off against elastomer stops; measure the peak with a load cell or accelerometer.

## 16. Assumptions to replace with data

- **Electronics:** sensor noise 1 nm/√Hz (corner 0.1 Hz); bandwidths TIA 5 kHz, DAC reconstruction 1.5 kHz, current loop 3 kHz.
- **Mechanics:**
  - bracket mode 800 Hz, Q 25;
  - air damping ζ 0.005;
  - stop model 300 Hz, ζ 0.3.
- **Environment:**
  - floor-vibration model (quiet slab);
  - kernel drop height 40 mm.
- **Parts:**
  - heater imbalance ~1 %;
  - shunt rated power 0.25 W;
  - fill 0.55 and inductance estimate (from `MAGNET_COIL.md`).
