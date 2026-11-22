# 00 — State (read this first every session)

**Updated:** 2026-11-21 · **Phase:** 2 (architecture). Config S kickoff done; EMFC studies next · **Branch:** `phase2-architecture` (from `v2-restructure`, D-038; local commits, not pushed).

## Done
- Phase 0: Altium audit, used-scale search, scale protocols, BOM audit (`docs/phase0/`, `docs/SCALE_SUPPORT_research.md`). All 15 answers in (D-010..D-024).
- Phase 1: requirements A1 approved (D-030..D-034). Display = user's MCUFRIEND RM68140 parallel shield. User owns a bench supply.
- Repo: GraviMetra is home (D-036), PlatformIO kept (D-037). Restructure on `v2-restructure`, pushed to origin.
- **Phase 2 Config S kickoff (all PROPOSED for Gate B):**
  1. Controller I/O + pin budget → `docs/phase2/CONTROLLER_IO.md`, `analysis/pin_budget.py`. LQFP64 is −11 pins for S+E → LQFP100 G491VET6 (D-040). Decoder-gated stepper EN (D-041).
  2. System architecture → `docs/phase2/SYSTEM_ARCHITECTURE.md`: block diagram, 4 boards (D-042), safety latch with ARM (D-043), USB-only logic power (D-044), single-point SELV–PE bond (D-045).
  3. Scale driver spec → `docs/firmware/SCALE_DRIVER_SPEC.md`, `docs/SCALE_SUPPORT.md` (D-046). A&D facts verified from the manual.
  4. Trickler concept → `docs/phase2/TRICKLER_CONCEPT.md`, `analysis/trickler_sizing.py` (D-047). **Found contradiction OI-22** (long kernel > +0.02 gr window).

## In progress
- Nothing half-done. Waiting on the user for:
  - `/effort max` (next study)
  - OI-19: merge the `v2-restructure` PR (`gh` CLI not installed here)
  - OI-21: shield chip markings
  - OI-22: tolerance policy
  - OI-23: powder measurements
  - whether to push `phase2-architecture`

## Next 3 actions
1. **EMFC mechanics feasibility study (needs `/effort max`):** lever ratio and force chain, flat-part flexures (kerf taper, edge quality, flatness, bolted-joint hysteresis), counterweight sizing (OI-15), check mass (OI-07) → `docs/phase2/EMFC_MECHANICS.md` + `analysis/emfc_*.py`.
2. Servo architecture + measurand study (analog / digital / hybrid; shunt+ADS1262 vs PWM/HRTIM, OI-03/OI-04). This fixes the EMFC control lines in `CONTROLLER_IO.md` (also `/effort max`).
3. Error/noise budget incl. SmCo span tempco (OI-02) (also `/effort max`). Then the power budget (OI-09) and the Gate B package.

## Effort needed next: **max** (EMFC mechanics, servo, error budget). Switch back to xhigh afterward.

## Where things live
- Home repo: `github.com/ahallsten/GraviMetra`. Keep the working copy outside OneDrive (OneDrive corrupted the old copy's git metadata).
- Analysis: `.venv/Scripts/python analysis/<script>.py` (D-039; `pip install -r analysis/requirements.txt`). Pin data: `analysis/data/stm32g491_pins.json`.
- Mirror of key docs: claude.ai Project "Mechanical Engineering Design" (`claude/EMFC_*`).
