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
- **D-040** 2026-11-21 · MCU package → **LQFP100 (STM32G491VET6)** · **CONFIRMED by user 2026-11-21**, replacing RET6 (LQFP64). LQFP64 is −11 pins short for Config S+E; LQFP100 leaves +16 after every Rev-B net. Same die and firmware. Alternatives considered: LQFP80 (+3, no room for Rev-B nets), LQFP64 + I²C expander (puts safety lines behind a bus; conflicts with SAF-03). G474VET6 stays open for OI-03 · **PROPOSED (Gate B)** · evidence: `analysis/pin_budget.out.txt`, `docs/phase2/CONTROLLER_IO.md`. Stock UNVERIFIED.
- **D-041** 2026-11-21 · Stepper EN through a 2-to-4 decoder gated by the hardware safety chain gives hardware one-motor-at-a-time (SAF-04) at the same pin cost as three EN lines. Shared DIR, per-channel STEP timers, one TMC2209 UART bus addressed by MS1/MS2, DIAGs OR-ed · **PROPOSED (Gate B)** · evidence: TMC2209 DS Rev 1.08 pin table and §4.
- **D-042** 2026-11-21 · Board partition: B1 mains (2-layer, in the metal compartment), B2 controller (4-layer), B3 display carrier (2-layer, passive plus an optional level translator), B4 EMFC precision module (4-layer, Config E only). Alternative: merge B3 into B2 behind the display (decide after the enclosure sketch) · **PROPOSED (Gate B)** · evidence: `docs/phase2/SYSTEM_ARCHITECTURE.md` §2; IRM DS (Class B without extra parts; 20/25 A inrush → T fuses).
- **D-043** 2026-11-21 · Safety chain: E-stop NC + Hall lid sensor + external windowed watchdog → SR latch set only by the ARM button. The latch output enables the 12 V motor eFuse, the stepper decoder, the coil driver and the check-mass actuator. The MCU only monitors · **PROPOSED (Gate B)** · evidence: SAF-01..04, §5.
- **D-044** 2026-11-21 · USB VBUS can power logic alone through a 5 V power mux; motors and coil need 12 V and SAFE, so service mode can't move anything · **PROPOSED (Gate B)**.
- **D-045** 2026-11-21 · SELV 0 V bonded to PE at one point on B2 (0 Ω with an RC option, chosen at bring-up) · **PROPOSED (Gate B)** · why: the scale RS-232 SG and the PC USB ground make loops otherwise.
- **D-046** 2026-11-21 · Scale driver layer: `Uart` → protocol `ScaleDriver` → common `ScaleSource` (integer µg, stability unknown/stable/unstable, stale flag, tare confirmed by read-back). The A&D FX-i/FZ-i is the tier-1 driver at 19200 8N1, A&D standard format, `erCd 1`, 20/s. Grams only. RS-232 on B2 is DE-9 male, DTE, straight F-F cable, MAX3232E, 2↔3 / 7↔8 swap jumpers. US Solid DBS is not supported (≤ 2/s) · **PROPOSED (Gate B)** · evidence: A&D manual §10.6–10.7, §14, §16, function table; `docs/firmware/SCALE_DRIVER_SPEC.md`.
- **D-047** 2026-11-21 · Trickler concept: three swappable printed rotating tubes with internal helix (start points: coarse Ø16/p10, medium Ø10/p6, fine Ø7/p5 mm). Each has a ≥ 14 mm feed throat from one hopper (≥ 850 cm³), GT2 belts (2:1 coarse, 3:1 medium/fine), NEMA 17s on grommets, an isolated trickler frame, ESD-safe parts bonded through metal bearings, and no shutter gate on Rev A. Three stages, not OpenTrickler's two, because the turndown is ~180:1 · **PROPOSED (Gate B)** · evidence: `analysis/trickler_sizing.out.txt`, `docs/phase2/TRICKLER_CONCEPT.md`; OpenTrickler (GPL-3.0, ideas only); US 10982939.
- **D-048** 2026-11-21 · Resolves OI-22. Charge tolerance is per powder profile; the HMI warns when tol < 1.1 × measured mean kernel mass; the DSP-05 overthrow goal applies only when tol ≥ 1.1 × kernel mass. Alternatives: keep +0.02 gr and accept 20–50 % dumps for long kernels; relax "never under" (D-031) · **CONFIRMED** · user 2026-11-21 · evidence: `analysis/trickler_sizing.out.txt` §1.
- **D-049** 2026-11-21 · Git: the user installed `gh` and approved pushing `phase2-architecture`. Claude opens the `v2-restructure` → `main` PR per D-036; the merge stays with the user · **CONFIRMED** · user.
- **D-050** 2026-11-21 · EMFC cell = single-lever Roberval built up from flat SendCutSend Al 6061 plates (MIC-6 base) and **purchased thin shim blades** (1095 or 301 full-hard; pivots 0.04–0.05 mm, guide 0.075–0.10 mm), bolted + bonded clamps. Load arm 10 mm, **r = 15**, conditional on Km ≥ 1.6 N/√W (r = 20 if Km < 1.2). Rejected: SendCutSend 1075 0.381 mm as hinges (~350× too stiff, 107 mg per 100 nm), knife edges, printed. Purchased monoblock = fallback/benchmark; machined monoblock = OPTIONAL (CNC) · **PROPOSED (Gate B)** · evidence: `analysis/emfc_mechanics.out.txt` §1–2, `docs/phase2/EMFC_MECHANICS.md`.
- **D-051** 2026-11-21 · Counterweight option U: trims the tare to +2 % FS coil force at an empty pan; no W/2 preload. A tared reading is W·(1−αΔT) for any preload, and preload adds W_pre·α·ΔT of zero drift (~104 mg/K). Resolves OI-15 · **PROPOSED (Gate B)** · evidence: §7 of the study. Supersedes the preload bullet in the requirements' top-risk note.
- **D-052** 2026-11-21 · Internal check mass 200 g, non-magnetic, on a 3-ball kinematic seat on the load receptor; geared-stepper fork lift (TMC2209 #4, CM_STEP/DIR/HOME), de-energized while weighing, SAFE-interlocked. Span u ≈ 1.7 ppm. Resolves OI-07 · **PROPOSED (Gate B)**.
- **D-053** 2026-11-21 · Designed-in adjusters: x and y corner load (50:1 flexure reducers, ~0.1 µm resolution), optical-null height, counterweight mass and height (sets K_g), coil centring, stops, 3-point level. Roll-released coupling (two blades at 90°). Constant-power coil drive recommended to the servo/measurand study · **PROPOSED (Gate B)**.

## Phase 2, 2026-11-22 (user answers + magnet/coil study)

- **D-054** 2026-11-22 · Hinge blades from purchased 1095 blue-tempered shim stock; the user buys it. Confirms the shim-blade part of D-050. Suggested buy: Precision Brand 3 in × 50 in rolls in 0.0015, 0.002, 0.003 and 0.004 in (pivots 0.04–0.05 mm, guide 0.075–0.10 mm), about $26–35 per roll at Zoro (one seller lists $77). Each roll makes hundreds of ~10 × 20 mm blades. Cut with a sharp shear, not snips · **CONFIRMED** · user · evidence: distributor listings in `docs/datasheets.md`.
- **D-055** 2026-11-22 · Resolves OI-16. MET-09 (±2 mg at 1/3 Max, e = 25 mm) stays the full-range goal, verified in Phase 7. The Gate C rig exit uses the dosing case (e ≤ 5 mm, ≤ 25 µm adjust) plus measured adjuster stability (OI-25) · **CONFIRMED** · user.
- **D-056** 2026-11-22 · LCD D0–D7 move to **PE8–PE15**, all 5 V-tolerant (FT/FT_a). New solver rule A11. Why: if the shield's 74HC245s run at 5 V, a bus read can't damage the MCU, so the OI-21 DMM check no longer blocks Gate B; it moves to before B3 layout (Phase 4). Rev A still has 16 spare pins · **PROPOSED (Gate B)** · evidence: DS13122 Table 12 I/O structure column, `analysis/pin_budget.out.txt`.
- **D-057** 2026-11-22 · Resolves OI-24. EMFC motor = fixed SmCo pot with a moving coil.
  - Magnet Ø31.75 × 12.7 mm Sm2Co17, axially magnetised (fallback Ø25.4 × 12.7).
  - Laser-cut low-carbon steel: 4.76 mm pole piece and top plate, two 6.35 mm wall rings, 6.35 mm base. Gap 4.0 mm radial.
  - Coil 3.0 × 7.14 mm on a 0.3 mm Al bobbin, AWG 34 (~585 turns, 57 Ω), centred at the Kf maximum.
  - Km 4.4 N/√W (Recoma 28) / 3.2 (TC16 at min Br) vs 1.6 needed → 6 mW at full scale. **r = 15 kept.**
  - Non-magnetic fasteners within ~100 mm of the pot.

  Why: the pot is fixed, so its size is free; Km sets every thermal error · **PROPOSED (Gate B)** · evidence: `analysis/magnet_coil.out.txt`, `docs/phase2/MAGNET_COIL.md`.
- **D-058** 2026-11-22 · Constant coil power is required for full-range weighing. With linear drive, the magnet temperature follows load: 6.7 mg at 520 g with Recoma 28. The implementation goes to the servo/measurand study: ±I0 bipolar PWM (Km unchanged, reluctance term constant, measurand = duty) or an 80/20 split winding with a bifilar heater (Km −11 %, linear measurand kept). Dosing loads don't need it · **PROPOSED (Gate B)**.
- **D-059** 2026-11-22 · Magnet grade path.
  - The Rev A rig uses an off-the-shelf 28–30 MGOe Sm2Co17 disc with magnet-temperature compensation.
  - The pot is sized so a temperature-compensated Sm2Co17 (Arnold Recoma 2:17 Stab "0" or EEC 2:17-TC16) is a drop-in.
  - Quotes are optional (OI-27); decide after rig tempco data. Demoted to fallback by D-062.

  Why: standard Sm2Co17 needs α to 0.6 % and has ~3.5 ppm sensor lag at 2 K/h; TC needs ~50 % · **PROPOSED (Gate C)**.
- **D-060** 2026-11-22 · Coil temperature is measured through winding resistance (+3930 ppm/K) by the coil drive, not by a sensor on the moving coil. Magnet and ambient keep TMP117s. This changes the method of the CONFIRMED "coil temperature sensing" · **CONFIRMED** · user 2026-11-22.
- **D-061** 2026-11-22 · PRs merged into `main` with merge commits, so the branch histories stay intact. PR #1 (`v2-restructure`) was merged by Claude on the user's approval at 18:36:34 UTC. PR #2 (`phase2-architecture` up to 9bfb2cf) shows as merged from the user's GitHub account 14 s later; Claude did not issue that merge, and its title was corrected afterwards to describe the Phase 2 work. Later Phase 2 commits (from abb7f28) go in a new PR. Closes OI-19 · **CONFIRMED** · user.
- **D-062** 2026-11-22 · Magnet drift strategy, answering the user's questions.
  - Baseline: an off-the-shelf standard Sm2Co17 disc, with α of the finished cell **learned in situ** from check-mass adjustments at different temperatures (0.14 % of α at 5 K spacing). Plus lag-compensated magnet temperature, constant coil power (D-058) and auto-adjust.
  - CANDIDATE: a passive Carpenter Temperature Compensator 30 shunt, tested on the rig (OI-30).
  - Custom TC magnet (D-059) becomes the fallback.
  - REJECTED: electromagnet (22–180 W in the cell vs a 6 mW coil budget); magnet + trim winding (≤ 104 mW and still needs the temperature); a second coil to cancel drift (both coils share B).

  · **PROPOSED (Gate B)** · evidence: `analysis/magnet_coil.out.txt` §12, `docs/phase2/MAGNET_COIL.md` §13.

## Phase 2, 2026-11-22 (servo and measurand study, `docs/phase2/SERVO_MEASURAND.md`)

- **D-063** 2026-11-22 · EMFC position loop is **digital in the STM32G491**.
  - 10 kHz, TIM-triggered ADC1+ADC2 simultaneous on the optical sensor halves (ratiometric).
  - PID + lead + low-pass + notch slot. 25 Hz crossover, PM 46°, GM 15 dB, coil-bracket mode −44 dB.
  - Output: DAC1 with 2nd-order sigma-delta at 50 kHz by DMA, into the V-I driver.
  - Built analog, the same law gains only 1.3° of phase. Digital wins on retune and notch, acquisition, anti-windup and host testing. Matches the stated preference in the Nov-2026 handoff (§5).
  - The precision path stays on B4; the loop's position and command signals cross to B2 as buffered analog.
  - Rejected: analog PID with firmware supervision (the Aug-2026 candidate); hybrid analog PD + digital I.

  · **PROPOSED (Gate B)** · evidence: `analysis/servo_measurand.out.txt` §3, §9.
- **D-064** 2026-11-22 · Measurand = **ADS1263** ADC1 at 1200 SPS across a **~150 Ω foil shunt**, with an LTC6655B-2.5 reference. ADC2 measures the coil voltage in the background. The ADS1263 is pin-compatible with the ADS1262 [DS SBAS661C].
  - Noise 0.034 mg/√Hz, negligible.
  - INL 3 ppm FSR typ / 12 max = 9.6 / 38 ppm of span against MET-04's 3.85 ppm, so a ≥ 3-point linearity adjustment is required (OI-34).
  - Rejected: ±I0 PWM through the coil (±0.42 N square-wave force; audible; 27 mH reversal; a 20 kHz version needs a ~21-turn, 350 mA coil).
  - Rig fallback: precision PWM-DAC duty (inherently linear).
  - Supersedes the VCS1625ZP 1.5 Ω shunt candidate.

  · **PROPOSED (Gate B)** · evidence: §7.
- **D-065** 2026-11-22 · Coil wound **4 × AWG 37 in hand** (~292 turns each). A+B in series form the force winding: 114 Ω, Kf 33.3 N/A, Km 3.13. C+D form a non-inductive hairpin heater, driven bipolar at ≥ 25 kHz.
  - Constant total power P0 = 18 mW (Recoma 28) / 35 mW (TC16).
  - All four in series (Km 4.42) stays available for rig A/B tests.
  - Implements D-058 and supersedes D-057's single AWG 34 winding.

  · **PROPOSED (Gate B)** · evidence: §1.
- **D-066** 2026-11-22 · Coil driver = a zero-drift op-amp (OPA2388 class) driving the coil directly over −20 … +120 % FS (−2.1 … +12.5 mA), on a single 5 V analog rail with a ~1.0 V return node.
  - Pan removal keeps the null; a negative reading means pan absent.
  - Hardware current limit; COIL_EN gated by SAFE.
  - **On SAFE the command ramps to zero over ~0.5 s (RC soft-off) instead of an instant cut.** A hard cut at full load hits the stop at 3.9 mm/s (8.5 kg-equivalent on a stiff stop); soft-off gives 1.0 mm/s.
  - Closes OI-04: no OPA593.
  - The soft-off changes how SAF-02/03 "coil disabled" is met, so it needs the user's confirmation (OI-33).

  · **PROPOSED (Gate B)** · evidence: §4, §8.
- **D-067** 2026-11-22 · Interface requirements from the servo study.
  - Optical sensor: fine channel ≥ ±50 µm linear at ≤ 1 nm/√Hz; coarse monotonic channel over the full stop range (±0.6 mm at the vane); bandwidth ≥ 5 kHz. OPR2100 outer cells are the candidate for the coarse channel.
  - Mechanics: non-collocated modes (coil bracket, vane mount) ≥ 500 Hz with Q ≤ 50. Elastomer-faced lever stops, ≤ 33 kN/m at the coil radius with soft-off (≤ 2 kN/m without), so the peak stays ≤ 1 kg-equivalent (MET-11).

  · **PROPOSED (Gate B)** · evidence: §4.
- **D-068** 2026-11-22 · Readout and servo behaviour.
  - Dispensing profile: sinc² × 100 ms (0.1 s latency, SD 0.24 mg at 10 g, 20 Hz output).
  - Settling profile: sinc³ × 0.33 s (settles 1.14 s; SD 0.03 mg at 10 g, 0.17 mg at 500 g in the assumed environment).
  - Stable flag: |slope| < 0.5 d/s and 1 s SD < 0.5 d.
  - Servo modes: OFF / ACQUIRE (coarse channel, ramped setpoint; null in 0.25–0.61 s) / TRACK (coarse takeover beyond ±75 µm) / FAULT.
  - Controller parameters are computed from measured plant data. They stay invalid until measured, and the servo refuses to start without them.
  - Finding: a steady falling stream reads the mass already released (impact force = weight in flight), so dosing needs no in-flight correction.

  · **PROPOSED (Gate B)** · evidence: §4, §6.
- **D-069** 2026-11-22 · No HRTIM needed: the measurand is DC (D-064), and the PWM-DAC fallback reaches sub-ppm with sigma-delta on a standard 170 MHz timer. **STM32G491VET6 stays.** Closes the HRTIM part of OI-03; the stock check remains · **PROPOSED (Gate B)**.

