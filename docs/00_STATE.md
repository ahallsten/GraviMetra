# 00 — State (read this first every session)

**Updated:** 2026-11-21 22:05 · **Phase:** Gate A PASSED → repo migration (branch `v2-restructure`) → Phase 2 next.

## Done
- Phase 0: Altium audit, used-scale search, scale protocols, BOM audit (`docs/phase0/`, `docs/SCALE_SUPPORT_research.md`). All 15 answers in (D-010..D-024).
- Phase 1: requirements A1 approved (D-030..D-034). Display = user's MCUFRIEND RM68140 parallel shield. User owns a bench supply.
- Repo: GraviMetra chosen as home (D-036), PlatformIO kept (D-037). Restructure done on `v2-restructure`: design pack archived, datasheets split, v2 docs added, AGENTS.md → CLAUDE.md. **Firmware code untouched.**

## In progress
- Push `v2-restructure` and merge the PR (OI-19).

## Next 3 actions
1. Phase 2 kickoff, Config S path first: system block diagram, controller I/O + pin budget (OI-18), RS-232 + A&D driver spec, trickler mechanics concept (OI-14).
2. **EMFC mechanics feasibility study → tell the user to type `/effort max`.**
3. Servo architecture study, then the error/noise budget (also `/effort max`).

## Effort needed next: xhigh (max for the Phase 2 hard studies)

## Where things live
- Home repo: `github.com/ahallsten/GraviMetra`. Keep the working copy outside OneDrive (OneDrive corrupted the old copy's git metadata).
- Mirror of key docs: claude.ai Project "Mechanical Engineering Design" (`claude/EMFC_*`).
