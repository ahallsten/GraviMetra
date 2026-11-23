# EMFC error and noise budget (Phase 2, Config E)

Rev 0 · 2026-11-22 · Effort: max · Numbers: `analysis/error_budget.py` → `analysis/error_budget.out.txt` (§ numbers below match it). Everything here is **PROPOSED for Gate B** unless marked otherwise.

- **Method** (outside methods, not from the user's textbook): GUM-style budgets. Independent standard uncertainties (k = 1) are combined root-sum-square and compared with the limits at k = 2. MET-03 is itself an SD, so it is compared at k = 1.
- **Noise** imports the servo study's plant, loop and noise model from `servo_measurand.py`, so the two studies can't drift apart. The script checks that it reproduces 0.032 / 0.172 mg.
- **Linearity** is a Monte Carlo of the linearity adjustment, 4000 draws per case. It includes:
  - randomised ADC INL shapes;
  - the mechanical and magnetic nonlinearities;
  - calibration-mass errors (OIML R111);
  - repeatability noise.
- **Environment is ASSUMED:** floor vibration, temperature ramps at the cell, drafts, static. The environment research and the rig (OI-32) replace it.

## 1. Bottom line

**Config E meets the AD520-class numbers on paper, in the assumed environment, with the conditions in §10.** No requirement is blocked by physics.

| Requirement | Prediction | Limit | Status | Rests on |
|---|---|---|---|---|
| MET-03 SD at 10 g | 0.07 mg | ≤ 1 mg | OK | — |
| MET-03 SD at 500 g | 0.31 mg (assumed floor); 0.90 mg at 10× the floor with a 0.9 s window | ≤ 1 mg | OK, floor-dependent | Fine corner adjust; floor ≤ 5.7× (0.33 s window) or ≤ 11× (0.9 s window) of the assumed spectrum (OI-32) |
| MET-04 linearity (p95, incl. 0.2 mg hysteresis) | 0.67–0.87 mg with a 9-point adjust; 1.17 mg with 3 points if the INL is typical | ±2 mg | OK with 9 points | D-030 masses only; INL shape (OI-34) |
| Span error at 520 g in use (MET-07 auto-adjust) | 0.94 mg (k = 2) | ±2 mg | OK | 0.5 K auto-adjust trigger |
| MET-06 static (no recalibration) | 5.3 ppm/K today → 1.3 ppm/K with best practice (k = 2) | ±2 ppm/K | Only with conditions (§6.2) | ≥ 10 K of learned history, B4 heat test, coil set ±20 µm |
| MET-08 zero over 60 s | 0.87 mg (baseline hinges) / 0.46 mg (light hinges), k = 2 | ≤ 1 mg | OK | Optical-null drift ≤ 100 nm per 60 s (OI-31) |
| MET-09 eccentric | 1.0 mg in x + 1.0 mg in y at the adjuster resolution | ±2 mg | Needs the fine adjusters | Lock-in stability (OI-25) |
| MET-05 settling | 1.14 s at 10 g; ~2.7 s at 500 g with the 0.9 s window | 3 s (goal 1.5 s) | OK | — |
| Dosing, 3σ of a 10 g charge | 1.44 / 0.93 mg = 0.022 / 0.014 gr | DSP-04 assumed 3 mg | Better than assumed | Null drift; static charge (ASSUMED) |

### OI-02 (span tempco, the top risk): closed by analysis for the operating spec, with the standard Sm2Co17 magnet
- With auto-adjust at ΔT ≥ 0.5 K, the span error at 520 g is 0.94 mg (k = 2). The worst single change (rough max INL drifting, 5 K/h ramps or a 10× floor) keeps it ≤ 1.6 mg.
- A TC magnet isn't needed and stays the fallback (D-062).
- MET-06 as written (static, no recalibration) is met only under the §6.2 conditions. That is a Gate B wording question.

### r = 15 stays
- The light hinge set (pivots 0.04 × 8 mm, guide 0.075 mm, truss lever) gives MET-08 more margin than r = 20 would.
- It does so without a longer cell, a heavier counterweight or lower lever modes.

### Surprises
1. **Linearity needs 9 points, not 3, if the ADS1263's INL is near its datasheet maximum and has any texture.** The 9-point set is built only from the D-030 masses.
2. **MET-03 at 500 g depends on the corner adjustment as much as on vibration.** With only the dosing-level adjust, placement scatter alone gives 6.1 mg.
3. **The y corner adjuster is needed even for dosing.** Left unadjusted, the roll-centre offset from shim tolerance gives 125 mg at 10 g. EMFC_MECHANICS checked only x for dosing.
4. **Static MET-06 with standard SmCo needs the curvature of α**, which means ≥ 10 K of learned history. A linear model alone misses by 2.5 ppm/K on a 20–30 °C test.
5. **One check-mass adjustment is 0.15 ppm, not 1.7 ppm.** The mechanics study used the 1 mg MET-03 limit as the SD; the loop predicts 0.077 mg at 200 g. α is therefore learnable to 0.02 %, not 0.14 %.

## 2. Inputs and conversions (output §1–3)

- **Conversions:**
  - 1 mg on the pan = 19.6 nA in the force winding = 2.94 µV on the 150 Ω shunt;
  - 1 ppm of the ADC's 5 V FSR = 1.70 mg;
  - 520 g sits at 62 % of +VREF.
- **Noise per reading (mg SD):**

  | Profile | 0 g | 10 g | 200 g | 500 g | 500 g at 10× floor |
  |---|---|---|---|---|---|
  | Dispensing, sinc² × 0.10 s | 0.20 | 0.24 | 1.60 | 3.87 | 38.6 |
  | Settling, sinc³ × 0.33 s | 0.031 | 0.032 | 0.077 | 0.172 | 1.70 |
  | High-load, sinc³ × 0.90 s | 0.019 | 0.019 | 0.040 | 0.088 | 0.86 |

  **Load-adaptive window (refines D-068):** 0.33 s below ~100 g, 0.66–0.9 s above. A 500 g step then settles in ~2.2–2.7 s; MET-05 allows 3 s.
- **Magnet temperature estimate:**
  - TMP117 at 8 results/s with 8× averaging, followed by a 120 s ramp-fit estimator (value at the window end, no ramp lag): 0.25 mK → 0.08 ppm. Its ~2 LSB of noise dithers the 7.8 mK quantisation.
  - An NTC on ADS1263 ADC2 reaches 0.03 mK. That is the upgrade if the rig shows quantisation steps.
- **Magnet lag:**
  - τ = 9 s two-sided. In an ambient ramp the steel pot is nearly isothermal and heats the bonded magnet through both faces: L²/12a = 4 s, plus ~5 s through the bond lines.
  - MAGNET_COIL's 19 s is the one-sided bound.
  - At 2 K/h that is 1.7 / 3.5 ppm uncompensated and 0.5 / 1.1 ppm compensated (30 % residual ASSUMED).

## 3. MET-03 repeatability (output §4)

| Case | Noise | Placement | Temp. estimate | Drafts | Total (k = 1) |
|---|---|---|---|---|---|
| 10 g, fine corners, 1× floor | 0.045 | 0.005 | 0.001 | 0.05 | **0.07** |
| 500 g, fine, 1×, 0.33 s window | 0.175 | 0.242 | 0.042 | 0.05 | **0.31** |
| 500 g, fine, 3×, 0.33 s | 0.51 | 0.242 | 0.042 | 0.05 | **0.57** |
| 500 g, fine, 10×, 0.33 s | 1.70 | 0.242 | 0.042 | 0.05 | **1.72 FAIL** |
| 500 g, fine, 10×, 0.9 s | 0.86 | 0.242 | 0.042 | 0.05 | **0.90** |
| 500 g, coarse (dosing) corners, 1× | 0.175 | 6.11 | 0.042 | 0.05 | **6.1 FAIL** |

- **Noise** includes the zero reading (zero re-checked between loadings, ASSUMED test practice).
- **Placement:** 1.5 mm per axis by hand × the corner sensitivity, which is 0.23 ppm/mm after the fine adjust and 5.6–6.0 ppm/mm after the coarse one.
- **Vibration headroom:** the 500 g SD reaches 1 mg at 5.7× the assumed floor with the 0.33 s window, 9.6× at 0.66 s and 11.2× at 0.9 s.
- **Beyond ~11×:** isolation feet help only above ~10 Hz. A geophone feed-forward is the remaining option (not designed).

## 4. MET-08 zero stability over 60 s (output §5)

| Term (mg, k = 1) | Baseline hinges | Light hinges |
|---|---|---|
| Optical-null drift, 100 nm per cycle (ASSUMED, as 1σ) | 0.42 | 0.20 |
| Noise over 60 s | 0.044 | 0.044 |
| Thermal at ≤ 0.5 K/h (zero tempco 2.5 / 1.4 mg/K) | 0.021 | 0.012 |
| Drafts while dosing (ASSUMED) | 0.10 | 0.10 |
| Tare reading | 0.031 | 0.031 |
| **Total k = 1 / k = 2** | **0.44 / 0.87** | **0.23 / 0.46** |

- **Air buoyancy of the lever's unbalanced volumes:** ~1.3 mg per 1 % of air density, or 0.44 mg/K. Weather moves the zero by milligrams over hours. Dosing re-tares every throw, so this is harmless.
- **Requirements on the optical sensor and mount:** ≤ 100 nm per 60 s, and a null-to-vane thermal path ≤ 0.5 µm/K.

## 5. MET-04 linearity (output §6)

What drives the typical case (3-point fit, p95 of the maximum error):

| Source | mg |
|---|---|
| ADC INL | 0.95 |
| Calibration-mass errors | 0.27 |
| Repeatability noise | 0.28 |
| Mechanics and magnetics | 0.01 |

The 76 mg reluctance quadratic, the θ0 term and the Kf(x) and PCR quadratics are all smooth, so any of the fits removes them.

p95 of the maximum error over 0–520 g, including 0.2 mg of hysteresis:

| ADC INL | 3-point | 5-point | **9-point** |
|---|---|---|---|
| Typical (3 ppm FSR), smooth | 1.17 | 0.70 | 0.67 |
| Max (12 ppm FSR), smooth | 4.21 | 1.92 | 0.69 |
| Typical, rough | 2.60 | 1.49 | 0.68 |
| Max, rough | 9.96 | 5.30 | 0.87 |

- **The 9-point set:** 0, 70, 120, 170, 250, 320, 370, 500 and 520 g, using only D-030's F1 20/50/100 g and E2 200/500 g masses. 5 repeats per point, degree-5 least-squares fit.
- **Proposal:** the 9-point adjustment becomes the commissioning baseline (D-064 said "≥ 3-point").
  - OI-34 still measures the INL shape.
  - The M3 PWM-DAC fallback stays for an INL that 9 points can't follow.

## 6. Span vs temperature (output §7–8): OI-02

### 6.1 What the in-situ model learns

- **One auto-adjust** (200 g check mass, 10 readings): 0.15 ppm random; 1.1 ppm at a 10× floor.
- **α from two adjusts 5 K apart:** 0.065 ppm/K = 0.02 % of α; 0.32 ppm/K at a 10× floor. Only adjusts made while |dT/dt| < 0.5 K/h are used.
- **One coefficient covers every span path that tracks the magnet sensor:**
  - magnet −336 ppm/K;
  - electronics α_e 1.7 typ / 6.6 max ppm/K (the shunt is the Z201: 0.2 ppm/K typical, ±0.6 worst case, per `research/GATE_B_SOURCING.md`);
  - coil axial position in the gap: 1.6 ppm/K with the coil set ±20 µm from the Kf maximum, 4.0 with ±50 µm;
  - optical null via θ0: 0.4 ppm/K.

  **Only the part that doesn't track is error** (20 % ASSUMED).
- **α of Sm2Co17 curves**, by ~1 ppm/K per K (ASSUMED). A linear model learned at 22.5 °C is 2.5 ppm/K off on a 20–30 °C test. The quadratic term needs ≥ 10 K of history.

### 6.2 MET-06 static (after compensation, no recalibration)

| Case | k = 2 |
|---|---|
| As designed today: linear model, α_e typ, coil ±50 µm | 5.3 ppm/K FAIL |
| Quadratic model (≥ 10 K of history) | 2.05 ppm/K, marginal |
| Plus B4 heat test (α_e measured separately) and coil set ±20 µm | **1.33 ppm/K OK** |
| Worst electronics (α_e max), quadratic model | 3.3 ppm/K FAIL |
| TC16 magnet, linear model | 2.3 ppm/K |

### 6.3 Span error at 520 g in use, with auto-adjust (MET-07)

| Term | ppm (k = 1) |
|---|---|
| Check-mass value (transfer from the external E2 500 g), systematic | 0.32 |
| One adjust, random | 0.15 |
| Drift since the last adjust (≤ 0.5 K × residual coefficient) | 0.20 |
| Magnet lag after a ramp change (compensated) | 0.50 |
| Nonlinearity drift extrapolated from 200 g to 520 g | 0.35 |
| Aging between adjusts (4 h trigger, after 100 h burn-in) | 0.13 |
| Magnetic hysteresis after full-scale or pan-absent excursions (ASSUMED) | 0.30 |
| Coil-power regulation error not seen by the sensor | 0.29 |
| Shunt PCR not settled at read time | 0.26 |
| B4 heat that rises with load (46 mW with option C), after B4 TMP117 compensation | 0.16 |
| **Total** | **0.91 ppm = 0.47 mg; k = 2: 0.94 mg** |

**Sensitivity (k = 2 at 520 g):**
- one-sided lag: 1.35 mg;
- 5 K/h ramps: 1.52 mg;
- 10× floor: 1.47 mg;
- rough max INL drifting: 1.56 mg;
- trigger at 1 K: 0.99 mg; at 2 K: 1.22 mg;
- coil power 30 mW (the Ø30 × 10 rig magnet plus a realistic coil fill, review finding F-01): 1.01 mg.

**Throughput:** at 2 K/h, a 0.5 K trigger means 4 adjusts per hour (5 % of the time at 45 s each); a 2 K trigger means 1.2 %. **Dosing loads see 1/50 of every span term**, so the dosing profile can use a 2 K trigger.

### 6.4 OI-02 verdict

- **Operating spec: closed by analysis** with the standard Sm2Co17 magnet, ~1 mg at 520 g (k = 2). The TC magnet stays the fallback.
- **MET-06 as written** (static, no recalibration) is met only under the §6.2 conditions. Gate B options:
  - (a) keep MET-06, met after ≥ 10 K of history, which a deliberate +10 K enclosure soak at commissioning can provide;
  - (b) reword it to "with MET-07 auto-adjust active".

  **Recommendation: (a).** It costs only firmware and one commissioning step.
- **Rig confirmation:** a 15–30 °C span sweep measuring α, its curvature, lag τ and the non-tracking fraction.

## 7. Eccentric load (output §9)

- **MET-09:** 1.0 mg in x and 1.0 mg in y at 175 g, e = 25 mm, at the adjuster resolution (the mechanics allocation). Lock-in stability (OI-25) stays the open risk.
- **Dosing:** with the coarse adjust, 0.30 mg (x) and 0.28 mg (y) at 10 g, e = 5 mm. **With y unadjusted, 125 mg.** The y adjuster must be set to ≤ ~10 µm for dosing too.
- **MET-03 at 500 g** also needs the fine adjust (§3).

## 8. Warm-up and dosing (output §10–11)

### Warm-up
- **Pot and magnet:** τ = 21 min. The 18 mW constant coil power warms them by 144 mK. The sensor tracks this, and 8.5 mK remains at 60 min.
- **Enclosure** (heat map from `POWER_BUDGET.md`): 4.8–6.8 W of internal heat while dosing:
  - B1 1.0–1.7 W;
  - B2 0.67–1.02 W;
  - motors 1.69 W averaged over the cycle;
  - display 1.12–2.0 W;
  - B4 0.32 W;
  - cell 0.04 W.

  In one shared volume, the cell's ramp at 60 min is 2.1–2.9 K/h, just outside the 2 K/h design ramp. With a separated cell compartment, which takes B4, the cell and ~30 % of the rest, it is 0.7 K/h. The cell compartment can take at most ~4.7 W. UA and τ are ASSUMED until the packaging study.
- → Keep mains, controller and motor heat out of the cell compartment.
- Dosing is usable after ~10–20 min, because the zero is re-tared every throw.

### Dosing
- **3σ of a 10 g charge:** 1.44 mg with baseline hinges and 0.93 mg with light hinges (0.022 / 0.014 gr).
- It is set by null drift and static charge, not by span or magnet drift.

## 9. Lever ratio (output §12)

| | r = 15 | r = 20 |
|---|---|---|
| Coil arm b | 150 mm | 200 mm |
| Constant coil power P0 (Recoma 28) | 18 mW | 10 mW |
| Zero error per 100 nm of null drift, baseline / light hinges | 0.42 / 0.20 mg | 0.33 / 0.16 mg |
| MET-08 (k = 2), baseline hinges | 0.87 mg | 0.70 mg |
| Counterweight (option U) | 187 g | 310 g |
| Lever bending modes, same section | 1.00 | 0.56× |
| Cell length | — | +50 mm |

**→ Keep r = 15 and use the light hinge set.** It gives MET-08 0.46 mg (k = 2), better than r = 20 with baseline hinges.

## 10. Requirements this puts on the rest of the design (PROPOSED)

| # | Requirement | Why | Where |
|---|---|---|---|
| 1 | Optical-null drift ≤ 100 nm per 60 s; null-to-vane thermal path ≤ 0.5 µm/K | MET-08, dosing | OI-31; Phase 4 sensor mount |
| 2 | Coil axial position on the Kf maximum within ±20 µm. Find it by measuring span at ±0.3 mm offsets and taking the vertex, which resolves the position to ~0.1 µm | Coil-position tempco: 1.6 vs 4.0 ppm/K | Rig; coil adjuster |
| 3 | Reference the optical sensor to the pot, or match the sensor post's expansion to the pot's | Removes the coil-position path | Phase 4 |
| 4 | Fine corner adjust for MET-03 at 500 g; y adjuster set even for dosing | §3, §7 | Rig procedure |
| 5 | Magnet sensor: TMP117 bonded to the pot base directly under the magnet, or on the magnet face through a Ø3–4 mm laser-cut hole. Continuous conversion, 8× averaging, 8 Hz. Firmware 120 s ramp-fit estimator with lag compensation | 0.25 mK; 9 s two-sided lag | B4, firmware |
| 6 | Auto-adjust (MET-07) enabled as an explicit policy. Triggers: ΔT_magnet ≥ 0.5 K in the precision profile, ≥ 2 K in dosing; ΔT_B4 ≥ 1 K; 4 h elapsed; after an overload or pan-absent event; at power-up after warm-up. Learn α only from adjusts made while \|dT/dt\| < 0.5 K/h. Bounds and an audit log | §6 | Firmware, `CALIBRATION.md` (OI-37) |
| 7 | In-situ span model S0·(1 + α1ΔT + α2ΔT²) + α_e·ΔT_B4, fitted by regression over the adjust history. α_e comes from a B4-only heat test at commissioning: warm B4 by ~3 K while the cell stays steady | MET-06 | Firmware, commissioning |
| 8 | 9-point linearity adjustment with the D-030 masses, degree-5 fit | MET-04 | Commissioning |
| 9 | Load-adaptive settling window: 0.33 s below ~100 g, up to 0.9 s above | MET-03 at 500 g | Firmware (D-068) |
| 10 | B4 burn-in ≥ 100 h before commissioning | Reference aging 0.13 ppm per 4 h after burn-in | Build procedure |
| 11 | Cell compartment ramp ≤ 2 K/h after 60 min, i.e. ≤ ~4.7 W into it; motors, display and mains heat stay outside it. No heat sources inside the draft shield; air at the pan ≤ ~1 cm/s | MET-10, drafts | Packaging, power budget |
| 13 | B4 layout: coil driver and heater H-bridge thermally away from the reference, ADC and shunt, with the B4 TMP117 at the reference. Load-dependent B4 heat is 46 mW with option C, 91 mW with 12 V LDOs | Span term 0.16 ppm after compensation | Phase 4 B4 layout |
| 12 | Conductive, PE-bonded pan and a dissipative shield; measure the static term | Dosing static (0.1 mg ASSUMED) | Rig |

## 11. Contradictions and changes to log

1. **`docs/firmware/CALIBRATION.md` treats the internal mass as "verification by default"**, with span correction only under an explicit policy. MET-07 (PROPOSED), D-062 and this budget need auto-adjust enabled. → Gate B decision (OI-37).
2. **`CALIBRATION.md` still describes the old 250 g design:** a 0–250 g nominal sequence and the ADS1262. → Update with the 9-point set in Phase 4 (OI-20).
3. **D-064's "≥ 3-point linearity adjustment"** becomes a 9-point baseline.
4. **EMFC_MECHANICS §8 (1.7 ppm per adjust) and MAGNET_COIL §13 (α to 0.14 %)** both used σ = 1 mg, the MET-03 limit. The loop predicts 0.077 mg at 200 g, giving 0.15 ppm and 0.02 %.
5. **EMFC_MECHANICS §3 checked the dosing eccentric case in x only.** y unadjusted is 125 mg at 10 g.
6. **D-068's single settling window** becomes load-adaptive above ~100 g.
7. **MET-06's "without recalibration"** holds only after the in-situ model has learned α's curvature (≥ 10 K). → Gate B wording decision (OI-38).
8. **MAGNET_COIL §7's 19 s lag** is the one-sided bound. The two-sided estimate is 9 s.
9. **Review finding F-01:** the 4 × 292-turn coil doesn't fit its window when hand-wound randomly. The turn count will be set from a wound test section, ~240–255 per wire.
   - Km(A+B) becomes 2.7–2.95 and P0 20–24 mW; with the Ø30 × 10 rig magnet, ~30 mW.
   - The budget holds: 1.01 mg at 520 g (k = 2).
11. **Power budget:** B4's heat rises with load: 46 mW from empty to 520 g with the proposed 6 V supply (option C), 91 mW with the drawn 12 V LDOs. That makes it a span term, worth 0.16 ppm after B4 TMP117 compensation.
10. **Sourcing:** D-064's shunt "TCR ≤ 0.2 ppm/K" is a typical value. No catalogue foil part guarantees it as a maximum; the Z201's worst case is ±0.6 ppm/K. The B4 heat test and auto-adjust cover it.

## 12. Rig checks (Phase 3)

- Span vs temperature over 15–30 °C: α, its curvature, lag τ and the non-tracking fraction (OI-02).
- ADS1263 INL shape, using the 9-point mass set (OI-34).
- Optical-null drift over 60 s and against temperature (OI-31).
- Floor PSD, using the EMFC at 520 g as the seismometer (OI-32).
- Check-mass adjust repeatability (10 readings) against the 0.15 ppm prediction.
- Static: reading change with charged powder in the pan (OI-39).
- Ascending/descending hysteresis, and magnetic hysteresis after a full-scale or pan-absent excursion.
- Kf-maximum location by the ±0.3 mm method.

## 13. Assumptions to replace with data

- **Environment:**
  - floor spectrum at 1×, 3× and 10× the servo study's quiet slab;
  - cell ramps of 1 / 2 / 5 K/h;
  - drafts 0.05 / 0.1 mg;
  - static 0.1 mg;
  - placement scatter 1.5 mm;
  - powder centroid scatter 2 mm.
- **Thermal:**
  - α curvature 1 ppm/K² (Sm2Co17) and 0.3 ppm/K² (TC16);
  - non-tracking fraction 20 %;
  - lag-compensation residual 30 %;
  - enclosure internal heat 2–8 W.
- **Parts:**
  - INL shapes, and an INL tempco of 0.7 %/K;
  - reluctance tempco 0.05 %/K plus α;
  - PCR 80 % unsettled at read time;
  - magnetic hysteresis 0.3 ppm;
  - mechanical hysteresis 0.2 mg;
  - E2 certificate uncertainty ~1/3 of MPE.
- **Mechanics:**
  - null drift 100 nm per cycle;
  - null thermal path 0.5 µm/K;
  - coil axial shift 0.9 µm/K.
