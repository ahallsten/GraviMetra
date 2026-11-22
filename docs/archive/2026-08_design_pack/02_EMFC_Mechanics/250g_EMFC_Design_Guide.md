# 250 g EMFC Scale Design Guide

## 1. Target

- Capacity: 250 g.
- Target displayed readability: 0.01 grain = 0.647989 mg.
- Commercial benchmark: Sartorius WZB254-NC, 250 g capacity, 0.1 mg readability, approximately <=0.1 mg repeatability standard deviation and <=+-0.2 mg linearity under stated conditions.
- Internal engineering resolution should be finer than the display increment; aim for approximately 0.05-0.10 mg-equivalent digital resolution/noise where practical.

This is an ambitious custom instrument, but the performance class is demonstrated by commercial EMFC cells. The difficult parts are mechanical geometry, thermal stability, environmental isolation and calibration, not merely ADC bit count.

## 2. Operating principle

EMFC is a force-restoration system rather than a strain-gauge bridge.

1. Applied mass pushes on the pan/load receptor.
2. Flexures permit a tiny, well-defined displacement in the intended degree of freedom.
3. An optical null detector senses departure from the zero position.
4. An analog servo drives current into a moving coil inside a permanent-magnet field.
5. The coil restores the lever to null.
6. Precision measurement of the restoring current determines mass.

At equilibrium:

    F_load * L_load = F_coil * L_coil

and approximately:

    F_coil = kF * I

therefore mass is proportional to restoring current after geometry, calibration and temperature compensation are included.

## 3. Force scaling for the revised 250 g target

Full-scale load force:

    0.250 kg * 9.80665 m/s^2 = 2.45166 N

Use approximately 25:1 mechanical force reduction to keep the voice-coil electrical operating range near the earlier 50 g design.

    F_coil_FS = 2.45166 / 25 = 0.09807 N

With approximately 2 N/A actuator constant:

    I_FS = 0.09807 / 2 = 49.0 mA

Recommended practical range:

- empty-pan holding current after counterbalance: approximately 5-10 mA,
- 250 g load: approximately 54-60 mA,
- normal loop capability: 0-65 mA,
- hardware current limit: approximately 80 mA.

One 0.01-grain increment is approximately 6.354 uN at the pan. With 25:1 leverage and 2 N/A coil constant, this is about 0.127 uA of coil current. Across a 10-ohm shunt it is about 1.27 uV.

## 4. What the mechanical guides actually do

The moving member must not be allowed to freely float like a loudspeaker cone. An EMFC cell needs a **constraint system** that permits only the intended tiny vertical/rotational measurement motion while constraining lateral translation, yaw, roll and unwanted pitch.

There are two different concepts that are often conflated:

### Three-point or tripod support

A three-point base support is useful for mounting the entire scale because three points define a plane without rocking. A tripod/three-foot base can therefore help with:

- stable machine mounting,
- leveling,
- avoiding overconstraint of the entire scale chassis.

It does **not** provide the frictionless internal guidance required by the force-restoration mechanism.

### Flexure guidance

The internal moving load receptor should be guided by elastic flexures, ideally a monolithic parallel-flexure arrangement. Flexures provide motion through elastic bending, with no rolling bearings, sliding guides or lubricated pivots.

This avoids:

- stiction,
- bearing friction,
- backlash,
- lubricant viscosity changes,
- wear debris,
- mechanical hysteresis from sliding contact.

The internal mechanism should therefore resemble a parallel-guided flexure and lever, not a shaft sliding in bushings.

## 5. Recommended mechanical topology

Conceptual layout:

    powder pan / tray
          |
    load receptor
          |
    parallel flexure guide
          |
    short load-side lever
          +=============================== long coil-side lever
                                                   |
                                             optical vane
                                                   |
                                               voice coil
                                                   |
                                          permanent magnet gap

Use a monolithic flexure body if machining capability permits.

Recommended design goals:

- 25:1 effective force ratio, possibly using two flexure-lever stages if one long lever is impractical.
- very small closed-loop pan displacement.
- high lateral/torsional stiffness.
- hard overtravel stops before flexure yield.
- mechanical transport lock.
- mechanically adjustable counterbalance/preload.
- symmetric geometry wherever possible to reduce thermal and corner-load errors.

## 6. Material and flexure design

First prototype material: 7075-T651 aluminum.

Why:

- high strength-to-weight ratio,
- easy precision machining,
- common in monolithic flexure mechanisms,
- sufficient elastic range for thin flexures.

Later comparison: precipitation-hardened 17-4PH stainless steel if thermal/mechanical testing justifies it.

Do not copy generic flexure dimensions into production. Perform FEA for:

- 250 g rated load,
- intended overload,
- transport shock,
- peak von Mises stress,
- fatigue margin,
- parasitic rotations,
- lateral stiffness,
- first several resonance modes,
- lever-ratio variation over travel,
- thermal expansion symmetry.

The closed-loop sensor normally moves very little, but the flexures must also survive startup, overload and power-off conditions.

## 7. Load receptor and corner loading

A weighing pan creates moments when material lands away from center. The parallel-flexure guide must prevent these moments from substantially changing the measured force.

Design and test:

- center load,
- four corners,
- four edge midpoints,
- repeated removal/reinstallation of the pan.

Keep the load receptor stiff relative to the flexures. A thin flexible pan support can turn an otherwise excellent EMFC cell into a poor scale.

## 8. Counterbalance and dead load

Avoid wasting coil range holding up the empty tray.

Possible coarse-balance mechanisms:

- adjustable counterweight,
- screw-positioned balance mass,
- low-creep spring preload if characterized,
- magnetic preload if temperature dependence is acceptable.

Preferred first prototype: mechanical adjustable counterweight placed on the lever, because its behavior is easy to understand and it does not dissipate heat.

Target empty-pan current near 5-10 mA so both positive correction and full live load remain inside the actuator range.

## 9. Voice coil

Initial target:

- Sm2Co17 permanent magnets preferred for better temperature stability than ordinary NdFeB.
- approximately 0.4-0.6 T air-gap flux density.
- approximately 2 N/A force constant.
- approximately 15-25 ohm coil resistance.
- approximately 5-60 mA normal current.
- <=80 mA hardware-limited current.
- approximately 100 turns as an initial magnetic-model starting point, not a production specification.

First-order rectangular coil relationship:

    F approximately 2 * B * l * N * I

Magnetic modeling should be done in FEMM, Ansys Maxwell or equivalent before final dimensions are frozen.

### Coil lead routing

The two flexible coil wires themselves exert force. Route them:

- together as a pair,
- symmetrically,
- near the mechanism's neutral axis,
- with a repeatable low-force service loop,
- away from the highest magnetic field,
- without touching the enclosure during travel.

Measure zero shift while gently moving the cable outside the mechanism; if the reading changes, the lead dress is influencing the balance.

## 10. Optical null detector

Recommended components:

- Vishay TSAL6200 940 nm IR LED.
- Hamamatsu S3096-02 split photodiode.
- moving knife edge/vane attached to the guided lever.

At null, both photodiode segments receive approximately equal light.

    error = left - right
    health = left + right

Use the difference for the analog position loop and the sum for diagnostics.

Design the optical cavity to reject room light and dust. Provide physical adjustment during prototype development, then convert the successful geometry into fixed datum surfaces.

## 11. Analog servo

The fast position loop is analog in Revision A.

Path:

    split photodiode -> dual TIA -> differential error -> PI compensator -> OPA593 current servo -> coil -> mechanics

Why analog first:

- deterministic,
- independent of MCU scheduling,
- mechanism remains controlled during firmware activity,
- easier plant characterization,
- less risk of discrete-time instability during early mechanical development.

Target initial closed-loop bandwidth: approximately 5-15 Hz, then tune experimentally.

Do not finalize PI component values until the actual plant frequency response is measured.

Provide:

- selectable feedback resistors/capacitors,
- analog loop-injection point,
- servo reset/clamp via TMUX1101,
- test points for photodiode L/R, difference, sum, servo output, coil current and OPA593 output.

## 12. Thermal strategy

Temperature changes affect:

- permanent magnet flux,
- Young's modulus of flexure material,
- lever dimensions,
- coil resistance/self-heating,
- current shunt,
- voltage reference,
- op-amp offset,
- thermoelectric junction voltages.

Measure at least three temperatures:

1. magnet/yoke,
2. flexure body,
3. shunt/reference/ADC region.

Keep the charger, motor drivers and switching regulators away from the scale body. Avoid blowing cooling air directly across the mechanism.

Record temperature derivatives during characterization; transient gradients can matter more than final ambient temperature.

## 13. Environmental enclosure

At sub-milligram readability, an enclosure is part of the sensor.

Provide:

- draft shield around the pan,
- no direct fan airflow,
- grounded conductive structure around powder-handling components where appropriate,
- stiff isolated mounting away from auger vibration,
- adjustable three-foot base or equivalent leveling system for the scale chassis,
- bubble/electronic level provision if needed,
- thermal separation between electronics and mechanism.

## 14. Static control and energetic-powder safety

For combustible/energetic powders:

- keep mains, relays, motor drivers and charger switching outside the powder zone,
- bond conductive chassis and powder-contact hardware to protective earth,
- avoid isolated conductive objects that can charge electrostatically,
- use static-dissipative materials where appropriate,
- avoid exposed brush motors or sparking contacts near powder,
- route stepper wiring away from the weighing pan and shield/filter where needed,
- perform a formal process-safety review for the actual powder and installation.

The scale design should minimize electrostatic force because static can both create a safety hazard and produce false weight readings.

## 15. Automatic internal check mass

Use a known check mass that normally rests on a stationary support, not on the weighing mechanism.

A cam/linkage moves the mass onto a repeatable calibration landing point. During the actual measurement:

- actuator motor is stopped and preferably electrically disabled,
- linkage is mechanically disengaged or unloaded,
- check mass rests only on the intended load path,
- the scale waits for stability before accepting the check.

Use the internal mass primarily for span verification/drift checking. Periodic traceable calibration still requires certified external masses.

Recommended internal check mass: 50 g or 100 g depending packaging and mechanism balance. The exact conventional mass value should be measured/certified and stored in calibration data rather than assumed from the nominal engraving.

## 16. External calibration masses

For a 0.648 mg display increment, cheap uncalibrated masses are inadequate for final calibration.

Acquire a certified set with uncertainties well below the desired scale error. OIML E2 or appropriately certified ASTM Class 1 weights are reasonable starting points; select the final class from the desired uncertainty budget, not only nominal class name.

Useful nominal values:

- 20/25 g,
- 50 g,
- 100 g,
- 200 g,
- combinations to reach 250 g.

Store certificate conventional-mass values in the calibration procedure when available.

## 17. Characterization plan

Before claiming accuracy, collect:

- 20+ repeated placements at several masses,
- ascending and descending loading sequences,
- corner-load tests,
- 30-minute loaded creep tests,
- empty return-to-zero,
- battery vs wall operation,
- charger on vs suspended,
- motor idle vs running elsewhere in machine,
- temperature sweeps,
- enclosure-open/closed tests,
- static-control tests,
- vibration tests,
- 300-400 production-like cycles.

Log raw ADC code, coil current, temperatures, optical error/sum and all power states.

## 18. Source files in this pack

See `../01_Electrical_Design/Datasheets/` and `../05_AI_Reference/Component_Datasheet_Index.md`.

Commercial benchmark: `Sartorius_WZB254-NC_Benchmark_Datasheet.pdf`.
