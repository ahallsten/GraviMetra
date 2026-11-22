# 02 — Decision log (ADR-lite)

Format: ID · date · decision · status · alternatives considered · evidence.

## User decisions before Phase 0 (from the conversation and master prompt v2)

- **D-001** 2026-11-21 · Metrology spec = Torbal AD520 class (520 g, 1 mg, SD 1 mg, ±2 mg). The 0.01 gr / ±0.5 mg target is SUPERSEDED · CONFIRMED · alt: 0.01 gr (≈4× harder linearity) · user.
- **D-002** 2026-11-21 · SmCo magnets plus magnet temperature measurement · CONFIRMED · alt: NdFeB (≈3× worse tempco) · user; Sm-Co Br tempco −0.03 to −0.05 %/K.
- **D-003** 2026-11-21 · Two configurations: **S** (bought scale over RS-232) and **E** (DIY EMFC), sharing the dispenser, controller, firmware and enclosure · CONFIRMED · user.
- **D-004** 2026-11-21 · No printed parts in the scale cell or its supports; flat SendCutSend metal; no CNC (CNC only as an OPTIONAL with a non-CNC alternative) · CONFIRMED · user.
- **D-005** 2026-11-21 · Internal mains: IEC C14 fused, switched inlet + PCB-mount Mean Well modules on a dedicated board and compartment. External brick rejected · CONFIRMED · user (brick clutters the bench).
- **D-006** 2026-11-21 · HMI chosen from the user's displays (Nextion no longer CONFIRMED) · CONFIRMED · user.
- **D-007** 2026-11-21 · Toolchain KiCad 10 + build123d + ngspice; Claude Code with Opus 5.5 xhigh, max for hard studies · CONFIRMED · user.
- **D-008** 2026-11-14 · Three trickler channels, one motor at a time, bare TMC2209 (candidate) · CONFIRMED · handoff §7.

## Phase 0 answers (2026-11-21, user answered all 15)

- **D-010** Q1 · "kind of wait" = counterweight for pan/tray · CONFIRMED.
- **D-011** Q2 · Rebuild in KiCad, using Altium only as a spec (no import) · CONFIRMED · evidence: `phase0/PHASE0_AUDIT.md` (almost nothing wired; duplicate net; mismatched names).
- **D-012** Q3 · A&D FX-i/FZ-i/GF first, then Mettler MT-SICS, then Shimadzu; build Config S dispenser first, EMFC rig in parallel · CONFIRMED · evidence: `SCALE_SUPPORT_research.md` (AD520 frame lacks stable flag).
- **D-013** Q4 · All powder forms, 5–150 gr, ≤ 30 s/charge · CONFIRMED.
- **D-014** Q5 · One hopper, ≤ 1 lb, quick-empty · CONFIRMED.
- **D-015** Q6 · Removable conductive pan, presence detect, auto-tare · CONFIRMED.
- **D-016** Q7 · User owns Mean Well IRM-30-12 + IRM-20-5; grounded metal mains compartment from SendCutSend · CONFIRMED.
- **D-017** Q8 · No battery on Rev A; connector/footprint kept for Rev B. Resolves the conflict with handoff §9 ("battery CONFIRMED desired") for Rev A · CONFIRMED.
- **D-018** Q9 · 1–2 units; report cost per configuration and per prototype round · CONFIRMED.
- **D-019** Q10 · PETG/ASA plus ESD-safe for powder contact; tools: drill press, taps, heat-set tool, calipers · CONFIRMED.
- **D-020** Q11 · JLC places all SMT; user hand-solders THT and connectors · CONFIRMED.
- **D-021** Q12 · Test gear: Siglent SDS1204X-E (+ AWG, 16-ch logic probe), Keysight 33210A, Fluke 287, soldering iron. No masses, no bench supply listed · CONFIRMED (inventory).
- **D-022** Q13 · Display = Adafruit 3.5" TFT (HX8357D family; exact variant TBD) · CONFIRMED (family) / variant UNRESOLVED.
- **D-023** Q14 · CMake + CubeMX HAL/LL, ST-Link V3, original non-GPL firmware, CAN/RS-485 not populated · CONFIRMED.
- **D-024** Q15 · Garage 15–30 °C, footprint ≤ 450 × 350 mm, deliver STEP + STL + PDF · CONFIRMED.

## Gate A (2026-11-21 21:45, user approved)

- **D-030** Masses: defer the E2 purchase. Buy F1 20/50/100 g for the EMFC rig; buy certified E2 200 g + 500 g before Config E full-range verification · **CONFIRMED** · evidence: OIML R111 (F1 200 g = ±1.0 mg, 500 g = ±2.5 mg; E2 500 g = ±0.75 mg).
- **D-031** Charge acceptance +0.02 gr / −0, user-settable; overthrow → dump · **CONFIRMED**.
- **D-032** Metrology additions beyond AD520: stabilization goal ≤ 1.5 s, span drift ≤ ±2 ppm/K, auto internal adjust at ΔT ≥ 0.5 K, eccentric ±2 mg at 1/3 Max, warm-up ≤ 60 min, overload ≥ 1 kg static, ≥ 10 Hz output · **CONFIRMED** (requirements A0 approved as written).
- **D-033** Current-limited bench supply: user already owns one · **CONFIRMED** (no purchase).
- **D-034** HMI display = the user's MCUFRIEND-style 3.5" Arduino-shield TFT (sold as "Microyum"), 320 × 480, `readID()` 0x6814 (RM68140 per MCUFRIEND_kbv; verify), 8-bit parallel, 4-wire resistive touch; used successfully in Kaboom-Konveyor. SUPERSEDES the Adafruit HX8357D candidate · **CONFIRMED** · evidence: Kaboom-Konveyor `src/main.cpp`.

## Repo (2026-11-21 22:00, user approved)

- **D-036** Home repo = `github.com/ahallsten/GraviMetra`; working copy outside OneDrive; restructure on branch `v2-restructure` per `GRAVIMETRA_REPO_REVIEW.md`; Claude prepares the branch in the cloud and opens the PR · **CONFIRMED**.
- **D-037** Firmware toolchain = keep GraviMetra's PlatformIO (`native` Unity tests + `gravimetra_g491` STM32Cube target + GitHub Actions CI). SUPERSEDES FW-01's "CMake" default (ST-Link V3 and HAL/LL unchanged) · **CONFIRMED** · evidence: existing CI and 67 tests; user already uses PlatformIO.

## Phase 2 (started 2026-11-21)

- **D-038** 2026-11-21 · Phase 2 work goes on branch `phase2-architecture`, cut from `v2-restructure` (that PR is still unmerged, OI-19), so the restructure PR stays reviewable on its own · CONFIRMED (process) · Claude.
- **D-039** 2026-11-21 · Analysis environment = repo-local `.venv` (gitignored) pinned by `analysis/requirements.txt`; run scripts with `.venv/Scripts/python` · CONFIRMED (process) · Claude.
- **D-040** 2026-11-21 · MCU package → **LQFP100 (STM32G491VET6)**, replacing RET6 (LQFP64). LQFP64 is −11 pins short for Config S+E; LQFP100 leaves +16 after every Rev-B net. Same die and firmware. Alternatives considered: LQFP80 (+3, no room for Rev-B nets), LQFP64 + I²C expander (puts safety lines behind a bus; conflicts with SAF-03). G474VET6 stays open for OI-03 · **PROPOSED (Gate B)** · evidence: `analysis/pin_budget.out.txt`, `docs/phase2/CONTROLLER_IO.md`. Stock UNVERIFIED.
- **D-041** 2026-11-21 · Stepper EN through a 2-to-4 decoder gated by the hardware safety chain gives hardware one-motor-at-a-time (SAF-04) at the same pin cost as three EN lines. Shared DIR, per-channel STEP timers, one TMC2209 UART bus addressed by MS1/MS2, DIAGs OR-ed · **PROPOSED (Gate B)** · evidence: TMC2209 DS Rev 1.08 pin table and §4.
- **D-042** 2026-11-21 · Board partition: B1 mains (2-layer, in the metal compartment), B2 controller (4-layer), B3 display carrier (2-layer, passive plus an optional level translator), B4 EMFC precision module (4-layer, Config E only). Alternative: merge B3 into B2 behind the display (decide after the enclosure sketch) · **PROPOSED (Gate B)** · evidence: `docs/phase2/SYSTEM_ARCHITECTURE.md` §2; IRM DS (Class B without extra parts; 20/25 A inrush → T fuses).
- **D-043** 2026-11-21 · Safety chain: E-stop NC + Hall lid sensor + external windowed watchdog → SR latch set only by the ARM button. The latch output enables the 12 V motor eFuse, the stepper decoder, the coil driver and the check-mass actuator. The MCU only monitors · **PROPOSED (Gate B)** · evidence: SAF-01..04, §5.
- **D-044** 2026-11-21 · USB VBUS can power logic alone through a 5 V power mux; motors and coil need 12 V and SAFE, so service mode can't move anything · **PROPOSED (Gate B)**.
- **D-045** 2026-11-21 · SELV 0 V bonded to PE at one point on B2 (0 Ω with an RC option, chosen at bring-up) · **PROPOSED (Gate B)** · why: the scale RS-232 SG and the PC USB ground make loops otherwise.
- **D-046** 2026-11-21 · Scale driver layer: `Uart` → protocol `ScaleDriver` → common `ScaleSource` (integer µg, stability unknown/stable/unstable, stale flag, tare confirmed by read-back). The A&D FX-i/FZ-i is the tier-1 driver at 19200 8N1, A&D standard format, `erCd 1`, 20/s. Grams only. RS-232 on B2 is DE-9 male, DTE, straight F-F cable, MAX3232E, 2↔3 / 7↔8 swap jumpers. US Solid DBS is not supported (≤ 2/s) · **PROPOSED (Gate B)** · evidence: A&D manual §10.6–10.7, §14, §16, function table; `docs/firmware/SCALE_DRIVER_SPEC.md`.
- **D-047** 2026-11-21 · Trickler concept: three swappable printed rotating tubes with internal helix (start points: coarse Ø16/p10, medium Ø10/p6, fine Ø7/p5 mm). Each has a ≥ 14 mm feed throat from one hopper (≥ 850 cm³), GT2 belts (2:1 coarse, 3:1 medium/fine), NEMA 17s on grommets, an isolated trickler frame, ESD-safe parts bonded through metal bearings, and no shutter gate on Rev A. Three stages, not OpenTrickler's two, because the turndown is ~180:1 · **PROPOSED (Gate B)** · evidence: `analysis/trickler_sizing.out.txt`, `docs/phase2/TRICKLER_CONCEPT.md`; OpenTrickler (GPL-3.0, ideas only); US 10982939.
