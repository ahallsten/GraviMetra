# Firmware Architecture and Assurance Boundary

The current tree is a portable, deterministic firmware core plus an intentionally inert STM32 entry point. It is not a motion-capable board image: the Revision-A pin map, output polarities, clock tree, interrupt/DMA allocation, flash pages, and calibrated constants are not released. `main.cpp` performs HAL initialization and waits without configuring production GPIO while that gate is closed.

## Ownership and data flow

```text
target HAL adapters (pending released board map)
  -> register drivers: ADS1262, TMP117, TMC2209
  -> measurement: conversion -> temperature compensation -> tare -> stability
  -> policy: dispense planner / check-mass controller
  -> actuators: auger manager / check-mass adapter

Nextion / USB / CAN / RS-485
  -> transport-independent requests
  -> system lifecycle authority
  -> domain controllers

hardware E-stop (independent removal)
  -> supplementary firmware E-stop latch
  -> stop every STEP timer and inhibit all motor/check-mass outputs

all domains
  -> fault snapshots / telemetry / health epochs
  -> atomic configuration envelope
```

Drivers own register protocols and device verification, not dispensing or calibration policy. The auger manager owns the lower-level one-motor interlock; the planner cannot bypass it. The system state machine owns lifecycle authorization and treats HMI/communications as request sources only. Application messages and byte transports are separate so a final robot protocol can be added without rewriting scale logic.

## Determinism rules

- Control-facing containers and queues have fixed capacities.
- Hardware STEP generation is represented by `StepPulseTimer`; target adapters must use timer output, never delay-loop bit banging.
- HAL calls are nonthrowing and return explicit status. A target UART implementation must satisfy the buffer-ownership contract in `hal/interfaces.hpp`.
- No hardware-dependent numeric value becomes usable merely because it is default constructed. Configuration and calibration records must be explicitly validated and accepted.
- Profile changes invalidate old measurement readiness; motion policy must consume data from the requested, verified ADC profile only after its settling boundary.
- Invalid, stale, clipped, nonfinite, or diagnostically faulted measurements fail closed.

## Persistence boundary

`AtomicConfigStore` is a versioned CRC envelope for an application-defined stable serialization; raw compiler structs are not a storage format. Its two slots must occupy distinct complete erase units. The caller declares program and erase geometry, and the backend acknowledges writes only after they are durable. Header, padded payload, and final commit block are programmed in order. The newest integrity-valid generation is authoritative; a schema mismatch requires explicit migration and does not silently select an older schema.

The aggregate machine-configuration schema and its migration policy must be finalized with the released hardware configuration. It must cover scale/temperature calibration, stability policy, stage/TMC settings, service/HMI settings, check-mass metadata, communications settings, and calibration provenance.

## Target integration still gated

When hardware facts are released, target integration should instantiate adapters and domain services in this order:

1. Configure all actuator outputs to their proven disabled electrical levels before enabling peripheral ownership.
2. Capture reset cause and initialize retained fault history.
3. Prove the monitored E-stop contact and retain the hardware-independent inhibit.
4. Load and validate configuration/calibration; remain inhibited on missing, corrupt, incompatible, or rejected data.
5. Verify device identities and critical register readback.
6. Start bounded acquisition/safety/application scheduling and enable watchdog refresh only after every required subsystem produces fresh progress.
7. Allow operator-requested motion only after all preceding gates and the external hardware E-stop path have been bench verified.

The intended scheduling priorities are safety input and timer shutdown first, acquisition/DRDY next, measurement and policy next, and bounded HMI/log transport last. Exact rates, IRQ priorities, DMA channels, and timeout values require measured target evidence and must not be inferred from host tests.

## Verification vocabulary

- **Host verified** means deterministic logic passed the Unity simulation suite on a current C++17 compiler.
- **Target compiled** means every production translation unit compiled for STM32G491RET6; garbage collection may remove modules not instantiated by the inert entry point.
- **Hardware verified** requires a recorded board revision, released pin map, instruments, configuration version/CRC, raw logs, and completed gates from `HARDWARE_BRINGUP.md`.

Only the first two levels are available in this repository checkpoint. See root `AGENTS.md` for the exact last-known-good commands, counts, blockers, and next safe action.
