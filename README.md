# GraviMetra

A hobby, garage-built **three-stage (coarse / medium / fine) smokeless-powder dispenser** that weighs with either:

- **Config S:** a bought precision balance read over RS-232 (A&D FX-i/FZ-i/GF first, then Mettler MT-SICS, then Shimadzu), or
- **Config E:** a DIY **electromagnetic-force-compensation (EMFC) balance** built to Torbal AD520-class performance: 520 g capacity, 1 mg readability, 1 mg repeatability, ±2 mg linearity.

Mechanics are 3D-printed (Bambu X2D) except the weighing cell, which is flat SendCutSend metal. Boards are KiCad 10, fabbed at JLCPCB/PCBWay. Firmware is C++17 on an STM32G4.

## Status
- **Phase 1 (requirements) complete; Gate A passed.** Next: Phase 2 architecture studies.
- Live state: [`docs/00_STATE.md`](docs/00_STATE.md) · requirements: [`docs/01_REQUIREMENTS.md`](docs/01_REQUIREMENTS.md) · decisions: [`docs/02_DECISIONS.md`](docs/02_DECISIONS.md) · open items: [`docs/03_OPEN_ITEMS.md`](docs/03_OPEN_ITEMS.md).

## Safety
The machine handles combustible/energetic powder. The firmware starts in an inert state and must not energize any motor or actuator until the pin map is validated in CubeMX and against a released schematic, the hardware E-stop is verified with motion power isolated, and limits have bench-measured values. Nothing here is hazardous-location, machinery-safety or metrology certification. A passing build or simulation doesn't establish safe powder handling or weighing accuracy.

## Firmware: build and test
PlatformIO environments (`platformio.ini`):

| Environment | Purpose |
|---|---|
| `native` | Host unit tests (Unity); needs a C++17 GCC/Clang on `PATH`. Runs in CI. |
| `gravimetra_g491` | STM32G491RET6 target (STM32Cube HAL/LL, ST-Link upload). Uses the `nucleo_g491re` manifest only for startup/linker files, not its pinout. |

```text
pio test -e native
pio run -e gravimetra_g491 -j 1
pio run -e gravimetra_g491 -j 1 -t upload
```

Firmware docs: [`docs/firmware/ARCHITECTURE.md`](docs/firmware/ARCHITECTURE.md), [`HARDWARE_BRINGUP.md`](docs/firmware/HARDWARE_BRINGUP.md), [`CALIBRATION.md`](docs/firmware/CALIBRATION.md).

## Repository map
```text
CLAUDE.md            Standing rules + confirmed decisions (read first)
docs/                State, requirements, decisions, open items, master prompt, research
docs/firmware/       Firmware architecture, bring-up, calibration
docs/archive/        Superseded Aug-2026 design pack and Nov-2026 handoff (history only)
analysis/            Engineering calcs (Python + pint)
mech/                Mechanical CAD (build123d) and exports
hw/                  KiCad 10 projects (one per board) + shared library
firmware/            PlatformIO C++17: include/, src/, test/
reference/           Read-only inputs: datasheets, legacy Altium exports
```

## License
MIT (see `LICENSE`).
