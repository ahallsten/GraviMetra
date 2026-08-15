# Electrical Architecture and Component Review

## Frozen system assumptions

- 250 g EMFC weighing capacity.
- Target display increment: 0.01 grain = 0.647989 mg.
- Four auger channels are provisioned, but only one NEMA17 motor is intended to run at a time.
- BigTreeTech TMC2209 StepStick-style modules are used rather than bare TMC2209 ICs.
- Nextion NX8048T050 HMI is connected by 5 V power and TTL UART.
- Main controller MCU: STM32G491RET6.
- USB-C is provided for firmware updates, configuration and logging.
- CAN and RS-485 are physically implemented with external transceivers.
- Precision sensor/analog electronics are on a separate PCB from controller, motor and switching-power electronics.
- The scale PCB should fit within approximately 3 x 3 inches. The controller/low-voltage power PCB should fit within approximately 5 x 5 inches.
- 120 VAC is kept out of the precision and controller PCBs. Mean Well AC/DC modules are mounted in a segregated mains compartment or on a separate mains power assembly.
- IEC-C14 inlet, fuse, double-pole mains switch and protective-earth bonding are required.
- A hardwired normally-closed E-stop removes motion power/enable independently of firmware while keeping the controller/HMI/scale alive for diagnostics.
- Internal automatic check-mass calibration and manual certified-mass calibration are both supported.

## Important safety scope

The machine may encounter combustible or energetic powders. The electronics in this design pack are therefore organized to reduce ignition sources, static accumulation and fault energy. This pack does not replace a formal hazardous-material/process risk assessment, applicable electrical code, explosive-dust classification, or machinery-safety review. Keep mains wiring, switching power, relay contacts and motor drivers in a physically segregated enclosure away from the powder path. Bond conductive powder-contact structures to protective earth and design the powder-handling system to avoid electrostatic charging.

## Component review conclusions

### MCU - change from STM32G431CBT6 to STM32G491RET6

The STM32G431 was a good control MCU for the earlier scale-only concept, but the expanded system needs four TMC2209 UARTs, one Nextion UART and one RS-485 UART, in addition to SPI, I2C, USB and CAN. The STM32G491 family provides five USART/UART peripherals plus one LPUART, USB device and two FDCAN controllers. This makes the STM32G491RET6 a substantially better architectural fit without requiring software UARTs or UART multiplexers.

Use the LQFP64 package for prototype friendliness and JLCPCB/PCBWay assembly.

Key design requirements from the ST documentation:

- 1.71-3.6 V VDD operating range; design at 3.3 V.
- Place 100 nF ceramic decoupling at each VDD/VSS pair, close to the pins.
- Add approximately 4.7-10 uF bulk ceramic close to the MCU.
- VDDA requires separate local decoupling; feed from the 3.3 V rail through a ferrite bead or quiet RC/ferrite network where appropriate.
- Keep VREF+ clean and decoupled even though the precision weighing ADC is external.
- Follow ST AN5093 for grounding, high-current separation and placement.
- Expose SWDIO, SWCLK, NRST, 3V3 and GND on a programming header.
- Provide a BOOT0 strategy consistent with the STM32G491 boot architecture. A pull-down and accessible test pad/jumper are preferred.
- USB device D+/D- are routed as a matched 90-ohm differential pair to the Type-C receptacle.

Files: `STM32G491_Datasheet.pdf`, `STM32G4_RM0440_Reference_Manual.pdf`, `STM32G4_AN5093_Hardware_Development.pdf`.

### Precision ADC - ADS1262

Keep. It is well matched to the EMFC current-shunt measurement.

- 32-bit delta-sigma converter.
- Up to 38 kSPS.
- 10 multiplexed analog inputs.
- Integrated PGA and internal reference available.
- Separate analog and digital supplies.
- Operate AVDD at 5 V and DVDD at 3.3 V.
- Use an external precision reference for the production design while preserving the ability to use the internal reference during bring-up.
- Place AVDD, DVDD and reference bypass capacitors immediately at the pins according to the typical application in the datasheet.
- Route the 10-ohm shunt with true Kelvin traces directly to the ADC input filter.
- Do not run SPI clock traces parallel to the shunt inputs or reference traces.
- Use DRDY as an interrupt input to the MCU.

Initial shunt ADC filter:

- 68 ohm series resistor in each differential input lead.
- 100 nF differential capacitor as a starting value.
- Add optional unpopulated common-mode capacitors for lab tuning.

File: `ADS1262_Datasheet.pdf`.

### Force-coil driver - OPA593

Keep. It provides ample voltage and current margin for the approximately 5-60 mA normal coil-current range and approximately 80 mA hardware limit.

- Minimum total supply is 8 V; use a nominal 9-12 V actuator rail.
- Up to 250 mA output capability.
- Adjustable current limit.
- Thermal-warning/current-limit status should be routed to MCU GPIO.
- Use a large copper thermal area and thermal vias under/around the power package as required by the selected package.
- Keep its thermal plume away from the reference, shunt, ADC and mechanical sensor.

File: `OPA593_Datasheet.pdf`.

### Optical front end - OPA2388 + Hamamatsu S3096-02 + TSAL6200

Keep.

OPA2388 is a dual zero-drift, rail-to-rail precision amplifier suited for two transimpedance channels and differential/servo stages.

Initial optical architecture:

- 940 nm TSAL6200 emitter, DC-driven rather than PWM.
- S3096-02 dual photodiode.
- Two OPA2388 transimpedance stages.
- Measure both photodiode difference and sum.
- Difference is the servo position error.
- Sum is a health/contamination/alignment diagnostic.

Starting TIA values:

- 33.2 kohm feedback resistor.
- 100 pF C0G feedback capacitor.
- Provide alternate footprints or selectable values for loop tuning.

Files: `OPA2388_Datasheet.pdf`, `Hamamatsu_S3096-02_Datasheet.pdf`.

### Temperature sensing - TMP117

Keep. Use three sensors minimum:

1. magnet/yoke,
2. flexure/mechanical frame,
3. precision AFE/shunt/reference area.

Use one I2C bus if addresses permit, otherwise use a simple I2C mux or address-strapped variants as required. Place the sensors thermally close to what they are measuring and away from self-heating components.

File: `TMP117_Datasheet.pdf`.

### Precision reference - LTC6655-2.5

Keep as the production reference choice.

Important datasheet requirements:

- Input bypass capacitor of at least 0.1 uF placed close to the part.
- Output capacitance is required; follow the exact selected LTC6655/LTC6655LN variant guidance in the datasheet.
- Avoid loading the reference with rapidly changing loads.
- Place it in a thermally quiet region away from regulators, OPA593, shunt and MCU.
- Use a guard/ground moat strategy only if it does not create return-current discontinuities.

File: `LTC6655_Datasheet.pdf`.

### Precision 5 V analog regulator - LT3045

Keep.

The LT3045 is used to clean the precision analog supply. Feed it from the charger/system bus or a preregulated rail with enough headroom. Use the datasheet-required SET resistor/capacitor, input/output capacitors, current-limit components and layout.

Because the scale analog load is modest, direct linear regulation from an approximately 7-12 V system bus can be acceptable thermally if total analog current is kept low; verify the dissipation calculation during schematic review.

File: `LT3045_Datasheet.pdf`.

### Analog-loop reset/anti-windup - TMUX1101

Keep as a low-leakage analog switch used to reset or clamp the analog integrator during startup, overload, transport or a fault.

File: `TMUX1101_Datasheet.pdf`.

### Stepper drivers - four BTT TMC2209 modules

Keep and use sockets.

- VM accepts the intended 12 V motor rail.
- 3.3 V logic is compatible with the STM32.
- Each module receives STEP, DIR, EN and a dedicated UART from the MCU.
- One motor at a time is permitted by firmware and electrical-power budgeting.
- Add local bulk capacitance at every module socket, not only one capacitor for the whole board.
- Do not hot-plug a motor or driver module.
- Route motor phase pairs together and away from USB, UART, CAN, RS-485 and precision-board interconnects.
- Add a removable motor connector adjacent to each socket.
- Provide an accessible VREF point and enough airflow/heatsink clearance for the BTT module.

The TMC2209 itself is a chopper driver; the module already contains the required sense resistors and local implementation circuitry. The carrier board therefore needs primarily VM, GND, VIO, control signals and local bulk/decoupling support.

File: `TMC2209_Datasheet.pdf`.

### HMI - Nextion NX8048T050

Keep because it is already owned.

- Supply: 5 V.
- Budget 1 A at 5 V for the HMI rail even if typical operation is lower.
- UART only.
- Route MCU TX to Nextion RX and MCU RX to Nextion TX.
- Add 22-100 ohm source-series resistors near the MCU on TX/RX footprints for EMI tuning.
- Add ESD protection at the external HMI connector if the cable is detachable.
- Use a 4-pin keyed connector: +5V, GND, MCU_TX, MCU_RX.
- Keep the cable to a few inches where practical.

File: `Nextion_NX8048T050_Dimensions.pdf`; official product page is referenced in the datasheet index.

### USB-C

Use USB 2.0 device-only operation.

Recommended connector: GCT USB4105 or equivalent USB2.0 Type-C receptacle.

- Place 5.1 kohm Rd resistors from CC1 and CC2 to ground for a USB device/sink presentation.
- Do not use USB VBUS to power the entire machine.
- Use VBUS only for USB attach/VBUS sensing as required by the STM32 USB implementation.
- Protect D+/D- using TPD2EUSB30 or equivalent low-capacitance ESD array.
- Place the ESD array immediately behind the connector with a very short ground return.
- Route D+/D- as a 90-ohm differential pair with continuous reference plane and no stubs.

Files: `GCT_USB4105_USB-C_Receptacle.pdf`, `TPD2EUSB30_Datasheet.pdf`.

### CAN - TCAN1042H-class transceiver

The STM32G491 provides the FDCAN controller but not the physical-layer transceiver.

Use an external TCAN1042H/TCAN1042HV-class device.

- 5 V transceiver supply with logic-level compatibility appropriate to the selected VIO variant.
- 120 ohm termination selectable by jumper/DIP switch, not permanently installed unless this unit is known to be at a bus end.
- CANH/CANL brought to a pluggable screw terminal.
- Add common-mode choke and TVS footprints even if the first prototype omits the choke.
- Put ESD/transient protection at the connector side of the transceiver.

File: `TCAN1042H_Family_Datasheet.pdf`.

### RS-485 - THVD1450

Use one half-duplex transceiver.

- Operates from 3-5.5 V; use 3.3 V or 5 V according to final logic/power choice.
- Extended common-mode range and strong ESD rating are useful for machine wiring.
- A/B on pluggable screw terminal.
- 120 ohm termination selectable by jumper.
- Provide bias/failsafe resistor footprints but do not populate blindly; the THVD1450 has internal failsafe behavior and bus-wide biasing must be coordinated.

File: `THVD1450_Datasheet.pdf`.

## Power architecture

### Wall power

Use existing Mean Well modules:

- IRM-30-12: 12 V, 2.5 A, 30 W - primary motor/charger DC source.
- IRM-20-5: 5 V, 4 A, 20 W - HMI and low-voltage system source.

Mount these in a segregated mains compartment or on a separate mains PSU assembly. Their PCB pins carry mains potential on the input side. Maintain manufacturer creepage/clearance and enclosure requirements.

The IRM-30 datasheet allows 85-305 VAC input and specifies 12 V / 2.5 A for the IRM-30-12. The datasheet also gives specific wave/manual solder limits; treat these as through-hole parts to install after SMT reflow.

Files: `MeanWell_IRM-30_Datasheet.pdf`, `MeanWell_IRM-20_Datasheet.pdf`.

### Battery

Recommended production baseline:

- protected 2S Li-ion pack,
- 7.4 V nominal, 8.4 V fully charged,
- approximately 10 Ah / 74 Wh baseline,
- integrated 2S protection/BMS,
- pack temperature sensor strongly preferred.

A 40-50 Wh pack is a reasonable minimum for shorter usage. A 70-100 Wh pack is preferred for a 2-4 hour mixed-use target with one intermittent NEMA17 plus display and electronics.

Harvested laptop cells are not recommended in the finished machine unless the cells are capacity/IR matched and assembled into a properly protected and balanced pack by a competent battery-pack builder. Do not charge loose unmatched reclaimed cells in the machine.

### Charger/power path - MAX77960B

Use for a 2S pack.

- 3.5-25.4 V input operating range.
- 2S/3S Li-ion support.
- Integrated buck-boost charger and power path.
- Program a conservative charge current; start around 1 A and verify the full wall-power thermal budget.
- Firmware should be able to reduce/suspend charge activity during final precision settling.
- Use pack thermistor input and safety timer/termination features exactly as required by the datasheet.

File: `MAX77960B_Datasheet.pdf`.

### Battery-to-12 V motor rail - TPS61088

This replaces the earlier TPS63070 recommendation because a NEMA17 rail needs more power.

- 2.7-12 V input.
- Up to 12.6 V output.
- 10 A-class integrated switch.
- Configure for approximately 12.0 V output from the 2S pack.
- Use the exact inductor, compensation, current-limit, input/output capacitor and PCB layout procedure from the datasheet.
- Keep the hot switching loop compact and far from the precision-board connector.

File: `TPS61088_Datasheet.pdf`.

### Battery-to-5 V system rail - TPS62135

Use for the HMI/controller 5 V rail while on battery.

- 3-17 V input.
- Up to 4 A output.
- Configure to 5.0 V.
- Use forced-PWM mode if measurement testing shows that variable-frequency/PFM operation causes interference; otherwise power-save mode is acceptable outside precision windows.

File: `TPS62135_Datasheet.pdf`.

### 3.3 V digital rail - TPS62172

Use a small 3.3 V buck for MCU, logic and digital interfaces. The 0.5 A rating is ample for the control logic if the Nextion and transceivers are powered from their intended rails.

File: `TPS62172_Datasheet.pdf`.

### Source selection - TPS2121

Use power muxes or an equivalent ideal-diode arrangement so wall supplies automatically take priority over battery-derived rails without back-feeding.

At minimum, separately manage:

- 12V_MOTOR_WALL vs 12V_MOTOR_BAT,
- 5V_WALL vs 5V_BAT.

Keep charger/system-bus behavior in mind so no source can unintentionally feed another source.

File: `TPS2121_Datasheet.pdf`.

## E-stop and main power

### Rear mains input

Recommended chain:

IEC-C14 inlet -> replaceable fuse -> double-pole mains switch -> segregated mains distribution -> Mean Well modules.

Connect protective earth directly to the metal chassis with a dedicated bolt, star washer and earth conductor. Do not route protective earth through a PCB trace before bonding the chassis.

### E-stop

Use a normally-closed hardwired E-stop loop that removes the ability to energize motion outputs independently of MCU firmware.

Recommended result when pressed:

Disabled:

- 12 V power to all four stepper-driver VM inputs or their hard enable path,
- future robotic-arm motion-enable output,
- automatic calibration-mass actuator.

Remain powered:

- MCU,
- Nextion,
- scale/EMFC analog electronics,
- USB/CAN/RS-485,
- fault logging.

Use a monitored relay/contactor or appropriately designed high-side load switch in the enclosed electronics compartment. For a production machine handling energetic material, a formal machinery-safety assessment should determine whether a certified safety relay and dual-channel E-stop architecture are required.

## Automatic check-mass provision

Reserve on the controller board:

- one actuator-power connector (5 V and/or 12 V),
- one protected low-side or high-side actuator-control output,
- two limit/home sensor inputs,
- one spare PWM/timer output,
- one spare UART/I2C/SPI expansion header if a smarter actuator is later used.

Mechanically, the check mass should rest on a stationary support during normal weighing and be transferred gently to the weighing mechanism only during a calibration/check sequence. The actuator must be mechanically unloaded/disconnected from the weigh mechanism during the actual stable measurement.

## Assembly strategy and reflow

Target JLCPCB/PCBWay SMT assembly for:

- STM32G491,
- ADS1262,
- OPA2388,
- OPA593 if package/stock pricing is reasonable,
- TMP117,
- MAX77960B,
- TPS61088,
- TPS62135/TPS62172,
- LT3045,
- LTC6655 if assembly sourcing is economical,
- USB-C receptacle,
- CAN/RS-485 transceivers,
- ordinary passives.

Hand-install after SMT reflow:

- BTT TMC2209 plug-in modules,
- Mean Well through-hole AC/DC modules or separate mains assembly,
- pluggable terminal blocks,
- IEC inlet wiring,
- E-stop wiring/relay,
- photodiode/LED if mechanically mounted off board,
- expensive precision shunt/reference if assembly markup or availability is poor,
- large connectors.

No selected mainstream SMT IC requires an exotic assembly process; they are intended for standard lead-free SMT manufacturing. However, the exact orderable part's MSL/peak-reflow rating must be checked in the distributor/manufacturer quality data before purchase. The Mean Well modules and through-hole optical parts are not treated as reflow components.

## Future tool-battery option

Do not charge a DeWalt/Milwaukee battery in this machine. Treat a tool battery as an external DC source only, using an adapter that retains the manufacturer's pack protection. A future optional input can accept approximately 15-21 V from a nominal 18/20 V tool pack and feed a dedicated buck stage to the machine's 12 V/5 V buses. Leave this as a TODO until the exact tool-battery family is selected.
