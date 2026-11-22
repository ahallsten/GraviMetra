# Trickler mechanics concept (Phase 2, OI-14)

Rev 0 · 2026-11-21 · Status: **PROPOSED (Gate B)**. Numbers come from `analysis/trickler_sizing.py` → `analysis/trickler_sizing.out.txt`. Kernel geometry and densities are **ASSUMED** until measured (action at the end).

## 1. Concept

```
          ┌──────── hopper (≥ 850 cm³ usable, ESD-safe PETG, bonded) ────────┐
          │  rotary shut-off valve + dump chute (quick-empty, DSP-08)          │
          └──┬───────────────┬───────────────┬────────────────────────────────┘
       ≥14 mm throat    ≥14 mm throat    ≥16 mm throat        (feed opening ≥ 6 × kernel length)
             │               │               │
   [fine  Ø7 bore]   [medium Ø10 bore]  [coarse Ø16 bore]     swappable printed tubes, internal helix,
     60T pulley        60T pulley         40T pulley          2 ball bearings each, adjustable tilt ±5°
        │ GT2 6 mm        │                  │
     20T NEMA 17       20T NEMA 17        20T NEMA 17         motors on grommets, rear of frame
             └──── outlets converge within a Ø30 mm circle over the pan centre ────┘
                    (trickler frame on its own feet; no contact with the scale or cell base)
```

- **Three rotating tubes**, one hopper, belt-driven steppers, one motor at a time (DSP-01, DSP-08, D-041).
- **The tubes are quick-change printed parts in sets** (ball / short-cut / long-kernel / flake). The powder profile stores which set is fitted. Why: the bore and helix that meter a 0.2 mg ball kernel well are wrong for a 2.5 mg, 2.2 mm-long stick. OpenTrickler does the same with low, mid and high-flow small tubes.
- **Why three stages, not OpenTrickler's two:** coarse-to-fine turndown is about **180:1**:
  - Coarse: 146 gr in 12 s = 12 gr/s.
  - Fine: 0.4 gr in 6 s = 0.07 gr/s.

  That is about 14:1 between neighbouring stages. A single tube can't meter both 12 gr/s and single long kernels.

## 2. Key numbers (worst case 150 gr; ASSUMED kernel data)

| Item | Coarse | Medium | Fine | Note |
|---|---|---|---|---|
| Hands over at | target − 4 gr | target − 0.4 gr | stable reading ≥ target | Coarse 3–5 gr per the OpenTrickler tuning guide |
| Mass flow needed | 12.2 gr/s | 0.60 gr/s | 0.07 gr/s, then ≤ 1 kernel/s pulse-and-wait | Final approach: flow × latency < ½ kernel |
| Bore × helix pitch (start point) | 16 × 10 mm | 10 × 6 mm | 7 × 5 mm | Pitch ≥ 2 × kernel length (4.4 mm) so long kernels lie along the helix |
| Tube speed (η·φ = 0.05–0.20) | 20–136 rpm | 4–29 rpm | 1–8 rpm | Screw-conveyor model (outside method); **measure on the rig** |
| Belt | 20T:40T GT2 6 mm | 20T:60T | 20T:60T | Tube pulley pitch diameter ≥ tube OD |
| STEP rate at 1/16 µstep | ≤ 91 kHz | ≤ 29 kHz | ≤ 8 kHz | Hardware timers (CLAUDE.md). Coarse can run 1/8 |
| Torque at motor | ≈ 1.3 mN·m | less | less | ~160× margin on a small NEMA 17. Size the motor for smoothness, run it at low current |
| Fine resolution | — | — | 1/16 rev ≈ 0.5–2.2 mg | Comparable to one long kernel |

Cycle-time budget: coarse 12 s + medium 6 s + fine 6 s + 3 motors-off settles × 1 s = **27 s**, against 30 s (DSP-06). The 3 s margin is thin. Config E with a 1.5 s settle gives 28.5 s.

## 3. Contradiction found: long-kernel quantization vs DSP-04 / DSP-05

A long extruded kernel is ASSUMED to measure 0.76–0.97 mm × 2.1–2.2 mm, giving a mass of **1.6–2.5 mg (0.025–0.038 gr)**. That is **larger than the whole +0.02 gr (1.30 mg) acceptance window**.

The model: kernels arrive singly, the controller stops at the first reading ≥ target, and the overshoot is uniform. Then P(landing in the window) = tol / m_k:

- **52–81 %** for long-kernel powder, against the DSP-05 goal of ≥ 90 % for extruded.
- **100 %** for ball, short-cut and flake.

So **DSP-04 (default tol +0.02 gr) and DSP-05 (≤ 10 % overthrow for extruded) can't both hold for long-kernel powders.** No mechanism fixes this, because kernels are discrete. The options, for Gate B:

1. **(Recommended)** Tolerance is per powder profile. The firmware warns when tol < 1.1 × the measured mean kernel mass, and the DSP-05 goal applies only when tol ≥ 1.1 × m_k. For a large long kernel that means +0.04 gr. Why: it is honest about physics and keeps +0.02 gr where it is achievable.
2. Keep +0.02 gr and accept 20–50 % overthrow (dump and re-throw) for long-kernel powders. That costs cycle time.
3. Reconsider "never under" (D-031) for long kernels, e.g. ±½ kernel. This changes a CONFIRMED decision, so it is listed only for completeness.

## 4. Design features and why

| Feature | Why |
|---|---|
| Feed throat ≥ 14 mm on every tube, from a common hopper manifold | No-arching rule of thumb: opening ≥ 6 × particle size, using kernel length (2.2 mm → 13.2 mm) |
| Internal helix, pitch ≥ 2 × kernel length; smooth bore option for ball powder | Patent US 10982939 describes spiral threads for metering extruded kernels 1–2 at a time. Too short a pitch makes long kernels bridge between ribs |
| Tilt adjustable ±5° per tube; outlet height adjustable | Feed rate is sensitive to inclination. Absorbs printing tolerance (design-in adjustability) |
| Thin-wall outlet lip with a drip edge | Reduces kernels clinging and dropping late (in-flight error) |
| ESD-safe PETG for hopper, tubes and funnel; tubes bonded through metal bearings to the PE-bonded frame | DSP-10. Static also causes force errors on the pan |
| One NEMA 17 per tube, belt drive, rubber grommets; trickler frame on its own feet | Belts and grommets reduce vibration into the frame. The separate frame decouples it from the cell (DSP-07) |
| Motors and drivers at the rear, outside the draft shield | Heat and switching nodes kept out of the powder zone (SAF-06). Steppers are brushless |
| Hopper: rotary shut-off valve plus a dump chute to a cup, lift-off body | Quick-empty (DSP-08). Minimizes resident powder (SAF-06) |
| No servo shutter gate on Rev A (OpenTrickler has one) | Tricklers stop dead when the stepper stops. One more actuator and a sliding interface in the powder path don't pay for themselves. Revisit if in-flight drip is measured |

## 5. Prior art credit

- **OpenTrickler** (github.com/eamars/OpenTrickler, GPL-3.0, read 2026-11-21): two rotating printed tubes, NEMA 17 + 40T GT2 belts, low/mid/high-flow tube variants by kernel size, hopper valve, coarse stop 3–5 gr, tuning goals coarse < 10 s and total < 15 s. Ideas only, no files or code (FW-02).
- **What changed and why:**
  - Three stages, for the 150 gr / 30 s turndown.
  - Hardware one-motor interlock.
  - An isolated frame, not the scale lid, carries the tricklers.
  - Bonded ESD-safe parts.
  - A ≥ 14 mm feed throat on every tube, for long kernels.
- US 10982939 (dual-barrel dispenser patent): internal spiral threads; extruded kernels dispensed 1–2 at a time. Concept reference only; no claim analysis done.

## 6. Actions before Gate C (Phase 3 trickler rig)

1. **Measure each powder you'll use (user):**
   - 20 kernels with calipers (diameter, length).
   - The mass of 100 kernels on a 1 mg scale.
   - Bulk density: mass of a level 10 cm³ scoop.

   This replaces every ASSUMED row in the sizing script.
2. Print the coarse, medium and fine tubes, then measure mg/rev against rpm and tilt for ball, short-cut and long kernels. This fits η·φ.
3. Measure the A&D step latency (driver spec §7). It sets the fine-stage pulse-and-wait period.
4. Log 50 throws per powder form against DSP-05 using the chosen tolerance policy.
