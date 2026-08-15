# GraviMetra

GraviMetra is deterministic firmware for a 250 g electromagnetic-force-compensation scale and configurable staged powder feeder. Revision A uses an analog fast position-restoration loop; firmware measures restoring current, supervises safety and stability, and controls up to four mutually exclusive auger stages.

## Safety and implementation status

This checkout began as a design-only baseline. The production STM32 pin map, signal polarities, calibration constants, and hardware thresholds are not released in the design pack. The firmware therefore compiles with an intentionally unassigned board configuration and enters an inert wait loop without touching production GPIO.

Do not remove that gate or energize an auger/check-mass actuator until all of the following are true:

- the peripheral/pin allocation has been validated in STM32CubeMX for the STM32G491RET6 LQFP64,
- the mapping and polarities have been checked against a released schematic and assembled board,
- safe startup and the normally-closed hardware E-stop have been verified with motion power isolated,
- current, speed, acceleration, timeout, and diagnostic limits have traceable bench values.

The project documentation is engineering guidance, not hazardous-location, machinery-safety, metrology, or process certification. A successful build or simulation does not establish safe powder handling or weighing accuracy.

## Build and test

PlatformIO environments are defined in `platformio.ini`:

| Environment | Purpose |
|---|---|
| `native` | Portable host tests on systems with GCC on `PATH`; intended for CI and Linux/macOS development. |
| `gravimetra_g491` | STM32G491RET6 firmware using STM32Cube HAL/LL and ST-Link upload support. |

From a PlatformIO shell:

```text
pio test -e native
pio run -e gravimetra_g491 -j 1
pio run -e gravimetra_g491 -j 1 -t upload
```

The native environment requires a current C++17-capable GCC or Clang on
`PATH`. The serialized target command avoids a parallel object/link race that
was observed in this Windows workspace.

The target environment uses PlatformIO's `nucleo_g491re` manifest only to select the matching STM32G491RET6 startup files, linker description, and toolchain. It does not adopt the Nucleo connector pinout.

## Repository map

```text
firmware/
  include/gravimetra/   Public module interfaces and fixed-capacity data types
  src/                  Portable implementations and target entry point
  test/                 Unity host tests and simulated fault/measurement traces
docs/                   Architecture, hardware bring-up, and calibration guidance
EMFC_Scale_Design_Pack/ Revision-A design inputs and local manufacturer references
AGENTS.md               Durable implementation context, progress, and blocker ledger
```

The detailed firmware contract is in `EMFC_Scale_Design_Pack/04_Firmware/Codex_Firmware_Implementation_Prompt.md`. `AGENTS.md` records what is actually implemented and verified.

## Hardware bring-up and calibration

- Follow `docs/HARDWARE_BRINGUP.md` before enabling target outputs.
- Follow `docs/CALIBRATION.md` for manual certified-mass calibration and internal check-mass policy.
- Read `docs/ARCHITECTURE.md` for module ownership, persistence semantics, and the target-integration gate.
- Preserve raw readings and every conversion intermediate in telemetry. Never replace missing hardware data with a plausible default.
