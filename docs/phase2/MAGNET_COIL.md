# EMFC magnet and coil, first-order design (Phase 2, OI-24)

Rev 1 · 2026-11-22 (§13 added: alternatives the user asked about)
- Numbers come from `analysis/magnet_coil.py`, output in `analysis/magnet_coil.out.txt` (§ numbers below match it).
- Method (outside method, not from the user's textbook): 2-D axisymmetric finite-volume magnetostatics with a linear recoil-line magnet. Steel is linear (μr 1000) in the sweep. The selected design is re-solved with non-linear steel on an ASSUMED conservative mild-steel B-H curve.
- The solver matches closed forms to within 0.05 %: a bare magnet's on-axis field and a thick solenoid's centre field. Force by the Lorentz integral matches dλ/dz within 0.02 %.
- Plant inputs are from `EMFC_MECHANICS.md` §10.

## 1. Bottom line

- **Km is not the constraint.** A fixed SmCo pot with a moving coil gives Km 4.4 N/√W (Recoma 28, nominal Br) and 3.2 N/√W (temperature-compensated EEC TC16 at minimum Br), against the 1.6 N/√W needed. The pot uses an Ø31.75 × 12.7 mm (1¼ × ½ in) disc, 56 mm OD.
  - Full-scale coil power is **6 mW** (12 mW with the TC grade), against 45 mW assumed in the mechanics study.
  - r = 15 stands, and **OI-24 is closed (PROPOSED)**.
- **The constraint is temperature, i.e. OI-02.** Standard Sm2Co17 moves the span by −336 ppm/K net, which is 2.6 g at 520 g over the 15 K garage swing. Reaching ±2 ppm/K requires three things together:
  - α known to 0.6 %;
  - the magnet-temperature sensor tracking within ~6 mK;
  - constant coil power.

  The sensor lag alone is ~3.5 ppm (1.8 mg at 520 g) at a 2 K/h ramp. A TC grade (net ≈ +4 ppm/K) turns this into a 50 % problem. That is the strongest finding here.
- **Coil power must be held constant for full-range weighing.** Otherwise the magnet temperature follows the load (6.7 mg at 520 g with Recoma 28). Dosing loads of a few grams don't need it.
- **Moving coil, fixed magnet.** A moving pot would add 1.3 kg of counterweight and multiply the inertia J by 7.
- **Buildable without CNC.** Every steel part is a flat laser-cut disc or ring. The ½ in magnet equals two ¼ in wall rings, and the gap is set with a removable gauge while the adhesive cures (loudspeaker practice).

## 2. Requirement

F_FS = 520 g × 1.02 (trim, D-051) × g / 15 = **0.347 N**.
- With 1.2× headroom [ASSUMED], F_max = 0.416 N.
- P = F²/Km².
- Lever stops at ±3 mrad move the coil ±0.45 mm (D-050 geometry).

## 3. Magnet grades [DS]

| Grade | Type | Br typ / min (T) | μ_rec | α(Br) | Availability |
|---|---|---|---|---|---|
| Arnold Recoma 28 (or any 28–30 MGOe 2:17) | Sm2Co17 | 1.10 / 1.04 | 1.09 | −350 ppm/K (20–150 °C) | Off-the-shelf discs |
| Arnold Recoma 2:17 Stab "0" | Sm2Co17, compensated | 0.83 / — | 1.05 | ≈ 0 (−60..+100 °C) | Made to order |
| EEC 2:17-TC16 | Sm2Co17, compensated | 0.825 / 0.785 | 1.06 | −10 ppm/K typ (−50..150 °C) | Made to order |
| Arnold Recoma 22 | SmCo5 | 0.94 / 0.90 | 1.03 | −450 ppm/K | Off-the-shelf |

The EEC table text was misaligned when extracted. Values were mapped row by row using (BH)max ≤ Br²/4μ0, and **need confirming with EEC**.

Arnold's Stab sheet says SmCo is "the material of choice in measuring systems such as electronic balances". It notes that compensated grades trade remanence for coefficient, and that they are made "according to specific customer requirements". NdFeB is excluded: SmCo is CONFIRMED, and NdFeB is about 3× worse in α.

## 4. Selected geometry (PROPOSED)

```
        coil (moving, on 0.3 mm Al bobbin)
              ┌──┐            ┌──┐
   top plate ═╡  ╞═══╗    ╔═══╡  ╞═ top plate (ring, 4.76)   ← gap 4.0 mm radial
   pole  ████ │  │   ║    ║   │  │ ████
   magnet▓▓▓▓    wall rings 2 × 6.35 (ID 47.7, OD 56)
   base  ████████████████████████████ (6.35)
```

| Part | Size | Note |
|---|---|---|
| Magnet | Ø31.75 × 12.70 mm Sm2Co17, axial, 83 g | Fallback Ø25.4 × 12.7 (most common disc): Km 3.4, TC 2.4, 50 mm OD |
| Pole piece | Ø31.75 × 4.76 steel disc | Gap height = 4.76 mm |
| Top plate | Ring ID 39.75 × 4.76 | Gap 4.00 mm = 0.35 clearance + 0.30 bobbin + 3.0 coil + 0.35 clearance |
| Wall | 2 × 6.35 rings, ID 47.75 | Stack height = magnet height (imperial magnet, imperial plate) |
| Base | 6.35 disc/square | Sized for ≤ 1.1 T mean; 1.3 T saturated the corner under the magnet edge |
| Pot | OD 56 mm (or square for dowels), height 23.8 mm, steel 265 g | Fixed to the base plate; its mass is outside the plant |
| Coil | ID 33.05, OD 39.05, height 7.14 (1.5 × gap height), centred on the top-plate mid-plane | 13.8 g with bobbin (the plant assumed 15 g) |

Sweep (§3 of the output): 5 magnets × 3 plate thicknesses × 7 radial builds × 6 coil heights, with each coil placed at its Kf maximum.

Selection rule: the highest Km with coil ≤ 15 g and pot OD ≤ 60 mm, while the TC drop-in still keeps Km ≥ 1.6 × 1.25. Why: the pot is fixed, so its mass is free, while every thermal error scales with coil power ∝ 1/Km².

## 5. Results (selected design, non-linear steel)

| Quantity | Value |
|---|---|
| Km: Recoma 28 nom / min Br, TC16 nom / min Br | 4.43 / 4.19, 3.35 / 3.18 N/√W |
| Full-scale power (Recoma 28 / TC16 min) | 6.1 / 11.9 mW; at F_max 8.8 / 17 mW |
| Mean gap B; average B in winding; leakage factor σ | 0.54 T; 0.50 T; 2.5 |
| Steel mean B (pole rim / base / wall); 99th percentile | 0.80 / 1.18 / 0.95 T; 1.31 T |
| Kf vs coil position | −0.12 % at ±0.2 mm, −0.63 % at the stops (±0.45 mm). A 100 nm null drift with the coil set within ±0.05 mm of the maximum gives 0.4 ppm |
| Inductance (estimate, frozen permeability) | 79 nH/turn², dL/dz −2.7 µH/m/turn² |
| Reluctance force ½I²dL/dz at FS | 144 ppm of FS, ∝ load² with linear drive; **constant with ±I0 PWM** |
| Eddy damping, solid Al bobbin | 2.0 N·s/m at the coil → ζ ≈ 2.6 open loop (empty). One axial slit → ζ ≈ 0. Choice: servo study |
| Stray field at 50 / 100 / 140 mm | 2.6 / 0.28 / 0.11 mT. Forces on 1095 blades and the check mass ~1e-11 N (negligible). **Non-magnetic fasteners within ~100 mm** |
| Coil dipole in a 20 µT horizontal field (unshielded bound) | 2.4 ppm of FS torque; up to 4.8 ppm span change if the balance is turned 180° → treat as span; auto-adjust after moving |

**Winding: SUPERSEDED by D-065** (4 × AWG 37 in hand: two in series for force, two as a non-inductive heater; Km 3.13, see `SERVO_MEASURAND.md` §2). The single-winding numbers below remain the all-series reference. **Original proposal: AWG 34.** N ≈ 585, R 57 Ω, Kf 33 N/A, I_FS 10.4 mA, V_FS 0.59 V, L ≈ 27 mH (estimate), L/R 0.48 ms. Fill factor 0.55 is ASSUMED (random hand winding). Orthocyclic winding would give +13 % Km.

| AWG | N | R Ω | Kf N/A | I_FS mA | V_FS V |
|---|---|---|---|---|---|
| 30 | 231 | 8.9 | 13.2 | 26.3 | 0.23 |
| 32 | 368 | 22.4 | 21.0 | 16.5 | 0.37 |
| **34** | **585** | **56.7** | **33.3** | **10.4** | **0.59** |
| 36 | 930 | 143 | 53.0 | 6.5 | 0.94 |

## 6. Thermal and constant power (input to the servo/measurand study)

- Coil rise above the pot: 58–84 mK. Gap conduction is 79 mW/K; the mount is ASSUMED 20 mW/K.
- Pot rise: ~50–70 mK at an ASSUMED 8 K/W to ambient.
- With linear drive, 0 → 520 g changes the magnet temperature by ~37 mK. That is 6.7 mg at 520 g with Recoma 28, settling over minutes like creep. **Constant power is required at full range** (D-058).

| Option | Km | Constant power | Comment |
|---|---|---|---|
| a) ±I0 bipolar PWM, one winding | 4.43 | 9 mW | Measurand = duty × I0 (HRTIM question, OI-03). Also makes the reluctance term constant |
| b) Split winding 80 % force + 20 % non-inductive bifilar heater | 3.96 | 11 mW | Linear current measurand (ADS1262 + shunt) is kept. Heater is in the same volume |
| c) Heater resistor on the bobbin | 4.43 | — | Heat enters elsewhere: residual gradient and a transient after each load step |

Coil temperature can come from the **winding resistance** (+3930 ppm/K) measured by the drive, so the moving coil needs no sensor or extra leads (D-060).

## 7. Span tempco and OI-02

| Grade | Net Kf tempco* | 15 K swing at 520 g, uncompensated | α accuracy for 2 ppm/K | Sensor-lag error at 2 K/h |
|---|---|---|---|---|
| Recoma 28 | −336 ppm/K | 2.6 g | 0.6 % | 3.5 ppm (1.8 mg at 520 g) |
| EEC TC16 | +4 ppm/K | 31 mg | 50 % | 0.04 ppm |
| Recoma 22 (SmCo5) | −436 ppm/K | 3.4 g | 0.5 % | 4.6 ppm |

\*Includes an ASSUMED +14 ppm/K geometric part (copper coil +17, steel gap +12 partly cancelling). The all-aluminium lever ratio doesn't change with uniform temperature.

The lag model puts the sensor on the base under the magnet. The magnet's conduction time constant is 4h²/(π²a) = 19 s, using SmCo k 10 W/m·K and c 350 J/kg·K [DS].

Dosing loads of 10 g see 1/50 of these numbers, so either grade works for Config E dosing.

The full-range metrology goals (D-032: ±2 ppm/K span drift, MET-09) are where the grade matters. With Recoma 28 they depend on three things at once:
- firmware compensation, with α learned from check-mass adjustments at different temperatures;
- auto-adjustment at ΔT ≥ 0.5 K;
- a lag-compensated magnet temperature.

A TC grade gives the same scheme a 50× easier job, so the pot is designed for **TC as a drop-in** (same size; Km still ≥ 3.2).

## 8. Moving coil vs moving magnet

| | Moving mass | Extra counterweight | Added J | Fulcrum load |
|---|---|---|---|---|
| Moving coil | 14 g | 52 g | 3 900 g·cm² (already in the plant's 15 g) | +66 g |
| Moving magnet (pot on lever) | 348 g | 1 307 g | 99 000 g·cm² (7.4× the plant J) | +1.7 kg |

Moving magnet is **rejected**. Blade stiffness grows with tension (D-050), and the stray field would move with the lever.

Moving-coil leads: two AWG 44 loops about 10 mm long, crossing at the fulcrum axis, add 0.012 % of K_tot each. AWG 40 adds 0.08 %.

## 9. Fabrication without CNC (PROPOSED)

- All steel parts are laser-cut plate: discs, rings and a square-outline base/top for dowel holes.
  - Grade, thicknesses and B-H curve are to be confirmed against what SendCutSend actually stocks (OI-28).
  - Low-carbon cold-rolled steel is preferred over A36 for permeability.
- Concentricity target is ≤ 0.1 mm against 0.35 mm clearance. The pole piece and magnet are bonded to the base while a removable gap gauge holds them centred in the top-plate ID (four 4.0 mm shims or a cut ring). The gauge is assembly tooling, not part of the cell.
- Magnet-height mismatch (metric magnet with imperial plates) is made up with steel shim stock under the magnet.
- Bobbin: thin-wall Al tube, or 0.3 mm Al rolled with a bonded seam. An open seam is the same as the "slit" (no damping) option.
- The coil is wound on the bobbin with a simple stepper-driven jig and bonded to a flat Al bracket on the lever.
- Handling: SmCo is brittle and the pot snaps together hard. Use a non-magnetic assembly fixture.

## 10. Contradictions and changes to log

1. **EMFC_MECHANICS assumed Km 1.6 at r = 15. The magnet/coil study finds 3.2–4.4.**
   - Keep r = 15, because the zero error per 100 nm of null drift improves with r (0.42 → 0.33 mg going to r = 20).
   - The Km surplus could pay for r = 20 at ~3.5 mW if the zero-drift allocation (0.3 mg) stays tight. That is left to the error budget, not decided here.
2. **CLAUDE.md lists "magnet, coil and ambient temperature sensing".** This study proposes measuring coil temperature through winding resistance, not a sensor on the moving coil. That is a change of method, flagged for confirmation (D-060).
3. **D-032's ±2 ppm/K span drift goal is at risk with standard Sm2Co17.** It needs 0.6 % compensation accuracy plus lag compensation. A TC grade or relaxing the goal are the alternatives. This goes to the error-budget study; nothing is picked here.
4. **Linear current drive adds a 144 ppm FS load² term (reluctance force).** It is deterministic and corrected by a linearity adjustment, and it vanishes with ±I0 PWM. This is one more input to the analog-vs-PWM choice.

## 11. Rig checks (Phase 3)

- Gap B with a gaussmeter at 3 heights, against the solver.
- Kf by F1 weights at 3 coil positions; find the Kf maximum.
- Coil R and L (LCR meter at 100 Hz / 1 kHz).
- Damping ζ with solid vs slit bobbin, from step response.
- Pot and magnet thermal time constants with the TMP117 on the base.
- Span tempco in a 15–30 °C sweep (OI-02 data).
- Stray field at the blade and check-mass positions.

## 12. Assumptions to replace with data

- Fill 0.55.
- Clearance 0.35 mm per side.
- Bobbin 0.3 mm 6061 (σ 2.3e7 S/m).
- Steel B-H curve (conservative mild steel).
- Plate thicknesses 3.18 / 4.76 / 6.35 / 9.53 / 12.7 mm.
- Thermal values: R_pot 8 K/W, mount 20 mW/K, h 5 W/m²K.
- Geometric tempco +14 ppm/K.
- Garage ramp 2 K/h.
- Headroom 1.2.

## 13. Alternatives the user asked about (output §12)

| Idea | Result | Verdict |
|---|---|---|
| Buy a standard SmCo with a known/measured drift | Vendors publish only a typical α (Arnold −0.035 %/K), not a per-part tolerance. We don't need one: two check-mass adjustments 5 K apart measure α of the **finished cell** to 0.14 % (0.36 % at 2 K), better than the 0.6 % needed and including coil, steel and electronics | **Baseline** (D-062). The remaining limit is the magnet's temperature at each instant (lag ~3.5 ppm at 2 K/h), not the coefficient |
| Passive shunt of temperature-compensator alloy (Carpenter Temperature Compensator 30, 30 % Ni-Fe, stock strip/bar) | Shunt diverts flux fraction s and diverts less as it warms. s × k = 336 ppm/K → 1.7–6.7 % of gap flux for k = 2–0.5 %/K, costing the same % of Km (Km has 2.7× margin). k is unpublished. Residual set by the curvature of μ(T), ASSUMED ±10–30 ppm/K | **CANDIDATE rig experiment** (OI-30). Off-the-shelf alternative to a custom TC magnet |
| Two coils on the lever | Opposed pair for constant power: 18 mW vs 9 mW for ±I0 PWM (half the copper each). For drift: both coils sit in the same field, so any force or ratio still carries B(T) | No help for drift. Constant power stays with D-058's options |
| Electromagnet with closed-loop field current | Needs ~1 800 A-turns for the same Kf. **22 W** in an enlarged OD 100 mm pot, 180 W (burns out) in the same envelope, against a 6 mW coil budget. The SmCo disc gives the equivalent 10 160 A-turns of MMF for free. Also: the field depends on a second ppm-regulated current, plus iron hysteresis (power-up state) and iron permeability vs temperature | **Rejected**: a far bigger accuracy problem (heat) |
| Magnet + trim winding | 43 A-turns, up to 104 mW, varying with temperature. Still needs the magnet temperature (a ppm field sensor isn't available at this price), so it only moves the firmware correction into hardware | **Rejected** |
