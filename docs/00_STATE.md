# 00 — State (read this first every session)

**Updated:** 2026-11-22 · **Phase:** 2 (architecture). Config S kickoff, EMFC mechanics, magnet/coil, servo/measurand and the error budget done; power budget and Gate B package next · **Branch:** `phase2-architecture` (pushed; from `v2-restructure`).

## Done
- Phase 0: Altium audit, used-scale search, scale protocols, BOM audit (`docs/phase0/`, `docs/SCALE_SUPPORT_research.md`). All 15 answers in (D-010..D-024).
- Phase 1: requirements approved; now Rev A2 (D-048 per-profile tolerance, D-040 LQFP100).
- Repo: GraviMetra is home (D-036), PlatformIO kept (D-037). PR #1 (v2 restructure) and PR #2 (Phase 2 so far) merged into `main` 2026-11-22 (D-061). Work continues on `phase2-architecture` (commits from abb7f28 are not in `main` yet); open a new PR for them at the next checkpoint the user wants merged.
- **Phase 2 Config S kickoff:**
  - Pin budget: `docs/phase2/CONTROLLER_IO.md`.
  - System architecture: `docs/phase2/SYSTEM_ARCHITECTURE.md`.
  - Scale driver: `docs/firmware/SCALE_DRIVER_SPEC.md`, `docs/SCALE_SUPPORT.md`.
  - Trickler: `docs/phase2/TRICKLER_CONCEPT.md`.
  - Decisions D-038..D-049.
- **Phase 2 EMFC mechanics study** (`docs/phase2/EMFC_MECHANICS.md`, `analysis/emfc_mechanics.py`; D-050..D-053, PROPOSED):
  - Single-lever Roberval, r = 15 if Km ≥ 1.6.
  - Purchased shim blades: SendCutSend 1075 is ~350× too stiff.
  - Sub-µm corner adjusters for MET-09.
  - Counterweight option U: preload doesn't help span.
  - 200 g check mass.
- **Phase 2 magnet/coil study** (`docs/phase2/MAGNET_COIL.md`, `analysis/magnet_coil.py`; D-057..D-060, PROPOSED):
  - Fixed SmCo pot (Ø31.75 × 12.7 Sm2Co17), moving coil. Km 4.4 (3.2 with a TC grade) vs 1.6 needed; 6 mW at FS; r = 15 kept. OI-24 closed.
  - Constant coil power required at full range (D-058). Standard Sm2Co17 makes OI-02 a 0.6 % compensation problem; TC grade is a drop-in (D-059).
- **Phase 2 servo + measurand study** (`docs/phase2/SERVO_MEASURAND.md`, `analysis/servo_measurand.py`; D-063..D-069, PROPOSED):
  - Digital loop in the G491 (10 kHz, 25 Hz crossover, PM 46°); ADS1263 + 150 Ω foil shunt measurand; 4-wire-in-hand coil with a hairpin heater for constant power; op-amp coil drive (OI-04 closed); no HRTIM (G491 stays).
  - Open risks: MET-04 needs a 3-point linearity adjustment (ADC INL); 500 g SD may be floor-limited; coil soft-off + elastomer stops need the user's OK.
- User answers 2026-11-22: shim stock CONFIRMED (D-054); MET-09 approach CONFIRMED (D-055); LCD bus moved to 5 V-tolerant pins so OI-21 no longer blocks (D-056); coil temperature by winding resistance CONFIRMED (D-060).
- Magnet drift alternatives (D-062): baseline = OTS Sm2Co17 + in-situ α learned with the check mass; Carpenter TC30 shunt = rig experiment (OI-30); electromagnet rejected (22–180 W).
- User answers 2026-11-22 (D-070): coil soft-off on SAFE + elastomer stops CONFIRMED; coil hand-wound 4 × AWG 37 on a printed winding jig CONFIRMED (jig design in `mech/coil_winder/`, in progress).
- **Phase 2 error/noise budget** (`docs/phase2/ERROR_BUDGET.md`, `analysis/error_budget.py`; D-071..D-074, PROPOSED):
  - Config E meets the AD520-class numbers on paper in the assumed environment. MET-03 0.31 mg at 500 g; MET-04 ≤ 0.87 mg with a 9-point adjust; span error at 520 g in use 0.93 mg (k = 2); MET-08 0.46 mg (light hinges).
  - **OI-02 closed by analysis for the operating spec** with standard Sm2Co17 + in-situ span model + auto-adjust at 0.5 K. MET-06 static only after ≥ 10 K of history (Gate B wording, OI-38).
  - r = 15 stays with the light hinge set.
  - New Gate B decisions: auto-adjust policy vs `CALIBRATION.md` (OI-37) and MET-06 wording (OI-38).

## In progress
- Background agents (2026-11-22, interrupted twice by the usage limit; partial files are uncommitted in the working tree):
  - Power budget OI-09: `analysis/power_budget.py`, `docs/phase2/POWER_BUDGET.md`.
  - Coil winding jig: `mech/coil_winder/`, `analysis/coil_winding.py`, `docs/phase2/COIL_WINDING.md`.
  - Independent review: `docs/phase2/research/PHASE2_REVIEW.md`.
  - Gate B sourcing (OI-03/28/29): `docs/phase2/research/GATE_B_SOURCING.md`.
  - Not yet resumed: packaging study (`mech/packaging/`, OI-08), display timing (`docs/phase2/DISPLAY_TIMING.md`, OI-12), environment inputs (`docs/phase2/research/ENV_INPUTS.md`).
- Waiting on the user for:
  - **Powder measurements (OI-23). The user asked to be reminded of this at the end of every session.**
  - Buy 1095 shim stock (D-054, OI-26).
  - Optional: TC SmCo quote (OI-27, now a fallback).
  - Later, before B3 layout: DMM check of the shield's 74HC245 pins (OI-21, deferred by D-056).

## Next 3 actions
1. Integrate the agents' results as they land (power budget → error budget §8 warm-up numbers; review findings → fix or log; sourcing → OI-03/28/29), then resume the packaging, display and environment agents.
2. **Gate B package:** one-screen summary, ≤ 5 decisions (auto-adjust policy OI-37, MET-06 wording OI-38, B2/B3 merge OI-08, plus the D-050..D-074 PROPOSED set), and the CLAUDE.md invariant update (ADS1263, digital loop).
3. Tell the user `/effort xhigh` after Gate B.

## Effort needed next: **xhigh** (the max-effort studies are done).

## Where things live
- Home repo: `github.com/ahallsten/GraviMetra`. Keep the working copy outside OneDrive (OneDrive corrupted the old copy's git metadata). `gh` is at `C:\Program Files\GitHub CLI\gh.exe` (not on the bash PATH).
- Analysis: `.venv/Scripts/python analysis/<script>.py` (D-039; `pip install -r analysis/requirements.txt`). Pin data: `analysis/data/stm32g491_pins.json`.
- Mirror of key docs: claude.ai Project "Mechanical Engineering Design" (`claude/EMFC_*`).
