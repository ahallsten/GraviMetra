# GraviMetra + Kaboom-Konveyor review (2026-11-21)

## What's on disk

| Location | What it is | Health |
|---|---|---|
| `OneDrive\Documents\6. GitHub\GraviMetra\` (outer) | An old **copy** of the repo with a nested clone inside it | **Broken git:** `.git/packed-refs` unreadable ("Invalid argument"), typical of OneDrive Files-On-Demand corrupting git metadata. Otherwise identical to the nested clone except for the CubeMX guide |
| `…\GraviMetra\GraviMetra\` (nested) | The real clone of `github.com/ahallsten/GraviMetra` | Clean; `main` at `fbe4faa`, in sync with `origin/main`. 4 commits, 108 tracked files, ~16k lines |
| `PlatformIO\Projects\Kaboom-Konveyor\` | First attempt: Arduino Uno/Mega, HX711 load cell, AccelStepper, MCUFRIEND TFT | Separate repo; reference only |

## GraviMetra contents → verdict

| Item | What it is | Verdict | Why |
|---|---|---|---|
| `firmware/` (C++17, 26 modules, 67 Unity host tests) | Measurement pipeline, stability, tare, calibration, check-mass controller, temperature compensation, ADS1262/TMC2209/TMP117 drivers, auger manager, dispense planner, E-stop, fault manager, health supervisor, config store (CRC/dual-copy), telemetry, service framing | **KEEP, then MODIFY** | Good architecture: HAL isolated behind interfaces, fixed-capacity, fail-safe "unconfigured" sentinels, no fabricated hardware values. Every source file passes `g++ -std=c++17 -Wall -Wextra -fsyntax-only` (checked today) |
| `platformio.ini` + `.github/workflows/firmware.yml` | `native` host tests + `gravimetra_g491` target; CI on push | **KEEP** | Working CI, and you already use PlatformIO. See decision D-037 (supersedes FW-01's CMake default) |
| `docs/ARCHITECTURE.md`, `CALIBRATION.md`, `HARDWARE_BRINGUP.md` | Firmware-level docs | **KEEP, update** | Consistent with the new rules; update capacity, channel count and HMI |
| `AGENTS.md` | Codex handoff: invariants, unresolved hardware list, milestone log | **MERGE into CLAUDE.md**, then leave a 3-line pointer | The invariants are good. Its milestone log is stale: it lists modules as unchecked that now exist with tests |
| `README.md`, `LICENSE` (MIT) | — | KEEP (README rewritten) | |
| `EMFC_Scale_Design_Pack/01_Electrical_Design/*.md` (architecture review, connector/IO plan, power budget, assembly/reflow plan) | Aug-2026 electrical design notes | **ARCHIVE**, mine in Phase 2 | Based on superseded facts (250 g, 0.01 gr, 4 channels, BTT modules, Nextion), but the MCU/ADC/layout guidance is still useful input |
| `02_EMFC_Mechanics/250g_EMFC_Design_Guide.md` | 250 g cell design guide | **ARCHIVE**, mine in Phase 2 | Capacity and approach superseded |
| `03_Altium/…Step_by_Step_Guide.md` | Altium guide | **ARCHIVE** | Altium dropped (D-011) |
| `04_Firmware/Codex_Firmware_Implementation_Prompt.md` | Firmware contract for Codex | **ARCHIVE** | Superseded by the master prompt + CLAUDE.md; keep for history |
| `05_AI_Reference/*` | AI context and datasheet index | **ARCHIVE** | Superseded by the handoff + new docs |
| `06_STM32CubeIDE/…Setup_Guide_Expanded.md` | CubeMX setup guide | **KEEP** in `docs/reference/` | CubeMX is still used to validate the pin map |
| `Datasheets/` (29 PDFs, ~100 MB, in git) | Manufacturer PDFs | **Split**: still-relevant → `reference/datasheets/`; superseded (S3096-02, MAX77960B, TCAN1042H, THVD1450, Nextion, TPS2121) → archive. No new PDFs in git; use a URL + revision index (`docs/datasheets.md`) | Repo bloat; history isn't rewritten (non-destructive) |

### Firmware deltas needed (not done yet; queued)
1. Capacity 250 g → 520 g (`kFrozenLiveCapacityMg`, calibration points).
2. Stages 4 → 3 (`dispense_stage_4`, `kMaximumAugers = 4`). Keep it configurable; rename "auger" to "trickler".
3. HMI: replace the Nextion driver with a display driver for the RM68140 8-bit parallel TFT + LVGL. Keep `ui_model` (transport-independent).
4. Add the Config S scale-driver layer (A&D → MT-SICS → Shimadzu) behind the existing measurement interface, plus a scale simulator.
5. Temperature compensation: magnet + coil + ambient (SmCo), with auto internal adjust on ΔT (MET-07).
6. AGENTS.md "Rev A analog loop" is now a *candidate* only; the servo partition is a Phase 2 study.

## Kaboom-Konveyor → what to take
- **Display identified:** MCUFRIEND-style 3.5" Arduino-shield TFT, 320 × 480, `readID()` = **0x6814** (RM68140 controller per the MCUFRIEND_kbv ID table; verify against the panel). **8-bit parallel** interface, not SPI.
- **Touch:** 4-wire resistive on shared pins (XP=D7, XM=A1, YP=A2, YM=D6); calibration TS_LEFT 176, TS_RT 921, TS_TOP 177, TS_BOT 939 (portrait).
- **UI ideas:** numeric keypad, tare/calibrate/test-mode/home buttons, grains/grams toggle, EEPROM settings.
- The libraries (MCUFRIEND_kbv, Adafruit GFX, TouchScreen, AccelStepper, HX711) are Arduino/AVR and **won't be reused directly**. The STM32 firmware will use LVGL plus a small RM68140 parallel driver written from the controller datasheet.
- **MCU impact:** 8-bit parallel needs about 13 GPIO (8 data, ideally on one port for single BSRR writes, plus CS/RS/WR/RD/RST) and 2 ADC pins for touch. Bandwidth is better than SPI (a full 307 kB frame in tens of ms). The pin budget on the LQFP64 gets tight → Phase 2 pin plan (OI-18).
- Leave the Kaboom-Konveyor repo as-is (legacy).

## Recommendation: one home repo
Use **`ahallsten/GraviMetra`** on GitHub as the single source of truth for requirements, decisions, CAD, KiCad, firmware and analysis. **Don't keep the working copy inside OneDrive**: OneDrive is already corrupting git metadata here.

Proposed migration (branch `v2-restructure`, non-destructive):
1. Fresh clone from GitHub, outside OneDrive (e.g. `C:\Users\Aunders\dev\GraviMetra`), or let Claude push a branch from the cloud session.
2. Move `EMFC_Scale_Design_Pack/` → `docs/archive/2026-08_design_pack/` with a SUPERSEDED banner; relevant datasheets → `reference/datasheets/`.
3. Add the v2 structure: `CLAUDE.md`, `docs/00_STATE…03_OPEN_ITEMS`, `docs/phase0/`, `analysis/`, `mech/`, `hw/`, `reference/altium_legacy/`.
4. Merge the AGENTS.md invariants into CLAUDE.md; AGENTS.md becomes a pointer.
5. Rewrite the README; open a PR and merge.
6. Afterwards, delete the outer OneDrive copy (stale, broken git) once you confirm the GitHub copy is complete.
