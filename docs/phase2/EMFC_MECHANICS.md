# EMFC mechanics feasibility study (Phase 2, Config E)

Rev 0 · 2026-11-21 · Effort: max · Numbers: `analysis/emfc_mechanics.py` → `analysis/emfc_mechanics.out.txt`. Everything here is **PROPOSED for Gate B** unless it is marked otherwise.

No textbook index is in the repo, so **every method here is an outside method** (beam theory, beam-column, Euler buckling, virtual work, lumped plant), named in the script header. Material properties are ASSUMED handbook values until the purchased stock's certificate is in hand. The mechanics allocations in §5 are provisional shares; the error-budget study replaces them.

## 1. Bottom line

> **Update 2026-11-22 (error budget, D-071..D-074):**
> - r = 15 stays, and the light hinge set (§2b) becomes the baseline for MET-08 margin.
> - §8's 1.7 ppm per check-mass adjust used σ = 1 mg (the MET-03 limit). The loop predicts 0.15 ppm.
> - §3's dosing eccentric case covered x only. **y must be adjusted for dosing too**: 125 mg at 10 g unadjusted.
> - The fine corner adjust is also needed for MET-03 at 500 g.
>
> See `ERROR_BUDGET.md`.

> **Update 2026-11-22 (D-057):** the magnet/coil study (`MAGNET_COIL.md`) reaches Km 4.4 N/√W (3.2 with a temperature-compensated grade), so the r = 15 condition (Km ≥ 1.6) is met with margin and full-scale coil power is ~6 mW, not 45 mW. Coil + bobbin is 13.8 g (15 g assumed here).

1. **A DIY flat-part EMFC cell can reach AD520 class, but not with SendCutSend flexures.**
   - SendCutSend's only spring stock is 1075 at 0.381 mm [V, sendcutsend.com 2026-11-21]. As measuring hinges it is **~350× too stiff**: 107 mg of zero error per 100 nm of optical-null drift.
   - The hinges must be **purchased thin shim blades**: 1095 spring steel or 301 full-hard, 0.04–0.10 mm. That is "flexure stock", which the master prompt allows as a buy.
   - Every other cell part stays flat SendCutSend aluminium (D-004 holds).
2. **Architecture (CANDIDATE):** a single-lever Roberval cell.
   - Load arm a = 10 mm, **r = 15** (coil arm 150 mm). r = 15 is conditional on the magnet/coil study reaching **Km ≥ 1.6 N/√W**, which gives 45 mW at full load. Fall back to r = 20 if Km < 1.2.
   - Plant: soft, ~1 Hz open loop. The servo supplies nearly all the stiffness.
3. **Cut tolerance can't set corner-load accuracy.** SendCutSend's ±0.127 mm leaves the x eccentric error ~265× over the MET-09 allocation.
   - MET-09 as written needs **two fine corner adjusters**: x hinge-height ≤ 1 µm, y coupling lateral ≤ 0.4 µm.
   - A fine screw through a 50:1 flexure-lever reducer cut from plate gives 0.1 µm per 5°. **Lock-in stability is the open risk.**
   - Dosing alone needs only a ≤ 25 µm coarse adjust.
4. **Counterweight (OI-15): option U** (counterweight trims the tare only).
   - Preloading to W/2 does **not** reduce the tared span error from magnet tempco.
   - It adds ~104 mg/K of zero drift and only cuts peak coil power 4×. This contradicts the requirements' top-risk note (§9 below).
5. **Check mass (OI-07): 200 g CANDIDATE.**
   - Span-adjust uncertainty ≈ 1.7 ppm (0.9 mg at Max).
   - It sits on a kinematic seat on the load receptor and is lowered by a geared stepper on the reserved CM_STEP/DIR/HOME lines, interlocked by SAFE.

## 2. Options ranked

| # | Option | D-004 / no-CNC | Metrology potential | Main risk | Verdict |
|---|---|---|---|---|---|
| **B** | **Built-up flat-plate Roberval: SendCutSend Al plates + purchased shim blades, bolted/bonded clamps, corner and null adjusters** | Yes (blades are purchased flexure stock) | AD520 class feasible (§5) | Adjuster lock-in stability; clamp microslip; assembly skill | **RECOMMENDED (CANDIDATE)** |
| B2 | B with a two-stage lever (r1 × r2) | Yes | Same, lower coil power or a shorter cell | Twice the pivots, couplings and adjusters | HOLD, only if Km < 1.2 forces r > 20 |
| A | Purchased EMFC weigh cell (used monoblock, or OEM such as Sartorius WZB: 250 g / 0.1 mg / 0.6 s [V, local datasheet]) | Purchased cell | Best | Not DIY (defeats Config E's purpose), interface unknown, supply luck; price not checked | **Fallback / benchmark** |
| D | Monolithic notch-hinge block (wire EDM or milled) | Needs CNC | Best (no joints) | Excluded by the no-CNC rule | OPTIONAL only (not designed) |
| C | B with SendCutSend 1075 0.381 mm blades only | Yes | Fails: 107 mg / 100 nm | — | **REJECTED** |
| E | Knife edges / ball or jewel pivots | Yes | Friction and hysteresis at mg level, wear | — | REJECTED |
| F | Printed structure | Violates D-004 | Polymer creep and hysteresis | — | REJECTED |

## 3. Recommended architecture (B, CANDIDATE)

```
                    pan (conductive, removable) on a kinematic seat
                               │
        ┌──── load receptor (rigid Al) ────┐ ← check mass (200 g) lowered onto a 3-ball seat here
        │                                  │
  upper guide link ═══ blades ═══ fixed post ← x corner adjuster (hinge height, 50:1 flexure reducer)
  lower guide link ═══ blades ═══ fixed post
        │  h_g = 60, L_g = 70, blade pairs at ±30 mm (w = 60)
        │
  coupling: pitch blade + 90° roll-release blade in series ← y corner adjuster (lateral position)
        │ a = 10 mm
  ══════╪══════════════════ lever (Al, 25 mm deep, truss-pocketed) ══════════════════╪══ coil ═ vane
        ▲ fulcrum: 2 tension blades                                       b = 150 mm  │ optical null
  counterweight (height-adjustable) at 40 mm on the load side                         │ (height adj.)
                                     base: MIC-6 plate, 3-point feet
```

| Element | Baseline | Why |
|---|---|---|
| Base | MIC-6 cast tooling plate, 0.375–0.5 in, on 3 adjustable feet | Cast and stress-relieved, so it's the flattest stock SendCutSend offers. Flatness isn't published, so **check it with a straightedge and hand-lap the clamp areas**. 3 points avoid rocking |
| Plates (posts, links, receptor, lever, clamps) | Al 6061-T6, one material | Lowest α/k (0.14 µm/W vs 1.1–1.3 for SS/Ti). Heat-flow gradients matter more than α itself (§5, item 9) |
| Pivot blades (fulcrum, coupling) | 1095 or 301 shim, **0.04–0.05 mm**, 6–10 mm wide, free length 2 mm | Zero-drift budget (§5, item 1). Stress ≤ 8 MPa at the stops, 60× yield margin |
| Guide blades | Same stock, **0.075–0.10 mm**, 10 mm wide, free length 1.5 mm | Roll stiffness scales with t³/λ³, but the guide reaches lever stiffness only through (a/L_g)² = 0.02 |
| Blade form | Rectangles cut from shim (shear or paper trimmer), clamped by bars with bolts **outside** the blade width and edge-located on dowels. Photo-etched dog-bone blades as the upgrade | No holes in a 0.05 mm blade; no laser heat-affected zone. Photo-etching is a purchased flat process, not CNC (quote needed) |
| Clamps | Bolted **and bonded** (thin epoxy bondline under high preload); preload ≥ 100 × shear/μ | 10 nm of fulcrum slip = 1 ppm of ratio (§5, item 7) |
| Coupling | Pitch blade + roll-release blade at 90° in series | Keeps the y-eccentric roll moment in the guide. Only 0.001 % reaches the lever (§5, item 3) |
| Coil | Moving coil at b = 150 mm, leads routed along the lever and across **at the fulcrum axis** (AWG 40–44 or a flex loop) | Lead stiffness is otherwise comparable to the hinges. Moving-magnet (no leads) goes to the magnet study |
| Stops | Lever stops ±3 mrad at the coil (±0.45 mm); receptor stops 20 µm beyond; transport lock | MET-11 (1 kg): blades unloaded by the stops; 60× yield margin even if only the lever stops engage |

## 4. Sensitivity models (first order, small motion)

| # | Mechanism | Model | Consequence |
|---|---|---|---|
| 1 | Hinge stiffness × optical-null drift δs | ΔW = K_tot·(δs/b_s)/a | Sets blade t and b. Tension stiffening K = √(EIP)·coth(kλ) has a floor that blade length can't lower (kλ ≈ 0.9 at full load) |
| 2 | x eccentric (pitch): fixed-side vs moving-side hinge spacing mismatch Δh | Receptor rotation rate dθ/dz = Δh/(L_g h_g). Virtual work on moment W·e gives **ΔW/W = e·Δh/(L_g h_g)** | Δh ≤ 0.96 µm for MET-09 |
| 3 | y eccentric (roll): guide carries M_x through S-bending of the lateral blade pairs. Blade stiffness asymmetry moves the roll-stiffness centre by y* | With the coupling at y = 0 holding z: **ΔW/W = 2·e_y·Δy/w²** | Δy ≤ 0.41 µm. y* can be ~4.5 mm from ±5 % shim tolerance, so the adjuster needs ±5 mm range |
| 4 | Residual hinge angle θ0 at null × K(P) | M = K(P)·θ0 at fulcrum and coupling | 127 ppm span per mrad (calibrated). Nonlinearity 0.36 mg at 0.5 mrad. d(θ0)/dT ≤ 3.9 µrad/K for 0.5 ppm/K |
| 5 | Gravity stiffness from vertical offsets | K_g = F·Δz | Loose (±2 mm for 20 % of K_tot). Used as a **tuning knob** via counterweight height |
| 6 | Lever bending under coil force | δ = F b³/(3EI) | 3 mg linear (span). 0.08 mg nonlinear |
| 7 | Clamp slip along x at the fulcrum | Δr/r = slip/a | 10 nm = 1 ppm, so no slip is allowed |
| 8 | Coil heat flowing along the lever | Δr/r = α·ΔT_arms, ΔT ∝ P ∝ W² | 0.26 mg nonlinearity at Km 1.6. **A constant-power coil drive** makes it a calibrated constant |

## 5. Results vs the provisional mechanics allocations

| Item | Allocation (PROPOSED) | Baseline result | Status |
|---|---|---|---|
| 1. Zero error per 100 nm of optical-null drift (full load) | ≤ 0.3 mg | 0.42 mg (0.05 × 10 mm pivots, 0.10 guide). **0.20 mg** with 0.04 × 8 mm pivots, 0.075 mm guide and a truss-pocketed lever | Closes with the lighter variant; δs = 100 nm is a conservative ASSUMED value |
| 2. x eccentric at 175 g, e = 25 mm | ≤ 1 mg (5.7 ppm) | 265 mg unadjusted → needs Δh ≤ 0.96 µm | **Adjuster required** |
| 3. y eccentric at 175 g, e = 25 mm | ≤ 1 mg | needs Δy ≤ 0.41 µm. Lever path 0.013 mg with a 3 mrad axis skew | **Adjuster required** |
| 4. Nonlinearity from θ0 | ≤ 0.5 mg | 0.36 mg at θ0 = 0.5 mrad | OK if θ0 ≤ 0.6 mrad (jig ~0.3 mrad + electronic setpoint trim) |
| 5. Span tempco (mechanics) | ≤ 0.5 ppm/K | 0.9 ppm per 0.1 K change in lever vertical gradient | Thermal symmetry and enclosure; **measure** |
| 6. Coil-heat nonlinearity | (part of linearity) | 0.26 mg at Km 1.6; 0.83 mg at Km 0.9 | Constant-power drive recommended (servo/measurand study) |
| 7. Overload 1 kg | no yield | 60× yield margin | OK |
| 8. Guide buckling under eccentric load | — | P_cr / P ≥ 350 | OK |

## 6. Adjustability (designed in, because cut tolerance is ±0.127 mm)

1. **x corner:** fixed-post upper hinge height. 50:1 flexure reducer, M3×0.35 or a purchased 1/4-80 adjuster. ±0.3 mm range, ~0.1 µm per 5°, with a lock.
2. **y corner:** coupling lateral position. Same reducer, ±5 mm range.
3. **Optical null height:** sets θ0 ≈ 0 mechanically. Final trim is electronic, through the servo setpoint (no mechanical work).
4. **Counterweight:** mass trim (slugs) and height (sets K_g, the open-loop stiffness).
5. **Coil position:** axial and radial centring in the gap.
6. **Level:** 3 feet with a bubble or tilt sensor. Level matters only through cosine and K_g.
7. **Stops:** set-screw stops for the lever and receptor.

## 7. Counterweight (OI-15) and check mass (OI-07)

**Counterweight, option U.** The counterweight trims the tare so the coil carries +2 % FS at an empty pan: 187 g at 40 mm on the load side, or less with a truss-pocketed lever.
- Option B (preload W/2) needs 249 g. Its only benefit is peak coil power: 45 → 11 mW at Km 1.6.
- Option B also adds W/2·α·ΔT of zero drift: ~104 mg/K at α = 4×10⁻⁴/K. That drift lands right where dosing works.
- The tared reading is W·(1 − αΔT) for any preload, so preload never helps span.

**Check mass, 200 g CANDIDATE.**
- Span u = √((σ/√N)² + u_cm²)/m. With σ = 1 mg, N = 10 and u_cm = 0.15 mg (transfer from the D-030 E2 200 g), that gives 1.7 ppm → 0.9 mg at Max. For comparison: 100 g gives 3.3 ppm (1.7 mg) and 500 g gives 0.9 ppm, but at 5 N of lifter load.
- **Material:** non-magnetic austenitic stainless (it sits near the SmCo field).
- **Seat:** a 3-ball kinematic seat on the load receptor, so the check covers the lever ratio as well as Kf and the electronics.
- **Actuator:** fork lift on a geared stepper (TMC2209 #4 on CM_STEP/DIR/HOME). The fork retracts ≥ 1 mm clear, and the motor is de-energized and gear-held while weighing.
- It is a span check/adjust, **not** traceable calibration (CAL-02).

## 8. What needs FEA and what is empirical

| Need | Method | Why |
|---|---|---|
| Blade stiffness and stress | Closed form (done) | Thin uniform blades are exactly the beam-theory case |
| Magnet circuit: B in the gap, Kf(x) uniformity, magnetic "spring" dKf/dx·I | **FEMM 2D axisymmetric (open tool)** | Gives Km (sets r) and a load-dependent stiffness term like item 1 in §4 |
| Lever and receptor modes, bolted-stack stiffness | Closed-form beams first; CalculiX only if the first mode is within 10× of the servo bandwidth | Plates are simple. Check when the servo bandwidth is set |
| Hysteresis, creep, clamp microslip, adjuster lock-in, optical-null drift, corner load | **Empirical on the rig** | Can't be predicted to the needed level by any affordable model |

## 9. Contradictions and findings to log

1. **D-004 vs flexure stiffness.** SendCutSend-only flexures are infeasible (§1). Resolution: purchased shim blades, which the master prompt's "flexure stock if needed" allows. → **User confirmation requested.**
2. **The requirements' top-risk note vs the result.** The note in `01_REQUIREMENTS.md` suggests a preloaded counterweight so the coil "carries at most half the range". For tared readings that doesn't reduce magnet-tempco span error; it only halves peak current. The OI-02 closure must come from compensated SmCo grades, auto-adjust and characterization (error-budget study).
3. **MET-09 vs dosing (OI-16).** MET-09 at e = 25 mm needs sub-µm adjusters. Dosing at e ≤ 5 mm needs only a 25 µm adjust. Recommend keeping MET-09 as the full-range goal, verified in Phase 7. The Gate C rig exit uses the dosing case, plus measured adjuster resolution and stability.
4. **Lever ratio depends on Km.** The magnet/coil study must report Km = Kf/√R before r is frozen.

## 10. Plant estimates for the servo study (to be measured)

| Quantity | Empty | Full (520 g) |
|---|---|---|
| J about the fulcrum | 13 400 g·cm² | 13 900 g·cm² |
| K_tot (blades, tension-stiffened), plus adjustable K_g | 59 mN·m/rad | 66 mN·m/rad |
| Open-loop f_n | 1.06 Hz | 1.10 Hz |
| Coil force per pan mg | 0.654 µN (131 nA at Kf 5 N/A) | full scale 68 mA at Kf 5 N/A |
| Vane | 160 mm radius: 1 µrad = 160 nm | — |

## 11. Rig measurements (Phase 3 Config E characterization plan inputs)

- K_tot and K_g, from the coil-current vs null-offset slope at 0 / 260 / 520 g.
- f_n and damping, by a Bode sweep using the scope's AWG.
- Optical-null drift δs over 60 s and over temperature.
- Corner load, before and after adjustment, and adjuster lock-in stability after thermal cycling.
- Hysteresis (0 → Max → 0) and creep (Max held 30 min).
- Coil-lead stiffness.
- Span tempco.
