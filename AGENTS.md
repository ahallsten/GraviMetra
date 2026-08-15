# GraviMetra Engineering Context

This file is the durable handoff for work in this repository. Update it whenever an implementation milestone, architectural decision, verification result, or hardware blocker changes. Keep transient scratch notes out of it.

## Current objective

Build deterministic C++ firmware for the 250 g Revision-A EMFC scale and configurable staged auger feeder described in `EMFC_Scale_Design_Pack/04_Firmware/Codex_Firmware_Implementation_Prompt.md`.

The firmware must be buildable for the STM32G491RET6, have host-executable tests for hardware-independent logic, and never substitute fabricated measurements or calibration values for missing hardware facts.

## Sources of truth

Read these before changing architecture or hardware-facing code:

1. `EMFC_Scale_Design_Pack/05_AI_Reference/AI_Reference_Context.md`
2. `EMFC_Scale_Design_Pack/01_Electrical_Design/Electrical_Architecture_and_Component_Review.md`
3. `EMFC_Scale_Design_Pack/01_Electrical_Design/Connector_and_IO_Plan.md`
4. `EMFC_Scale_Design_Pack/02_EMFC_Mechanics/250g_EMFC_Design_Guide.md`
5. `EMFC_Scale_Design_Pack/04_Firmware/Codex_Firmware_Implementation_Prompt.md`
6. Local manufacturer PDFs under `EMFC_Scale_Design_Pack/01_Electrical_Design/Datasheets/`

If documents disagree, do not silently choose one. Record the conflict here and keep the affected behavior disabled or configurable until the owner resolves it.

## Frozen system facts and invariants

- MCU: STM32G491RET6, 170 MHz Cortex-M4F, LQFP64.
- Revision A closes the fast EMFC position loop in analog circuitry. Firmware supervises and measures it; firmware does not close that loop.
- Primary measurement is the ADS1262 differential reading across a nominal 10-ohm four-terminal shunt. Actual shunt resistance and reference voltage are calibration/configuration data.
- Exact mass conversion is `1 grain = 64.79891 mg`.
- A displayed increment is a readability target, not a claim of accuracy.
- Four TMC2209 auger channels are provisioned, but enabled stage count and order are configuration. At most one auger may be enabled or pulsed at any time.
- STEP pulses must come from hardware timers on target hardware. Commanded steps are diagnostics, not closed-loop position feedback.
- The normally-closed hardware E-stop independently removes motion capability. Firmware monitoring is supplementary and must disable all augers and the check-mass actuator.
- The Nextion sends requests only. Application state and safety authority stay in firmware.
- Active dispensing and precision settling use distinct ADC profiles. Motors may not run continuously in the final low-noise profile.
- No dynamic allocation is permitted after initialization in real-time or control paths.
- Drivers must remain independent of application policy, and application messages must remain transport-independent.
- Calibration/configuration persistence must be versioned, CRC-protected, and atomic/dual-copy.
- The watchdog may be refreshed only after the health supervisor verifies progress from required subsystems.
- Do not assume Modbus RTU or invent the final CAN/RS-485 robot protocol.

## Hardware facts still unresolved

These are deliberate blockers, not values to guess:

- Final STM32 peripheral and pin assignment. The design pack requires validation in CubeMX before schematic capture; this repository does not yet contain a released schematic/netlist or CubeMX project.
- Signal polarities for E-stop monitoring, OPA593 warnings, driver enables, charger controls, and check-mass sensors.
- Final ADC rates, digital filters, PGA/reference selections, channel map, and measured profile-settling times.
- Actual shunt resistance, reference calibration, coil constant, lever coefficients, tare/span/nonlinearity coefficients, and temperature coefficients.
- Stability thresholds, optical diagnostic limits, power limits, and all hardware-dependent timing limits.
- Final TMC2209 currents, microsteps, chopper modes, UART wiring details, and motor direction conventions.
- Charger-control availability, battery/rail sensing transfer functions, check-mass actuator type, and certified check-mass value.
- Final external machine protocol over CAN or RS-485.

Code may define typed fields and invalid/unconfigured sentinels for these items. It must fail safe and report configuration invalidity rather than supplying plausible-looking defaults.

## Architecture direction

- Put portable deterministic domain logic in normal C++ modules with no HAL dependency so it can run in host tests.
- Isolate STM32 HAL/LL calls behind small interfaces for clocks, GPIO, SPI, I2C, UART, timers, flash, USB, FDCAN, watchdog, and monotonic time.
- Use fixed-capacity arrays, queues, buffers, and snapshots in control paths.
- Make safety checks layered: safe GPIO startup, E-stop gate, one-motor interlock, state-machine authorization, timeout/fault shutdown, and hardware-independent E-stop removal.
- Expose raw ADC code, voltage, current, uncorrected/corrected/tared mass, stability diagnostics, state, active motion, temperatures, power, and faults in telemetry.
- Treat internal check mass as verification by default. A failed check must never silently alter calibration.

## Repository state and progress

As of 2026-08-14:

- The repository contains an MIT license and the Revision-A design pack.
- The design pack is currently untracked in Git; preserve it and do not rewrite its source documents as part of firmware implementation.
- The attachment provided with the task and the in-repository firmware prompt are content-identical after line-ending normalization.
- No pre-existing firmware source, PlatformIO configuration, tests, generated pin map, or CubeMX project was found.
- A new `firmware/` tree, root `platformio.ini`, root README, CI workflow, and gated bring-up/calibration documentation are now present.
- The target environment is `gravimetra_g491`. It uses the `nucleo_g491re` manifest only as the matching STM32G491RET6 toolchain/startup carrier; it does not adopt Nucleo connector pins.
- Resolved target packages on 2026-08-14: PlatformIO Core 6.1.19, ST STM32 platform 19.7.1, STM32CubeG4 framework 1.6.1, and Arm GCC 7.2.1.

### Milestone log

- [x] Audited the implementation prompt and high-level design context.
- [x] Established this root-level durable context/handoff file.
- [x] Add a dedicated STM32G491 PlatformIO target and host-test environments.
- [x] Establish a compile-time pin-map contract and inert safe startup without pretending the unresolved map is released.
- [ ] Implement and test the measurement, calibration, compensation, filtering, and stability core.
- [ ] Implement and test the motion interlock and staged dispense planner.
- [ ] Add hardware drivers, HMI/comms, power/safety, check-mass, fault, storage, telemetry, and health-supervisor modules.
- [ ] Complete target builds, simulated trace tests, and bring-up/calibration documentation.

### Validation evidence

| Date | Scope | Command | Result | Limitation |
|---|---|---|---|---|
| 2026-08-14 | Inert STM32 foundation | `pio run -e gravimetra_g491` | PASS; 564 B flash, 40 B RAM | Compilation only; no board connected, clock/pins intentionally unreleased. |
| 2026-08-14 | Windows host foundation | `pio test -e windows_host -f test_foundation` | Toolchain installed, compile rejected modern C++ namespace syntax | Bundled MinGW 5.1 predates the C++17 baseline; use `native` with a current host GCC/Clang. |

## Working rules

- Preserve unrelated/user-authored changes. Check `git status --short` before and after edits.
- Treat every hardware-dependent default as suspect; document its provenance or mark it invalid/unconfigured.
- Keep compiler warnings enabled and fix warnings introduced by firmware code.
- Run the narrowest relevant host tests while iterating, then the full host suite and STM32 target build before declaring a milestone complete.
- Update the milestone log and unresolved-hardware section in this file at every meaningful handoff.
- Do not claim hardware bring-up, calibration quality, accuracy, or safety validation from simulation-only results.
