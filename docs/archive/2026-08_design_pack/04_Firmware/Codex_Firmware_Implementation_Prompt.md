# Codex Prompt - Firmware for the 250 g EMFC Scale and Four-Stage Auger System

You are implementing firmware for a custom precision electromagnetic-force-compensation (EMFC) scale and automated multi-stage powder feeder. The system is a general precision powder-dispensing machine. Do not redesign the hardware unless you find a concrete contradiction that makes correct firmware impossible. Do not invent hardware measurements or calibration constants.

## Hardware

### MCU

- STM32G491RET6, 170 MHz Cortex-M4F, LQFP64.
- Use a PlatformIO-compatible STM32 framework. Prefer STM32 HAL/LL with deterministic C++ architecture.
- No dynamic allocation after initialization in real-time/control paths.
- Use hardware timers for step pulse generation.

The existing repository may contain an older Feather 32U4 PlatformIO environment. Do not overwrite it blindly. Add a dedicated STM32G491 firmware environment/target and preserve unrelated existing targets unless intentionally deprecated and documented.

### Precision scale

- ADS1262 precision ADC over SPI with DRDY interrupt.
- 10-ohm Kelvin shunt measures force-coil current.
- OPA593 and analog circuitry close the fast EMFC position loop; firmware does NOT directly close the fast position loop in Revision A.
- Optical electronics expose position-error/difference and optical-sum diagnostics.
- TMP117 sensors at magnet/yoke, flexure body and precision AFE.
- OPA593 current-limit and thermal-warning signals go to MCU GPIO.
- Rated live capacity: 250 g.
- Target displayed readability: 0.01 grain = 0.647989 mg.
- Approximate signal increment at current design point: ~1.27 uV on the 10-ohm shunt per 0.01 grain. Treat this as a configuration/engineering value, not a magical hard-coded truth.

### Auger motors

- Four BigTreeTech TMC2209 StepStick-style driver modules.
- Four NEMA17 motors.
- Only one auger motor is allowed to run at a time.
- Each TMC2209 has dedicated STEP, DIR, EN and dedicated hardware UART.
- Do not implement closed-loop shaft-position feedback.
- Firmware must prevent simultaneous auger motion even if multiple commands are received.

The dispensing concept is staged. Example:

1. large/coarse auger supplies most requested mass quickly,
2. medium auger may approach the target,
3. fine auger approaches more slowly,
4. very-fine auger can finish the final increment.

The exact number of enabled stages and transition points must be configurable. Never hard-code assumptions that four stages are always present.

### HMI

- Nextion NX8048T050.
- 5 V powered externally; TTL UART to STM32.
- One dedicated hardware UART.
- Firmware owns the machine state and safety logic. The Nextion is a user interface, not the safety controller.
- Define a robust Nextion protocol layer with explicit page/component commands and parsed events.
- UI should eventually expose target mass, units, current mass, dispense state, stability state, tare, calibration, battery/power state, faults and service diagnostics.

### USB

- USB 2.0 device on USB-C connector.
- Use for firmware/service interface, telemetry/log extraction and configuration.
- Prefer CDC serial initially unless there is a compelling reason for another USB class.

### CAN

- STM32 FDCAN peripheral plus external TCAN1042-class CAN transceiver.
- CAN is a future machine/robot interface; implement a clean abstraction and basic bring-up support but do not invent the final robot protocol.

### RS-485

- Dedicated UART plus THVD1450-class half-duplex transceiver.
- Implement transport abstraction and direction-enable handling.
- Final higher-level protocol remains a TODO; Modbus RTU may be evaluated later but should not be assumed unless selected.

### Battery/power

- Protected 2S Li-ion pack, approximately 7.4 V nominal.
- Recommended baseline capacity around 10 Ah / 74 Wh.
- MAX77960B-class charger/power path.
- Wall power uses Mean Well 12 V and 5 V modules.
- Firmware can monitor power-source and charger status and should reduce/suspend charging during final precision settling if hardware control is available.

### E-stop

- Hardware normally-closed E-stop removes motion capability independently of firmware.
- When E-stop is active, all augers and automatic calibration actuator must be disabled.
- MCU, HMI and scale remain powered so the system can display/log the event.
- Firmware must monitor E-stop state but must never be the only mechanism that stops motion.

### Automatic check mass

- Provision for an actuator that transfers a known internal check mass onto the scale.
- Actuator must be stopped/disabled and mechanically unloaded from the scale during the stable measurement.
- Provide two digital sensor inputs for home/end state.
- Automatic check is not a replacement for traceable manual calibration.

## Required architecture

Create modules/classes for at least:

1. Board/pin configuration
2. ADS1262 driver
3. TMP117 driver
4. TMC2209 driver/wrapper
5. Auger manager
6. Dispense planner/state machine
7. Nextion HMI transport and UI model
8. USB service/telemetry
9. CAN transport
10. RS-485 transport
11. Power/charger manager
12. E-stop/safety-input manager
13. EMFC measurement engine
14. Scale calibration model
15. Temperature compensation model
16. Filtering
17. Stability detector
18. Internal check-mass controller
19. System state machine
20. Fault manager
21. Nonvolatile configuration/calibration store
22. Structured logger/telemetry
23. Watchdog/health supervisor

Keep drivers independent from application policy.

## System states

At minimum:

- BOOT
- SELF_TEST
- WARMUP
- ZERO
- READY
- DISPENSE_STAGE_1
- DISPENSE_STAGE_2
- DISPENSE_STAGE_3
- DISPENSE_STAGE_4
- SETTLE
- VALIDATE
- REPORT
- UNLOAD_OR_CLEANUP
- AUTO_CHECK_CAL
- MANUAL_CALIBRATION
- SERVICE
- FAULT
- ESTOP

Stages that are disabled in configuration should be skipped cleanly.

## Dispensing algorithm framework

Do not hard-code a gunpowder-specific feed law. Implement a general mass-feedback feeder.

Each configured stage should have:

- motor channel,
- direction,
- run current,
- hold current,
- microstep mode,
- maximum speed,
- acceleration,
- minimum pulse/run amount,
- transition error threshold,
- optional learned material-in-flight/settling compensation,
- timeout,
- maximum allowed overshoot.

Only one stage may command motion at a time.

The algorithm should:

1. tare/verify empty pan,
2. accept target mass,
3. select coarse stage,
4. continuously monitor responsive scale estimate,
5. stop before target according to configured transition/predictive margin,
6. settle sufficiently to estimate delivered mass,
7. continue using successively finer stages,
8. enter final precision settling,
9. validate stability,
10. report result or fault/underfill/overfill state.

Do not continuously run a motor while using the final low-noise ADC profile. Explicitly transition between active-dispense and settle/read profiles.

## ADS1262 requirements

Implement register-level driver functions for reset, read/write registers, start/stop, channel selection, filter/data-rate/PGA/reference selection, signed conversion acquisition, DRDY handling, timeouts, status/fault interpretation and configuration verification.

Implement at least two profiles:

### FAST_DISPENSE

- relatively high output data rate,
- low latency,
- responsive filtering.

### PRECISION_SETTLE

- lower-noise data rate,
- 50/60 Hz rejection where useful,
- proper filter settling delay after profile change,
- reduced unrelated bus/UI chatter if testing demonstrates measurable coupling.

Do not assume 32 usable bits.

## Measurement conversion

Pipeline:

raw ADC code -> differential voltage -> coil current -> uncorrected mass -> temperature compensation -> tare correction -> final mass.

Expose every intermediate quantity for telemetry.

Use central configuration structures for:

- actual shunt resistance,
- reference voltage calibration,
- lever/scale factor coefficients,
- polynomial nonlinearity terms,
- temperature coefficients,
- unit conversion.

Exact conversion:

    1 grain = 64.79891 mg

## Stability detector

A measurement is stable only when all configured criteria are satisfied continuously:

- absolute slope below limit,
- standard deviation below limit,
- peak-to-peak below limit,
- sufficient number of valid samples,
- minimum stable duration,
- optical position error within bounds,
- coil current not saturated,
- no relevant power/temperature fault.

Return diagnostic reasons when unstable.

## Calibration

Support manual multi-point calibration at configurable nominal points; initially support:

- 0 g
- 25 g
- 50 g
- 100 g
- 150 g
- 200 g
- 250 g

Store the actual certified conventional-mass value when supplied rather than assuming nominal exactly.

Support:

- zero offset,
- linear scale,
- optional quadratic/cubic correction if justified by residuals,
- temperature coefficients,
- residual calculation,
- repeatability checks,
- calibration rejection when quality thresholds fail,
- CRC/versioned atomic storage.

## Internal check mass

Implement a separate verification sequence:

1. verify scale empty and stable,
2. command check-mass actuator,
3. verify actuator sensor state,
4. disable actuator drive,
5. wait for precision stability,
6. compare reading with stored certified/check-mass value,
7. log error,
8. optionally update a narrowly defined span correction only if policy enables it,
9. remove mass and verify zero return.

Do not silently recalibrate from a failed check.

## TMC2209 management

Use dedicated hardware UART per module.

Provide functions for:

- initialization and register verification,
- run/hold current,
- microstep configuration,
- StealthChop/SpreadCycle selection,
- driver status/fault retrieval,
- enable/disable,
- STEP pulse timing via hardware timer,
- direction setup timing,
- commanded step counting for diagnostics only.

Position feedback is not required.

## Nextion

Create a nonblocking parser and command queue.

The HMI must not directly manipulate hardware. It sends requests to the application state machine.

Initial pages/models should support:

- Home/Run
- Target setup
- Live dispensing
- Calibration
- Internal check
- Diagnostics
- Power/battery
- Fault history
- Service/configuration

Do not block the control loop while waiting for HMI responses.

## Faults

At minimum:

- ADS1262 timeout/config mismatch/overrange
- optical difference invalid
- optical sum invalid
- coil current overrange/saturation
- OPA593 current-limit or thermal warning
- TMP117 missing/out of range
- excessive zero drift
- calibration invalid/CRC failure
- battery undervoltage
- charger fault
- 5 V/12 V rail fault if sensed
- TMC2209 communication fault
- TMC2209 overtemperature/short/open-load diagnostic as supported
- motor-motion timeout
- two-motors-requested interlock violation
- stability timeout
- internal check-mass actuator timeout
- internal check failure
- E-stop active
- watchdog/reset history

Fault records must capture a measurement/power/state snapshot.

## Nonvolatile storage

Use versioned, CRC-protected data with atomic/dual-copy update.

Store:

- scale calibration,
- temperature compensation,
- stability thresholds,
- auger stage configuration,
- TMC current/microstep settings,
- HMI/service settings,
- check-mass certified value/tolerance,
- communication settings,
- calibration sequence/timestamp if available.

Abstract storage so external FRAM can be added later.

## Watchdog

Use IWDG. Refresh it only from a health supervisor that confirms all required subsystems/tasks have advanced. Do not refresh it unconditionally in the main loop.

## Logging

Structured CSV or binary telemetry should include:

- monotonic time,
- state,
- raw ADS1262 code,
- shunt voltage,
- coil current,
- raw/corrected/tared mass,
- optical difference/sum,
- all temperatures,
- active auger/stage,
- commanded motor speed/steps,
- TMC driver status,
- battery voltage,
- source/charger state,
- E-stop state,
- ADC profile,
- stability metrics,
- fault flags,
- calibration version.

## Communications

Keep application messages independent of transport so later robot integration can use CAN, RS-485 or USB without rewriting scale/dispense logic.

Implement basic service framing and leave the final external robot protocol as a clearly documented TODO.

## Tests

Create tests/simulated traces for:

- unit conversions,
- ADC code conversion,
- calibration polynomial,
- temperature correction,
- filters,
- stability decisions,
- tare acceptance/rejection,
- staged dispense transitions,
- attempt to run two motors simultaneously,
- E-stop during motion,
- noisy settling,
- slow drift,
- overshoot,
- ADC timeout,
- TMC fault,
- internal check success/failure,
- calibration CRC failure,
- power-source transition.

## Project organization

Use a modular tree such as:

    src/
      main.cpp
      app/
      drivers/
      measurement/
      motion/
      hmi/
      comms/
      power/
      safety/
      calibration/
      system/

Adapt to repository conventions where sensible.

## Milestones

1. Add STM32G491 PlatformIO target.
2. Establish pin map and safe startup.
3. USB CDC and SWD bring-up.
4. ADS1262 raw acquisition.
5. TMP117 acquisition.
6. scale voltage/current/mass pipeline.
7. filter/stability modules.
8. nonvolatile configuration/calibration.
9. one TMC2209 channel and timer-generated STEP.
10. expand to four TMC2209 UART channels with one-motor interlock.
11. Nextion transport/UI model.
12. system/dispense state machine.
13. charger/power monitoring.
14. E-stop integration.
15. internal check-mass controller.
16. CAN/RS-485 service bring-up.
17. fault manager/watchdog.
18. automated tests and simulated traces.
19. hardware bring-up documentation.

At every milestone compile, test, fix warnings and keep prior working behavior intact.

## Deliverables

- buildable firmware,
- PlatformIO configuration,
- pin-map header,
- drivers and modules above,
- unit/simulation tests,
- README with build/flash/bring-up/calibration procedures,
- documented TODOs for hardware-dependent tuning,
- no fake measurements or invented calibration results.
