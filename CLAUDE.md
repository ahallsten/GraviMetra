# CLAUDE.md — GraviMetra (EMFC scale + three-stage powder dispenser)

Start every session by reading this file and `docs/00_STATE.md`, then only what the next action needs.

## Project
Three-stage (coarse/medium/fine) smokeless-powder dispenser.
- **Config S** = a bought balance read over RS-232 (A&D first, then Mettler MT-SICS, then Shimadzu).
- **Config E** = a DIY EMFC balance, Torbal AD520 class (520 g, 1 mg, SD 1 mg, ±2 mg).

The user is a mechanical engineer: be technical and concise. The master prompt is `docs/MASTER_PROMPT.md`; requirements are in `docs/01_REQUIREMENTS.md`.

## Confirmed decisions (details in docs/02_DECISIONS.md)
- SmCo magnets plus magnet, coil and ambient temperature sensing. A counterweight carries the pan/tray.
- No printed parts in the cell or anything supporting it: flat SendCutSend metal, alignment by cut dowel holes, **no CNC**.
- Everything else printed on a Bambu X2D (256 × 256 × 260 mm), PETG/ASA, ESD-safe where it touches powder.
- Internal mains: IEC C14 fused/switched inlet → Mean Well IRM-30-12 + IRM-20-5 on a mains board in a grounded metal compartment.
- No battery on Rev A (connector kept).
- HMI = the user's 3.5" MCUFRIEND RM68140 8-bit parallel TFT (ID 0x6814) with resistive touch, driven by LVGL.
- JLC/PCBWay boards; JLC places all SMT. Tools: KiCad 10, build123d, ngspice.
- Firmware: **PlatformIO** (`native` host tests + `gravimetra_g491` STM32Cube target + GitHub Actions CI), HAL/LL, ST-Link V3, original non-GPL code.
- 3 trickler channels, one motor at a time, belt-driven steppers. One hopper ≤ 1 lb. Conductive removable pan with auto-tare.
- Garage 15–30 °C, footprint ≤ 450 × 350 mm. Build 1–2 units.

## Rules
- Status labels: CONFIRMED / CANDIDATE / PROPOSED / UNRESOLVED / SUPERSEDED / RELEASE BLOCKER / ASSUMED.
- Show contradictions; never silently pick one. Explain *why* for each decision in 1–2 sentences.
- Vendor specs come from current datasheets; prices and stock from distributors. Record sources in `docs/datasheets.md`. Don't commit new PDFs; link them.
- ERC/DRC pass ≠ correct. Prefer manufacturer reference designs.
- Do math in Python with pint (`analysis/`); state assumptions, design factors and a sanity check. Mechanical methods: use the user's textbook index if available, cite [BOOK p.###], and label outside methods.
- Readability ≠ accuracy. An internal check mass is not traceable calibration. No hazardous-location claims.
- Never read whole .kicad_pcb, .kicad_sch or STEP files into context; query them with scripts.
- **Checkpoint after every sub-deliverable:** update `docs/00_STATE.md`, append to `docs/02_DECISIONS.md`, git commit. Never leave more than ~20 min of work uncommitted. On "checkpoint": save now and confirm in one line.
- You can't change your own effort: before the EMFC mechanics, servo and error-budget studies, tell the user to type `/effort max`, and to switch back to `/effort xhigh` afterward.
- Stop at each gate with a one-screen summary and ≤ 5 decisions.

## Firmware invariants (carried over from the Aug-2026 AGENTS.md, updated)
- MCU candidate STM32G491RET6 (LQFP64). Stock and pin budget are under review (OI-03, OI-18).
- The fast EMFC position loop architecture (analog / digital / hybrid) is a Phase 2 study. The Aug-2026 "analog loop, firmware supervises" is a CANDIDATE only.
- Config E measurand candidate: ADS1262 across a 4-terminal shunt. Shunt value, reference voltage, coil constant, lever ratio and all coefficients are calibration data, never constants.
- Exact conversion: 1 grain = 64.79891 mg. Display in 0.02 gr steps at 1 mg readability.
- Trickler stages are configuration (3 enabled). At most one motor is enabled or pulsed at any time. STEP pulses come from hardware timers.
- The normally closed hardware E-stop removes motion independently. Firmware monitoring is supplementary and must disable all motors and the check-mass actuator.
- The HMI sends requests only; application state and safety authority stay in firmware.
- Separate measurement profiles for active dispensing and precision settling; motors are off during final settle.
- No dynamic allocation after init in real-time/control paths. Drivers are independent of policy; messages are independent of transport.
- Persistence: versioned, CRC-protected, atomic/dual-copy. Watchdog refresh only after the health supervisor confirms progress.
- Hardware-dependent values stay as invalid/unconfigured sentinels until measured. Fail safe, never invent defaults.
- Don't invent the CAN/RS-485 protocol (not populated on Rev A).
- Build/test: `pio test -e native` and `pio run -e gravimetra_g491 -j 1`. Run the narrowest tests while iterating, then the full suite and target build before declaring a milestone done.

## Repo map
- `docs/`: state, requirements, decisions, open items, master prompt, research (`phase0/`, `SCALE_SUPPORT_research.md`)
- `docs/firmware/`: firmware architecture, calibration and bring-up
- `docs/reference/`: CubeMX setup guide
- `docs/archive/`: superseded Aug-2026 design pack and Nov-2026 handoff (read-only; mine, don't follow)
- `analysis/`: Python/pint calcs
- `mech/`: build123d CAD
- `hw/`: KiCad projects + shared lib
- `firmware/`: PlatformIO C++17
- `reference/datasheets/`, `reference/altium_legacy/`: read-only inputs

## Phase map
0 audit ✔ · 1 requirements ✔ (Gate A passed) · 2 architecture · 3 de-risk (Config S dispenser first, EMFC rig) · 4 detailed design · 5 layout · 6 release · 7 characterize.
