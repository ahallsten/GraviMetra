# Altium Designer Step-by-Step Guide - EMFC Scale Controller and Sensor Boards

## Audience and intent

This guide assumes you have previously completed a few Altium boards but have been away from Altium for about two years. It therefore spells out the workflow and the design intent rather than assuming you remember where every command lives. Menu names vary somewhat between Altium versions; if a command moved, use Altium's search box rather than changing the design method.

The project consists of three physical electrical assemblies:

1. **Precision sensor/analog PCB** - target <=3 x 3 in.
2. **Controller/motor/low-voltage power PCB** - target <=5 x 5 in.
3. **Segregated mains PSU assembly** - IEC-C14, fuse, double-pole switch, protective earth and Mean Well IRM modules. Do not combine exposed mains routing with the precision sensor PCB.

Read `../01_Electrical_Design/Electrical_Architecture_and_Component_Review.md` before starting.

---

# Phase 0 - Do not start Altium until these project decisions are recorded

Create a plain-text `DESIGN_DECISIONS.md` beside the Altium project and record:

- 250 g capacity.
- 0.01 grain target display increment.
- STM32G491RET6.
- four BTT TMC2209 sockets, one motor active at a time.
- Nextion NX8048T050 on dedicated UART.
- USB-C device.
- TCAN1042-class CAN transceiver.
- THVD1450 RS-485 transceiver.
- protected 2S Li-ion pack, baseline ~74 Wh.
- Mean Well IRM-30-12 and IRM-20-5 for wall operation.
- hardware E-stop removes motion capability while logic/scale remain powered.
- sensor and controller boards separated by 6-18 in shielded cable.
- automatic internal check-mass interface.
- manual certified calibration mode.

This prevents schematic drift later.

---

# Phase 1 - Set up the Altium project

## Step 1 - Create a project folder

Recommended structure:

    EMFC_Scale/
      Altium/
        EMFC_Scale.PrjPcb
        Controller_Power.PcbDoc
        Sensor_AFE.PcbDoc
        Sch/
        Libraries/
        OutJobs/
      Mechanical/
      Datasheets/
      Firmware/
      Docs/

Copy the supplied datasheet folder into or link it from `Datasheets/`.

## Step 2 - Create a PCB project

In Altium:

- File -> New -> Project -> PCB Project.
- Save as `EMFC_Scale.PrjPcb`.
- Immediately save all files with meaningful names.

## Step 3 - Create hierarchical schematic sheets

Do not draw the whole project on one sheet.

Recommended controller sheets:

- `CTRL_01_MCU.SchDoc`
- `CTRL_02_USB.SchDoc`
- `CTRL_03_HMI.SchDoc`
- `CTRL_04_TMC2209_X4.SchDoc`
- `CTRL_05_CAN_RS485.SchDoc`
- `CTRL_06_POWER_INPUT_MUX.SchDoc`
- `CTRL_07_BATTERY_CHARGER.SchDoc`
- `CTRL_08_DC_DC_RAILS.SchDoc`
- `CTRL_09_ESTOP_SAFETY_IO.SchDoc`
- `CTRL_10_CAL_ACTUATOR_IO.SchDoc`
- `CTRL_11_SENSOR_BOARD_INTERFACE.SchDoc`

Recommended sensor sheets:

- `SNS_01_ADS1262.SchDoc`
- `SNS_02_REFERENCE_POWER.SchDoc`
- `SNS_03_PHOTODIODE_TIA.SchDoc`
- `SNS_04_ANALOG_SERVO.SchDoc`
- `SNS_05_OPA593_COIL.SchDoc`
- `SNS_06_TEMPERATURE_DIAGNOSTICS.SchDoc`
- `SNS_07_CONTROLLER_INTERFACE.SchDoc`

Use a top-level hierarchical sheet for each PCB if desired. Keep the two PCB netlists logically separable.

---

# Phase 2 - Libraries and component data

## Step 4 - Decide library strategy

For this project, use **project-specific source libraries** for critical/custom parts rather than relying entirely on installed compiled libraries.

Create:

- `EMFC_Scale.SchLib`
- `EMFC_Scale.PcbLib`

For generic R/C parts you may use trusted Altium/manufacturer libraries, but critical ICs and connectors should have locally verified symbols/footprints.

## Step 5 - For every component, enter parameters

At minimum:

- Manufacturer
- Manufacturer Part Number
- Supplier 1
- Supplier Part Number
- Description
- Value
- Package
- Datasheet URL
- Datasheet local filename
- DNP/assembly note if relevant

Do not leave manufacturer part numbers as free-form notes on the schematic.

## Step 6 - Footprint verification procedure

For every non-generic footprint:

1. Open manufacturer mechanical drawing.
2. Verify pad pitch.
3. Verify exposed pad dimensions.
4. Verify pin 1 orientation.
5. Verify courtyard/placement clearance.
6. Verify paste-window recommendation for exposed pads.
7. Verify board-edge requirements for USB-C/terminal connectors.
8. View the footprint in 3D.
9. Print 1:1 if the connector/module is mechanically critical.

Never trust an imported SnapEDA/Ultra Librarian footprint without checking it against the current manufacturer drawing.

## Step 7 - BTT TMC2209 socket footprint

Build the footprint around the actual BTT module pin spacing and orientation.

Important:

- Verify which edge is EN/MS/UART/STEP/DIR and which edge is VM/GND/motor/VIO.
- Mark module orientation clearly on silkscreen.
- Leave heatsink clearance above the module.
- Do not place tall parts under the module.
- Provide test points for VM, VIO, UART and VREF.

Order inexpensive female headers separately and hand-solder them after SMT assembly.

---

# Phase 3 - MCU schematic

## Step 8 - Place STM32G491RET6

Use LQFP64.

Before wiring pins, open STM32CubeMX and create a matching MCU project. Assign peripherals there first so Altium and firmware share one authoritative pin map.

Required peripherals:

- UART/USART x4 for TMC2209s.
- UART for Nextion.
- UART for RS-485.
- FDCAN for CAN.
- USB FS device.
- SPI for ADS1262.
- I2C for TMP117/power management.
- timer channels for four independent STEP outputs.
- GPIO DIR/EN lines.
- SWD.
- ADC inputs for power/status diagnostics as required.

Export or manually copy this pin map into a project table before routing.

## Step 9 - MCU power pins

Follow ST's datasheet and AN5093.

For every VDD pin:

- 100 nF X7R ceramic immediately adjacent to pin/pair.

Add:

- 4.7-10 uF ceramic bulk near MCU.
- VDDA decoupling as specified by ST, typically 100 nF + approximately 1 uF close to VDDA/VSSA.
- ferrite-bead footprint between digital 3V3 and VDDA if used.
- VREF+ decoupling per ST requirements.
- VBAT tied appropriately if no backup battery is used; follow datasheet recommendation.

Do not place all 100 nF capacitors in a row far away from the MCU. Their loop inductance matters.

## Step 10 - Reset/boot/debug

Add:

- NRST pull-up if recommended for the exact design.
- reset pushbutton optional but helpful.
- BOOT0 pull-down and accessible test pad/jumper.
- SWD 5- or 10-pin programming header.
- 3.3 V and GND reference on SWD header.

Place test pads for NRST and BOOT0.

---

# Phase 4 - USB-C schematic

## Step 11 - Place USB4105 or equivalent

Use USB2-only pins.

Connect:

- A6/B6 together -> D+.
- A7/B7 together -> D-.
- CC1 -> 5.1 kohm to GND.
- CC2 -> 5.1 kohm to GND.
- receptacle shell -> chassis/ground strategy through defined connection, not an accidental long trace.

USB is service/data only; do not attempt to power the machine from VBUS.

## Step 12 - Add ESD protection

Place TPD2EUSB30 between connector and MCU.

Layout order must be:

    connector -> ESD device -> MCU

not connector -> long trace -> MCU -> branch to ESD.

Put ESD ground via(s) immediately beside the ESD part.

## Step 13 - USB routing rule

Create a differential-pair net class for USB D+/D-.

Set the impedance from the actual JLCPCB/PCBWay stackup. Do not blindly use a trace width from the internet.

Requirements:

- 90 ohm differential nominal.
- matched within sensible USB FS tolerance.
- continuous reference plane.
- avoid vias if possible.
- no stubs.
- no plane split underneath.

---

# Phase 5 - Nextion interface

## Step 14 - HMI connector

Use a keyed 4-pin connector:

1. +5V_HMI
2. GND
3. MCU_TX -> Nextion RX
4. MCU_RX <- Nextion TX

Add:

- local 100 uF + 1 uF + 100 nF near connector.
- optional 22-100 ohm source series resistor footprints on TX/RX.
- optional ESD protection if cable leaves the electronics enclosure.

Budget the 5 V rail for 1 A HMI capability.

---

# Phase 6 - Four TMC2209 module sockets

## Step 15 - Repeatable channel sheet

Use one schematic channel repeated four times if you are comfortable with Altium multi-channel design. If not, copy a carefully reviewed channel four times and label every net explicitly.

Each channel needs:

- VM = switched 12V_MOTION.
- GND.
- VIO = 3.3 V.
- STEP.
- DIR.
- EN.
- dedicated UART.
- motor A1/A2/B1/B2 connector.
- bulk electrolytic/polymer capacitor adjacent to module socket.
- 100 nF/1 uF local ceramic support as appropriate.
- DIAG test point if exposed/useful.

## Step 16 - Local motor bulk capacitance

Provide approximately 100-220 uF low-ESR bulk per driver socket as a starting design, voltage rating >=25 V for a 12 V system. Verify BTT/Trinamic guidance and prototype transient measurements.

Do not rely on a single capacitor across the entire board.

## Step 17 - Motor routing

- Route phase A pair together.
- Route phase B pair together.
- Use wide copper appropriate for approximately 1-2 A phase currents.
- Keep motor loops compact.
- Keep all four motor channel regions away from USB and sensor-board connector.
- Do not route motor traces beneath the MCU crystal/USB/analog-sensitive sections.

---

# Phase 7 - CAN and RS-485

## Step 18 - CAN

Place TCAN1042-class transceiver near CAN connector.

Add:

- supply bypass directly at transceiver.
- CANH/CANL TVS footprint.
- optional common-mode choke footprint.
- selectable 120 ohm termination jumper.
- screw terminal: CANH, CANL, GND; optional shield/chassis terminal.

Do not assume MCU FDCAN means a transceiver is integrated; it is not.

## Step 19 - RS-485

Place THVD1450 near connector.

Add:

- local bypass.
- DE/RE control from MCU.
- A/B screw terminal.
- selectable 120 ohm termination.
- optional external bias resistor footprints.
- surge/ESD protection footprint.

---

# Phase 8 - Battery charger and source power

## Step 20 - Draw the power block diagram first

Before placing any switching regulator, draw this in the schematic notes:

    WALL 12V (IRM-30-12) ---> charger / wall motor source
                               |
                               v
                         MAX77960B
                               |
                          protected 2S pack

    battery/system ---> TPS61088 ---> 12V_MOTOR_BAT
    wall 12V -----------------------> 12V_MOTOR_WALL
                    source mux ----> 12V_MOTION ---> E-stop power cut ---> TMC sockets

    battery/system ---> TPS62135 ---> 5V_BAT
    wall IRM-20-5 -----------------> 5V_WALL
                    source mux ----> 5V_SYSTEM ---> Nextion, downstream logic

    5V/system ---> TPS62172 ---> 3V3_DIGITAL

    suitable system source ---> LT3045 ---> 5V_ANALOG

Review for every possible back-feed path before schematic release.

## Step 21 - MAX77960B

Do not freestyle this charger circuit.

Open `MAX77960B_Datasheet.pdf` and copy the appropriate 2S typical-application topology while recalculating values for:

- 12 V input.
- 2S battery.
- approximately 1 A starting charge current.
- thermistor input.
- desired switching frequency.
- required inductor rating.
- input/output capacitance.
- I2C/address/status configuration.

Place charger, inductor, switching capacitors and power FET current loops together exactly as recommended.

Give the charger a temperature-sensor/NTC connection to the battery pack.

## Step 22 - TPS61088 12 V battery boost

Open `TPS61088_Datasheet.pdf`.

Design for:

- VIN approximately 6.0-8.4 V depending pack state/protection cutoff.
- VOUT approximately 12.0 V.
- enough output for one active NEMA17/TMC2209 plus margin.

Use TI's equations/tool to select:

- feedback divider.
- inductor and saturation-current rating.
- current-limit setting.
- input and output capacitors.
- compensation components.

Copy the datasheet layout topology closely. The switching hot loop must be small.

## Step 23 - TPS62135 5 V battery rail

Design for 2S input -> 5.0 V.

Budget >=1 A for Nextion plus controller overhead; the 4 A regulator provides healthy margin.

Use forced PWM during precision operation if lab tests show PFM tones/noise coupling.

## Step 24 - TPS62172 3.3 V

Use fixed 3.3 V variant.

Place local input/output caps next to the IC and keep its switching node compact.

## Step 25 - LT3045 clean analog 5 V

Follow the datasheet exactly for:

- SET resistor.
- SET capacitor/noise reduction.
- input capacitor.
- output capacitor.
- current-limit connection.
- PG/enable if used.

Keep the LT3045 and its pass current physically separated from the LTC6655 reference and ADC enough to avoid local thermal gradients.

---

# Phase 9 - E-stop and safety I/O

## Step 26 - Hardware motion cut

The E-stop must not merely enter an MCU pin.

Use the NC E-stop loop to command a physical motion-power interruption device or certified safety element appropriate to the final risk assessment.

The circuit should cause 12V_MOTION or driver enable to fall to a safe disabled state when:

- E-stop is pressed,
- wiring opens,
- control power for the E-stop relay disappears.

Also route a dry auxiliary/contact sense to the MCU so firmware knows E-stop state.

Do not switch protective earth.

## Step 27 - Mains

Prefer a separate mains assembly.

IEC C14 -> fuse -> double-pole switch -> IRM modules.

Bond protective earth to chassis immediately at the inlet area.

If you build a mains PCB, create explicit high-voltage clearance rules and isolation cutouts. Do not route SELV copper under mains-side parts unless allowed by the module and safety design.

---

# Phase 10 - Sensor/AFE board

## Step 28 - ADS1262 placement

Place the ADC close to:

- precision shunt Kelvin sense entry.
- LTC6655 reference.

But keep it away from:

- OPA593 thermal region.
- coil connector current path.
- any switching regulator.

Use a continuous analog ground/reference plane.

## Step 29 - Shunt layout

The 10-ohm precision shunt must have four-terminal routing.

Two heavy/current terminals carry coil current.

Two separate sense traces leave the dedicated Kelvin sense points and go directly to the ADC input RC filter.

Never share a section of current-carrying trace with the sense path.

Avoid connector thermocouple junctions in the microvolt sense path where possible.

## Step 30 - LTC6655

Place in a quiet isothermal region.

Use required input/output capacitance from datasheet.

Do not put copper pours connected to warm power nodes beneath it.

Keep digital clocks away.

## Step 31 - Optical TIAs

Place OPA2388 extremely close to photodiode connector/device.

The photodiode/TIA summing nodes are high impedance:

- keep traces short.
- keep them clean.
- avoid solder-mask contamination assumptions; clean assembled boards.
- use guard copper only if you understand the guard potential and return path.
- no digital traces underneath.

## Step 32 - Analog PI servo

Make PI components selectable:

- resistor footprints in series/parallel options.
- C0G/film capacitor options where value permits.
- loop-break/injection resistor or jumper.
- TMUX1101 integrator reset.

Label test points clearly.

## Step 33 - OPA593 and coil

Place OPA593 near coil connector and shunt current path, away from reference/ADC.

Use wide power/current traces and dedicated return.

Provide thermal copper/vias according to package guidance.

Bring current-limit/thermal-warning signals back to controller.

## Step 34 - Temperature sensors

Place three TMP117 sensors physically to measure:

- flexure structure via remote/small board if necessary.
- magnet/yoke.
- AFE/reference/shunt board region.

Do not put the AFE sensor directly beside OPA593 and then call it the ADC temperature.

---

# Phase 11 - Schematic validation

## Step 35 - Compile and fix ERC intentionally

Project -> Validate/Compile.

Resolve:

- floating power pins.
- multiple output conflicts.
- missing drivers.
- unconnected required pins.
- inconsistent net names.

Use No ERC markers only after you understand why the warning is acceptable.

## Step 36 - Datasheet checklist review

For every IC, make a checklist row:

- every power pin connected?
- every ground/exposed pad connected?
- all required bypass capacitors?
- enable pins defined at boot?
- bootstrap/charge-pump parts if applicable?
- feedback network correct?
- programming/address pins defined?
- thermal pad/vias correct?
- unused inputs not floating?
- absolute max respected during source transitions?

Do not start PCB placement until this review is complete.

---

# Phase 12 - PCB stackup and rules

## Step 37 - Get manufacturer stackup before routing

Choose a standard JLCPCB/PCBWay 4-layer stackup first. For the sensor board, 6 layers are worth considering if the price remains acceptable.

Controller suggested 4-layer concept:

- L1 components/signals/power routing.
- L2 uninterrupted ground.
- L3 power distribution/slow signals.
- L4 signals/components.

Sensor suggested 4-layer concept:

- L1 precision components/signals.
- L2 continuous ground.
- L3 quiet power/slow control.
- L4 low-speed digital/secondary routing.

## Step 38 - Create net classes

At minimum:

- USB_DIFF.
- MOTOR_POWER.
- MOTOR_PHASE.
- ANALOG_PRECISION.
- SHUNT_KELVIN.
- 12V_POWER.
- 5V_POWER.
- 3V3_DIGITAL.
- CAN_DIFF.
- RS485_DIFF.

## Step 39 - Clearance and width rules

Calculate trace widths from actual copper weight, acceptable temperature rise and current.

Do not use one global width for everything.

USB differential geometry must come from stackup field solver/impedance calculator.

---

# Phase 13 - Placement order

## Step 40 - Place connectors and mechanical constraints first

Lock:

- board outline.
- mounting holes.
- USB-C at edge.
- terminal blocks.
- TMC sockets and heatsink envelope.
- sensor cable connector.
- battery connector.
- Nextion connector.

Use mechanical layers for keepouts/enclosure clearances.

## Step 41 - Place by current loop / signal chain

Controller placement order:

1. power-input/source-selection.
2. charger and power converters.
3. four TMC2209 sections near motor connectors.
4. MCU central quiet area.
5. USB at board edge near MCU.
6. CAN/RS-485 at their connectors.
7. HMI connector.
8. sensor-board digital connector away from motor switching.

Sensor placement order:

1. photodiode/TIA interface.
2. reference/ADC/shunt sense chain.
3. servo amplifier.
4. OPA593/coil current path.
5. temperature sensors.
6. controller connector.

---

# Phase 14 - Routing order

## Step 42 - Route critical analog first

Sensor board priority:

1. shunt Kelvin pair.
2. reference paths.
3. photodiode summing nodes.
4. ADC inputs.
5. analog servo.
6. coil current path.
7. SPI/I2C/control last.

## Step 43 - Route USB

Follow differential rules and continuous L2 plane.

## Step 44 - Route switching regulators

The regulator's datasheet layout example outranks aesthetic routing.

Keep:

- input cap-switch-ground loop minimal.
- switch node copper small.
- feedback trace away from switch node.
- power-ground thermal vias as specified.

## Step 45 - Route motors

Use short, wide, paired phase routing. Keep phase copper out of the MCU/USB/sensor-interface region.

## Step 46 - Ground strategy

Do not create arbitrary split grounds under mixed signals.

Use a continuous ground plane, but control where currents enter it by placement.

The following return currents must not share narrow copper:

- motor driver switching returns.
- charger/regulator switch currents.
- OPA593/coil current.
- ADC/reference/photodiode returns.

Think in current loops rather than net names.

---

# Phase 15 - Testability

## Step 47 - Add test points before DRC

Controller:

- AC-derived 12V DC.
- 12V_MOTION before and after E-stop.
- 5V_SYSTEM.
- 3V3.
- battery/system bus.
- charger status.
- each TMC UART/STEP/EN.
- USB D+/D- small probing pads if appropriate.
- CAN TX/RX and CANH/L.
- RS-485 TX/RX/DE and A/B.

Sensor:

- 5V_ANALOG.
- reference 2.5 V.
- photodiode left/right.
- optical difference/sum.
- PI output.
- OPA593 output.
- shunt current terminals.
- Kelvin sense points.
- ADS1262 DRDY/SPI.

## Step 48 - Add current-measurement jumpers

Where practical, add removable zero-ohm links or solder jumpers that allow you to measure subsystem current during bring-up.

---

# Phase 16 - DRC, review and fabrication

## Step 49 - Run full DRC

Fix every rule violation or document it intentionally.

## Step 50 - 3D review

Inspect:

- TMC module orientation.
- USB shell interference.
- terminal block access.
- mounting-hole clearance.
- Nextion connector direction.
- battery connector keying.
- sensor-board cable clearance.
- heatsink clearance.

## Step 51 - Schematic-to-PCB cross-probe review

Manually inspect every power net and every connector pin.

For each connector, verify pin numbering from the mating side and PCB side. Connector mirroring errors are common and expensive.

## Step 52 - Manufacturing outputs

Create an OutJob containing:

- Gerber X2 or ODB++ as accepted by assembler.
- NC drill.
- fabrication drawing.
- assembly drawing top/bottom.
- pick-and-place centroid files.
- BOM with manufacturer and supplier numbers.
- solder paste layers.
- schematic PDF for internal use.
- IPC-356 netlist if useful.

## Step 53 - Assembly split

Ask assembler to place mainstream SMT parts.

Mark as DNP/hand assembly:

- TMC2209 modules and socket headers if desired.
- Mean Well modules/mains wiring.
- large pluggable terminals if assembly cost is high.
- expensive precision parts if distributor sourcing is cheaper.
- through-hole optical/mechanical parts.

Before ordering, compare total extended cost with and without JLC/PCBWay sourcing for the LTC6655, precision shunt and specialized analog parts.

---

# Phase 17 - First-power bring-up order

Do not populate/connect everything and turn on mains for the first test.

## Controller board

1. Visual/microscope inspection.
2. Resistance check from each rail to ground.
3. Power from current-limited bench supply, not mains.
4. Validate 5 V and 3.3 V rails.
5. Program STM32 over SWD.
6. Verify clocks/reset/watchdog.
7. Bring up USB CDC.
8. Bring up Nextion UART.
9. Bring up CAN and RS-485 loopback/test nodes.
10. Insert one TMC2209 only.
11. Verify VM/VIO/orientation.
12. Test one low-current motor.
13. Repeat for four channels.
14. Validate hardware E-stop cuts motion.
15. Bring up battery converter without battery first if possible using a bench source simulating 2S voltage.
16. Bring up charger with an approved protected 2S pack and conservative current.
17. Only then integrate Mean Well wall supplies/mains enclosure.

## Sensor board

1. Inspect and clean board thoroughly.
2. Resistance checks.
3. Current-limited bench supply.
4. Verify clean 5 V analog rail.
5. Verify 2.5 V reference and allow warm-up.
6. Verify TMP117 sensors.
7. Bring up ADS1262 with shorted/dummy differential input.
8. Measure ADC noise floor before connecting actuator.
9. Verify optical LED current.
10. Verify left/right photodiode signals.
11. Verify null error changes polarity with vane motion.
12. Test analog servo using a dummy load/limited coil current.
13. Connect actual voice coil with low current limit.
14. Tune loop at very low mechanical authority.
15. Increase to normal operating range only after stable response.

---

# Phase 18 - Design-review gates

Do not order Rev A until these are answered yes:

- All critical footprints verified against manufacturer drawings?
- CubeMX pin map frozen and copied to schematic?
- Six required UARTs mapped without conflict?
- USB pins and clock requirements satisfied?
- CAN transceiver and termination included?
- RS-485 transceiver and termination included?
- Four TMC sockets correctly oriented?
- Only one-motor-at-a-time power assumption documented?
- E-stop independent of firmware?
- Charger is configured for 2S, not 1S?
- Battery connector cannot be reversed?
- No battery source can back-feed the Mean Well supply?
- Precision shunt truly Kelvin-routed?
- Reference/output capacitor requirements satisfied?
- Switching regulator layouts follow datasheet examples?
- Sensor cable carries digital ADC data instead of microvolt shunt signal?
- Mains physically segregated from powder zone and SELV electronics?
- Protective earth bonding point defined?
- Automatic calibration actuator cannot load the scale during stable measurement?
- Adequate test points exist?
- BOM expected prototype cost remains under the $300 target?

---

# Datasheet-first rule

Whenever this guide and a manufacturer's current datasheet disagree, stop and resolve the discrepancy. The current manufacturer datasheet and errata take precedence. Record the resolution in `DESIGN_DECISIONS.md` so the same question is not revisited later.
