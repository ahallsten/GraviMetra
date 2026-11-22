# 00 — State (read this first every session)

**Updated:** 2026-11-21 · **Phase:** 2 (architecture). Config S kickoff + EMFC mechanics done; servo/measurand next · **Branch:** `phase2-architecture` (pushed; from `v2-restructure`).

## Done
- Phase 0: Altium audit, used-scale search, scale protocols, BOM audit (`docs/phase0/`, `docs/SCALE_SUPPORT_research.md`). All 15 answers in (D-010..D-024).
- Phase 1: requirements approved; now Rev A2 (D-048 per-profile tolerance, D-040 LQFP100).
- Repo: GraviMetra is home (D-036), PlatformIO kept (D-037). **PR #1 `v2-restructure` → `main` open; the user merges** (OI-19).
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

## In progress
- Nothing half-done. Waiting on the user for:
  - Merge PR #1 (OI-19).
  - DMM check of the shield's 74HC245 VCC/DIR/OE pins (OI-21).
  - Powder measurements (OI-23).
  - Confirm purchased shim blades (D-050 / EMFC_MECHANICS §9.1).

## Next 3 actions
1. **Magnet/coil first-order study (OI-24):** SmCo grade (incl. temperature-compensated), pot geometry, Kf, R, Km, dKf/dx, moving coil vs moving magnet, constant-power options → `analysis/magnet_coil.py`. Effort max is OK.
2. **Servo architecture + measurand study (effort max):** analog / digital / hybrid; shunt + ADS1262 vs PWM duty (G474 HRTIM, OI-03); constant-power drive; coil driver (OI-04). Plant from `EMFC_MECHANICS.md` §10. Fixes the EMFC control lines in `CONTROLLER_IO.md`.
3. **Error/noise budget (effort max),** incl. OI-02 span tempco closure. Then the power budget (OI-09) and the Gate B package. Then tell the user `/effort xhigh`.

## Effort needed next: **max** (servo, error budget). Back to xhigh after those.

## Where things live
- Home repo: `github.com/ahallsten/GraviMetra`. Keep the working copy outside OneDrive (OneDrive corrupted the old copy's git metadata). `gh` is at `C:\Program Files\GitHub CLI\gh.exe` (not on the bash PATH).
- Analysis: `.venv/Scripts/python analysis/<script>.py` (D-039; `pip install -r analysis/requirements.txt`). Pin data: `analysis/data/stm32g491_pins.json`.
- Mirror of key docs: claude.ai Project "Mechanical Engineering Design" (`claude/EMFC_*`).
