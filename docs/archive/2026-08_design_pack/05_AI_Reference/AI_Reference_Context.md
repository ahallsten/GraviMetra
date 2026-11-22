# AI Reference Context - 250 g EMFC Scale and Multi-Stage Auger Controller

## Purpose

This file is a compact context handoff for an AI assistant helping with the Altium, firmware, mechanical, calibration, or bring-up work for this project. Treat the design decisions below as the current baseline unless the project owner explicitly changes them.

The detailed design rationale lives in:

- `../01_Electrical_Design/Electrical_Architecture_and_Component_Review.md`
- `../01_Electrical_Design/Power_Budget.md`
- `../01_Electrical_Design/Connector_and_IO_Plan.md`
- `../02_EMFC_Mechanics/250g_EMFC_Design_Guide.md`
- `../03_Altium/Altium_Designer_Step_by_Step_Guide.md`
- `../04_Firmware/Codex_Firmware_Implementation_Prompt.md`

## Frozen system target

- Technology: electromagnetic force compensation/restoration (EMFC/EMFR), not a strain-gauge load cell.
- Live weighing capacity: 250 g.
- Target display increment: 0.01 grain = 0.6479891 mg.
- Intended use: general precision powder dispensing with four staged auger channels available; only one auger may run at a time.
- Precision target is an engineering goal pending prototype qualification; do not equate display resolution with guaranteed accuracy.
- The fast position-restoration loop is analog in Revision A. Firmware supervises it and measures restoring current.
- Per-cycle stable empty-pan tare is allowed and recommended. This is not a substitute for span calibration.
- Both manual certified-mass calibration and automatic internal check-mass verification/calibration support are required.

## Safety context

The machine may encounter combustible or energetic powder. Electrical and mechanical advice must prioritize ignition-source separation, protective-earth bonding, electrostatic control, guarded mains wiring, hardware motion shutdown, and formal hazard review. Do not assume an ordinary benchtop environment. Keep mains and high-energy switching electronics physically segregated from the powder zone. The project documentation is engineering guidance, not certification to a hazardous-location or machinery-safety standard.

## System partition

### Assembly A - precision sensor/analog PCB

Target maximum footprint: approximately 3 x 3 inches.

Contains:

- optical null detector interface,
- OPA2388 transimpedance/servo amplifiers,
- ADS1262 precision ADC,
- LTC6655-2.5 precision reference,
- LT3045 quiet analog regulator,
- OPA593 linear voice-coil/current amplifier,
- VPG/Vishay four-terminal 10 ohm precision shunt,
- TMUX1101 integrator reset/anti-windup switch,
- TMP117 temperature sensors / interfaces,
- controller-board digital interface.

Do not place chopper motor drivers, battery charger, high-current switchers, USB connector, or mains components on this board.

### Assembly B - controller / motor / low-voltage power PCB

Target maximum footprint: approximately 5 x 5 inches.

Contains:

- STM32G491RET6 MCU,
- four BigTreeTech TMC2209 StepStick module sockets,
- Nextion UART/power connector,
- USB-C device/service port,
- TCAN1042-family CAN transceiver,
- THVD1450 RS-485 transceiver,
- 2S Li-ion battery charger / power-path subsystem,
- 12 V battery boost rail for one active stepper,
- 5 V system/HMI rail,
- 3.3 V digital rail,
- hardware E-stop interface,
- automatic check-mass actuator/sensor I/O,
- sensor-board interface.

### Assembly C - segregated mains power section

Use:

- IEC-C14 inlet,
- appropriately rated fuse,
- double-pole mains switch,
- protective-earth connection to chassis,
- Mean Well IRM-30-12 (12 V / 2.5 A) and IRM-20-5 (5 V / 4 A) as the preferred wall supplies.

The precision/controller PCBs should receive SELV DC only. If the Mean Well encapsulated modules are PCB-mounted, put them on a separate mains-qualified power board or mechanically isolated mains subassembly with correct creepage, clearance, fuse, PE bonding, and enclosure provisions.

## MCU selection

Use **STM32G491RET6**, LQFP64, 3.3 V.

Reason for selecting it over the earlier STM32G431: the expanded machine needs four dedicated TMC2209 UART channels, one Nextion UART, one RS-485 UART, USB, SPI, I2C, timers, and CAN. The STM32G491 family provides five USART/UART peripherals plus an LPUART, USB device, and two FDCAN controllers.

Important:

- FDCAN controller is inside the MCU; the CAN physical transceiver is not. Use external TCAN1042-family hardware.
- RS-485 physical transceiver is not integrated. Use THVD1450.
- Use STM32CubeMX before schematic pin assignment to prove the peripheral/pin mapping.
- Preserve SWD, NRST, BOOT0 access.
- Follow ST AN5093 and the STM32G491 datasheet for supply decoupling, VDDA/VREF, USB, reset, boot, crystal decisions, and PCB layout.

## Precision measurement chain

### Force model

At 250 g full scale:

    Fload = 0.250 kg * 9.80665 m/s^2 = 2.45166 N

Use approximately 25:1 mechanical force reduction so the electromagnetic actuator force remains near the original low-current design point.

With approximately 2 N/A actuator force constant:

    Fcoil_FS ~= 0.09807 N
    Icoil_FS ~= 49.0 mA

Target practical current range:

- empty pan after mechanical balancing: about 5-10 mA,
- full live load: about 54-60 mA,
- hardware current limit: about 80 mA.

A 0.01 grain increment corresponds approximately to:

    dm = 0.6479891 mg
    dI ~= 0.127 uA
    dV across 10 ohm shunt ~= 1.27 uV

Therefore microvolt-scale thermoelectric, reference, grounding, and layout errors matter.

### ADS1262

Use ADS1262 as the precision ADC.

Design-critical points:

- 32-bit delta-sigma architecture; usable effective/noise-free resolution is less than nominal 32 bits.
- AVDD nominal 5 V; DVDD nominal 3.3 V in this design.
- SPI to STM32; DRDY to interrupt-capable GPIO.
- Primary measurement is differential voltage across the 10 ohm shunt using true Kelvin connections.
- Initial input filter: 68 ohm in each leg and approximately 100 nF differential capacitor, with optional tuning footprints.
- Support fast profile during dispensing and low-noise/line-rejection profile for final settled readings.
- Follow the datasheet exactly for AVDD/DVDD/reference decoupling and any reference-input capacitor requirements.

### Precision shunt

Preferred class: VPG Foil Resistors VCS1625ZP / Y1606, 10 ohm four-terminal Z-foil or equivalent.

Design-critical characteristics from the VPG family documentation:

- four-terminal Kelvin connection,
- resistance range includes 10 ohm,
- about 1 W rating at +70 C with proper footprint,
- extremely low TCR compared with ordinary current-sense resistors,
- power coefficient and long-term stability are more important here than absolute initial tolerance because the system is calibrated.

At 60 mA with 10 ohm:

    P = I^2 R = 36 mW

Use the manufacturer-recommended four-pad footprint and generous copper. Route the two current pads separately from the two voltage-sense pads. Do not share the sense traces with load current.

### LTC6655-2.5 reference

Use as external precision reference.

Key layout/design points:

- put it in the thermally quiet ADC/reference area,
- minimum local input bypass per datasheet,
- use the specified low-ESR output capacitance range for the exact chosen variant,
- avoid rapidly changing loads on the reference,
- keep away from OPA593, shunt, MCU, regulators, and charging heat.

### LT3045 analog regulator

Use to create quiet analog power after a noisier upstream rail.

- Follow the datasheet SET pin, SET capacitor, input/output capacitor, current-limit, and stability guidance exactly.
- Check dropout and dissipation at worst-case input voltage.
- Do not use it to power HMI, motors, USB loads, or other high-current digital loads.

### OPA593 voice-coil driver

Use as the linear actuator/current amplifier.

- Supply rail: nominal about 9-12 V, subject to final actuator drop and thermal analysis.
- Normal current is much lower than the device's maximum; target <= 60-65 mA.
- Set a hardware current limit near 80 mA using the datasheet equation for the selected package/revision.
- Route thermal-warning/current-limit status to MCU.
- Provide thermal copper/vias per package recommendations.
- Keep its heat away from the ADC/reference/shunt/mechanism.
- Provide footprints for output isolation/snubber compensation because final values depend on measured voice-coil R, L, and cable capacitance.

### OPA2388 optical front end and servo

Use zero-drift precision amplifiers for two photodiode TIAs and low-frequency servo conditioning.

Initial TIA prototype values:

- feedback resistor: 33.2 kohm,
- feedback capacitor: 100 pF C0G,
- add selectable/alternate values for bench tuning.

Optical detector:

- TSAL6200 940 nm emitter, continuous DC drive,
- Hamamatsu S3096-02 split photodiode,
- measure both difference and sum.

Difference is position error; sum is an optical-health diagnostic.

### TMUX1101

Use to reset/clamp the analog integrator during startup, fault, overload, transport, or service operations. Check signal range relative to its supply and do not violate analog input limits.

### TMP117

Use at least three temperature measurements:

1. magnet/yoke,
2. flexure/mechanical frame,
3. precision AFE/shunt/reference region.

Compensation should be modular and based on calibration data; do not bury temperature coefficients in unrelated code.

## EMFC mechanical guidance

Use a monolithic parallel-flexure / compliant guidance mechanism, not sliding bearings.

The often-mentioned tripod/three-point concept applies primarily to the **support of the complete scale chassis**: three feet define a plane, simplify leveling, and prevent a rigid base from rocking on a non-flat surface.

It does **not** replace the internal EMFC guidance mechanism.

Inside the EMFC cell, flexures constrain unwanted translation and rotation while permitting the tiny intended displacement without stiction or backlash. Consider a two-stage compliant lever if a single approximately 25:1 unequal-arm lever is physically awkward.

Prototype material: 7075-T651 aluminum. Consider a later 17-4PH comparison after the first mechanism is characterized.

Use FEA before finalizing flexure dimensions. Analyze:

- rated 250 g stress,
- overload-stop stress,
- lateral/torsional stiffness,
- parasitic rotation,
- first several modes,
- fatigue margin,
- thermal symmetry,
- off-center loading.

Prefer SmCo magnets for improved thermal stability relative to common NdFeB grades. Characterize the actual assembled force constant versus temperature.

Provide:

- hard overload stops,
- transport lock,
- three-point leveling support for the complete sensor assembly,
- draft enclosure,
- grounded/static-dissipative pan and powder hardware where compatible,
- thermal and vibration isolation.

## Auger subsystem

Provision four BTT TMC2209 StepStick-style modules.

Only one auger may run at a time.

Typical operating sequence can be coarse -> medium -> fine -> very fine, with progressively smaller augers or lower feed rates. Firmware must enforce the one-motor-at-a-time interlock independently of UI commands.

Each driver gets dedicated:

- STEP,
- DIR,
- EN,
- hardware UART,
- 12 V VM,
- 3.3 V logic,
- motor connector,
- local bulk capacitance.

Do not hot-plug motors or TMC modules. Route phase pairs together and away from USB and the sensor-board cable.

## Nextion HMI

Use NX8048T050 via UART only.

- Power from 5 V rail.
- Budget up to 1 A for the HMI branch.
- Connector: +5 V, GND, MCU_TX, MCU_RX.
- A short cable of a few inches is expected.
- Use optional source-series resistors and external-connector ESD footprints.
- HMI is not part of the safety chain; a frozen display must not defeat E-stop.

## USB-C

USB is service/data only, not primary machine power.

Recommended implementation:

- GCT USB4105 or equivalent USB2 receptacle,
- CC1 -> 5.1 kohm to GND,
- CC2 -> 5.1 kohm to GND,
- TPD2EUSB30 close to D+/D- connector pins,
- 90 ohm differential routing based on the actual fab stackup,
- continuous reference plane,
- no stubs/plane breaks,
- VBUS handled only as required for USB attach/sense and protected accordingly.

## CAN and RS-485

CAN:

- STM32 FDCAN controller + external TCAN1042-family physical transceiver,
- screw terminal CANH/CANL/GND,
- selectable 120 ohm termination,
- TVS/common-mode-choke footprints near connector.

RS-485:

- THVD1450 half-duplex transceiver,
- screw terminal A/B/GND,
- selectable 120 ohm termination,
- optional bias/failsafe resistor footprints,
- dedicate one MCU UART.

Final application protocol is intentionally TODO. Keep scale/feeder application logic transport-independent.

## Battery and wall power

### Wall

Preferred existing supplies:

- IRM-30-12: 12 V / 2.5 A / 30 W,
- IRM-20-5: 5 V / 4 A / 20 W.

They are fed from the IEC-C14/fused/double-pole-switched mains section. Protective earth bonds the metal enclosure and suitable conductive structures.

### Battery

Finished machine recommendation: protected, matched **2S Li-ion pack**.

Baseline energy target:

- practical minimum for shorter duty: roughly 40-50 Wh,
- recommended baseline: roughly 70-75 Wh,
- 90-100 Wh acceptable if size/weight permits and the pack/BMS can support required current.

A 2S 10 Ah pack is approximately 74 Wh nominal.

Harvested laptop cells should be treated as prototype material only unless they are capacity/IR matched, assembled with proper cell-level construction, and protected by a suitable 2S BMS. Do not solder directly to unknown cells unless they were designed with tabs; use proper spot-welded interconnects and cell protection practices.

An external DeWalt/Milwaukee tool battery may be supported later as an **external DC source only** after the exact battery family and voltage are selected. Do not attempt to charge a tool pack with the machine.

### Charger/power path

Use MAX77960B-class 2S buck-boost charger/power path.

- Follow Analog Devices' exact inductor, switching-capacitor, thermistor, current-sense, input/output capacitor, and layout requirements.
- Set conservative charge current based on wall-supply budget and pack specification.
- Firmware should be able to reduce/suspend charging during the final precision settling interval if measurements show charger interference.

### Battery 12 V motor rail

Use TPS61088-class high-current boost for 2S -> 12 V motor rail. The earlier TPS63070 suggestion is superseded because it is not sized appropriately for a NEMA17 stepper rail.

Only one motor is allowed to run at once.

### 5 V and 3.3 V rails

- TPS62135-class 5 V buck from 2S battery/system bus for HMI/system load.
- TPS62172-class 3.3 V regulator for digital logic where its current budget is sufficient; verify actual final load before release.
- Use TPS2121-class source muxes as appropriate so wall-derived and battery-derived rails transfer cleanly.

## E-stop and hard power controls

Rear master power:

- IEC-C14,
- fuse,
- double-pole mains switch.

E-stop:

- use a normally-closed hardwired loop,
- remove/disable all auger driver motion power or enable independent of firmware,
- remove/disable future robot motion enable and automatic calibration actuator,
- keep MCU, Nextion, communications, and scale measurement alive so the fault is visible/logged.

Firmware should monitor the E-stop state but must not be the only means of stopping motion.

A formal risk assessment may require a safety relay/contactor rather than ordinary PCB logic for the final machine.

## Internal check-mass calibration mechanism

Reserve electrical I/O for a small actuator and at least home/end sensing.

Mechanically:

- a known reference mass rests on a fixed support when not in use,
- a cam/linkage gently transfers it to a defined calibration point,
- actuator force must be mechanically absent from the weighing mechanism during the reading,
- verify stable load before accepting the check.

A 50 g or 100 g check mass is a reasonable starting region, but store the actual calibrated mass value from its certificate. Manual multi-point calibration remains the traceability reference.

For final calibration work, obtain certified masses with uncertainty comfortably below the desired scale-error budget. An unverified hobby 50 g weight is insufficient to establish sub-milligram performance.

## PCB fabrication / assembly philosophy

Use JLCPCB or PCBWay for SMT assembly where economical. Hand-install:

- BTT TMC2209 modules and sockets,
- pluggable screw terminals,
- large external connectors,
- Mean Well modules / mains wiring assemblies,
- expensive or unavailable specialty precision parts when CBA/consigned assembly is not economical.

Use 0603 passives by default where precision/parasitics do not require another size. Use manufacturer-required packages for ICs. Check MSL/reflow requirements in current datasheets and distributor packaging before assembly; do not bake moisture-sensitive packages based on guesswork.

## Bring-up sequence

Bring subsystems up separately:

1. controller board with no TMC modules and no sensor board,
2. verify 3.3 V / 5 V rails, reset, SWD, USB,
3. verify Nextion UART,
4. insert one TMC module and test at current-limited bench supply,
5. verify four TMC channels one at a time,
6. verify CAN and RS-485 electrically,
7. sensor board from current-limited bench supplies without coil,
8. verify reference and ADC noise with shunt input shorted appropriately,
9. verify optical front end with a mechanical target,
10. verify OPA593/current loop into a dummy load,
11. connect real voice coil and tune analog loop at low current,
12. characterize mechanical plant and settle loop compensation,
13. calibrate mass/current conversion,
14. characterize temperature coefficients,
15. integrate auger motor activity and verify no unacceptable measurement corruption,
16. only then integrate complete powder-handling mechanism.

## Never assume these items

- Final flexure dimensions: must come from FEA and prototype tests.
- Final analog PI values: tune to measured plant.
- Final coil turns/resistance/inductance: verify manufactured coil.
- Final battery pack/BMS: choose an exact certified/protected pack before freezing charger current and connector.
- Final CAN/RS-485 protocol: TODO.
- Tool-battery input voltage/interface: TODO until exact battery family is selected.
- Hazardous-location compliance: requires separate formal assessment/certification.
- Display resolution equals accuracy: false; characterize repeatability, linearity, drift, stability, and uncertainty separately.
