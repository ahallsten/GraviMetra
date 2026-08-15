# Hardware Bring-Up

This sequence is deliberately gated. Stop at a failed gate, preserve logs, and correct the cause before proceeding. Simulation and compilation cannot satisfy a hardware gate.

## 1. Design-release gate

Before compiling a motion-capable image:

1. Validate all required peripherals in STM32CubeMX for STM32G491RET6 LQFP64: four dedicated TMC2209 UARTs, Nextion UART, RS-485 UART, ADS1262 SPI and DRDY interrupt, TMP117 I2C, USB device, FDCAN, SWD, and timer STEP outputs.
2. Export or review the generated alternate-function assignment against the released schematic.
3. Record every signal's electrical polarity, reset state, pull configuration, voltage domain, connector pin, and safe state.
4. Replace the unreleased board configuration only through a reviewed board-specific file. Set its release flag last.
5. Confirm that timer channels can generate STEP pulses without software delay loops and that no pin conflicts with SWD, USB, oscillator, boot, or analog supplies.

## 2. Unpowered inspection

- Verify controller, precision board, driver-module orientation, cable keying, protective earth, E-stop wiring, fusing, and isolated mains/SELV partitions.
- Leave stepper VM and the check-mass actuator supply disconnected.
- Check resistance to ground and for shorts on all rails before applying power.
- Verify the four-terminal shunt sense routing and precision-board cable separately from motor wiring.

## 3. Controller-only power and SWD

- Power only the low-voltage controller logic from a current-limited bench source.
- Flash the inert firmware and verify SWD reset/halt behavior.
- Confirm the MCU clock source and actual frequency from the reviewed CubeMX clock tree. The initial inert image intentionally does not claim a 170 MHz runtime clock configuration.
- Inspect reset cause and confirm that every motion/check-actuator output remains disabled during reset, boot, debugger halt, and firmware fault.
- Reserve and link two distinct nonvolatile erase units for the atomic configuration store. Verify the backend's program granularity, erase granularity, one-way programming behavior, and ordered durable-write contract with power-interruption tests before storing calibration.

## 4. E-stop and safe-output gate

- With motion power still isolated, exercise the normally-closed E-stop loop and monitored contact.
- Verify the hardware path removes motion capability without MCU participation.
- Verify firmware detects both asserted and wiring-fault/open-loop conditions according to the released electrical design, latches/logs the event, stops all timer channels, disables every TMC2209, and de-energizes the check actuator.
- Test reset, brownout, watchdog, and communication-flood cases. None may create a STEP pulse or enable motion.

## 5. USB/service and static interfaces

- Bring up USB CDC for service telemetry before motor activity.
- Confirm monotonic timestamps, configuration validity reporting, reset history, and fault snapshot extraction.
- Bring up Nextion, CAN, and RS-485 in receive/loopback modes without granting them hardware authority.

## 6. Precision board

- Power the precision board with motion and charging noise sources disabled.
- Verify precision rails, reference voltage, ADS1262 identity/register readback, DRDY timing, input mux, status bytes, and raw signed codes.
- Verify every TMP117 identity/configuration and plausible ambient reading; location/address mapping must match the schematic.
- Confirm OPA593 current-limit/thermal diagnostics, optical difference/sum diagnostics, and analog-loop reset behavior without applying a calibration claim.
- Characterize the actual fast and precision ADC profiles, including latency and settling after a profile change. Store selected settings as traceable configuration.

## 7. Measurement characterization

- Enter measured shunt resistance and reference calibration with provenance.
- Log raw code, differential voltage, coil current, uncorrected mass estimate, compensated mass, tare, optical diagnostics, and temperatures.
- Establish noise, drift, warm-up, overload, saturation, and environmental behavior before selecting stability limits.
- A display increment is not an accuracy result. Do not enable automatic validation until configured stability criteria are justified by captured traces.

## 8. One motor, then four channels

- Use a current-limited motor supply and mechanically safe test fixture.
- Validate one TMC2209 UART, current setup, enable polarity, direction setup time, hardware-timer STEP waveform, stop latency, timeout, and diagnostics.
- Exercise E-stop during motion before enabling another channel.
- Repeat per channel, then test competing requests and confirm the lower-level one-motor interlock prevents simultaneous timer activity and enables.
- Tune current, speed, acceleration, minimum run, and material-in-flight compensation only from the actual motor/auger/material test program.

## 9. Integrated dispensing

- Begin with inert, nonhazardous material and containment appropriate to the mechanism.
- Verify explicit switching between responsive dispensing and precision settling profiles.
- Capture noisy settling, slow drift, underfill, overshoot, power-source transitions, driver faults, ADC loss, and E-stop traces.
- Enable successively finer configured stages only after each preceding stage and fault path is repeatable.

Record each completed gate, firmware commit, board revision, instruments, configuration CRC/version, and raw logs in the engineering test record. Update `AGENTS.md` with durable conclusions and remaining blockers.
