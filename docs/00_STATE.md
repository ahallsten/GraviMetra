# 00 — State (read this first every session)

**Updated:** 2026-11-21 · **Phase:** 2 (architecture), Config S path first · **Branch:** `phase2-architecture` (from `v2-restructure`, D-038).

## Done
- Phase 0: Altium audit, used-scale search, scale protocols, BOM audit (`docs/phase0/`, `docs/SCALE_SUPPORT_research.md`). All 15 answers in (D-010..D-024).
- Phase 1: requirements A1 approved (D-030..D-034). Display = user's MCUFRIEND RM68140 parallel shield. User owns a bench supply.
- Repo: GraviMetra is home (D-036), PlatformIO kept (D-037). Restructure on `v2-restructure`, pushed to origin.
- Phase 2 / 3 of 4: **scale driver spec + support matrix** → `docs/firmware/SCALE_DRIVER_SPEC.md`, `docs/SCALE_SUPPORT.md` (D-046).
- Phase 2 / 2 of 4: **system architecture** → `docs/phase2/SYSTEM_ARCHITECTURE.md` (block diagram, 4-board partition D-042, power tree, grounding, safety chain D-043..D-045).
- Phase 2 / 1 of 4: **controller I/O + pin budget** → `docs/phase2/CONTROLLER_IO.md`, `analysis/pin_budget.py`. LQFP64 −11 for S+E → LQFP100 PROPOSED (D-040). Hardware one-motor EN decoder PROPOSED (D-041).

## In progress
- Phase 2 kickoff, Config S path. Remaining sub-deliverables, in order:
  2. ~~System block diagram~~ done
  3. ~~RS-232 + A&D driver spec~~ done
  4. Trickler mechanics concept (OI-14) → `docs/phase2/TRICKLER_CONCEPT.md` + `analysis/trickler_sizing.py`
- OI-19: `v2-restructure` PR still needs to be opened/merged by the user (`gh` CLI not installed here).

## Next 3 actions
1. Finish sub-deliverables 2–4 above, checkpointing after each.
2. **EMFC mechanics feasibility study → tell the user to type `/effort max`.**
3. Servo architecture study, then the error/noise budget (also `/effort max`).

## Effort needed next: xhigh (max for the Phase 2 hard studies)

## Where things live
- Home repo: `github.com/ahallsten/GraviMetra`. Keep the working copy outside OneDrive (OneDrive corrupted the old copy's git metadata).
- Analysis: `.venv/Scripts/python analysis/<script>.py` (D-039; `pip install -r analysis/requirements.txt`).
- Mirror of key docs: claude.ai Project "Mechanical Engineering Design" (`claude/EMFC_*`).
