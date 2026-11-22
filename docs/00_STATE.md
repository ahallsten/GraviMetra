# 00 — State (read this first every session)

**Updated:** 2026-11-22 · **Phase:** 2 (architecture). Config S kickoff, EMFC mechanics, magnet/coil and servo/measurand done; error budget next · **Branch:** `phase2-architecture` (pushed; from `v2-restructure`).

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

## In progress
- Nothing half-done. Waiting on the user for:
  - **Powder measurements (OI-23). The user asked to be reminded of this at the end of every session.**
  - Buy 1095 shim stock (D-054, OI-26).
  - Optional: TC SmCo quote (OI-27, now a fallback).
  - Confirm the coil soft-off on SAFE + elastomer stops (OI-33) and hand-winding the 4-wire coil (OI-35).
  - Later, before B3 layout: DMM check of the shield's 74HC245 pins (OI-21, deferred by D-056).

## Next 3 actions
1. **Error/noise budget (effort max):** combine the mechanics (zero drift, corner load, hinge θ0), magnet tempco (MAGNET_COIL §7, in-situ α), electronics (SERVO_MEASURAND §6–7: noise, INL, span tempco, PCR) and environment (floor, drafts) into MET-03/04/06/08 budgets. Settle OI-02 and the r = 15 vs 20 question.
2. Power budget (OI-09) and the **Gate B package** (one-screen summary, ≤ 5 decisions).
3. Then tell the user `/effort xhigh`.

## Effort needed next: **max** (error budget). Back to xhigh after it.

## Where things live
- Home repo: `github.com/ahallsten/GraviMetra`. Keep the working copy outside OneDrive (OneDrive corrupted the old copy's git metadata). `gh` is at `C:\Program Files\GitHub CLI\gh.exe` (not on the bash PATH).
- Analysis: `.venv/Scripts/python analysis/<script>.py` (D-039; `pip install -r analysis/requirements.txt`). Pin data: `analysis/data/stm32g491_pins.json`.
- Mirror of key docs: claude.ai Project "Mechanical Engineering Design" (`claude/EMFC_*`).
