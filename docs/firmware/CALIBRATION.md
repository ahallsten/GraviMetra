# Calibration and Internal Check Policy

Calibration must use traceable certified conventional-mass values and a defined environmental/test procedure. Nominal labels such as 25 g or 250 g are prompts for point selection; they are not substitutes for the certificate values.

## Prerequisites

- Hardware bring-up through stable raw measurement is complete.
- Warm-up duration and environmental limits have been established from prototype data.
- The pan is empty, clean, correctly installed, and protected from drafts/vibration/static effects appropriate to the test.
- ADS1262 precision-profile settling and stability limits are configured from measured traces.
- Optical, coil-current, temperature, power, and amplifier diagnostics are healthy.
- Calibration storage has passed version, CRC, and atomic-update recovery tests.

## Manual multi-point sequence

The initial nominal sequence is 0, 25, 50, 100, 150, 200, and 250 g; points remain configurable.

For each point:

1. Prompt for the actual certified conventional-mass value and certificate/reference identifier.
2. Verify the expected load range before accepting data.
3. Wait until every configured stability criterion remains satisfied for the minimum duration.
4. Capture the configured number of raw and intermediate samples plus temperatures and optical diagnostics.
5. Repeat loading where required to measure repeatability and hysteresis rather than fitting a single pass blindly.
6. Preserve rejected observations and their diagnostic reasons in the service log.

Fit zero plus a linear scale first. Evaluate quadratic or cubic terms only when configured residual/repeatability criteria show that the additional term is justified and sufficiently constrained by the available points. Reject a candidate when configured quality thresholds fail; do not silently retain a poor fit.

Commit an accepted model through the versioned, CRC-protected dual-copy store. A reset or interrupted write must leave either the previous valid calibration or the new valid calibration recoverable.

The storage backend must declare its real program and erase geometry. Each copy occupies a distinct complete erase unit, and a successful backend write/erase means the operation is already durable. For STM32G4 internal flash, use the released device/page geometry and 64-bit programming unit; do not place both copies in one flash page or acknowledge a buffered write as committed. Schema migration is explicit: firmware never silently loads an older-schema record when a newer valid record exists.

## Per-cycle tare

A tare is accepted only when the empty pan is stable, diagnostics are healthy, load lies inside the configured tare window, and the configured sample/duration criteria are satisfied. Tare removes the current offset; it must not alter span, nonlinearity, or temperature coefficients.

## Temperature compensation

Keep magnet/yoke, flexure/frame, and precision-AFE temperature terms explicit. Coefficients require calibration evidence over the intended temperature range. An absent sensor, out-of-range value, or unconfigured coefficient must be visible in diagnostics; firmware must not invent a compensation value.

## Internal check mass

The internal mass is verification by default:

1. Verify the empty scale and stable zero.
2. Apply the mass and verify actuator sensors.
3. Disable actuator drive and mechanically unload/decouple the actuator while the mass remains on the scale.
4. Wait for precision stability and compare with the stored certified check-mass value/tolerance.
5. Log the result and complete snapshot.
6. Remove the mass and verify actuator home plus stable zero return.

A failed check raises a fault and never silently changes the calibration. A narrowly defined span correction may be evaluated only when an explicit policy is enabled, its bounds and audit behavior are configured, and the mechanical/metrology rationale is documented.
