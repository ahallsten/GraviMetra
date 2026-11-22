# 250 g EMFC Scale + Four-Stage Auger Controller - Design Pack

**Pack date:** 2026-08-13  
**Status:** Revision-A engineering design baseline; schematic/PCB not yet released for manufacture.

## What this pack contains

This pack consolidates the current electrical, mechanical, PCB-layout, firmware-planning, component-datasheet, and safety guidance for a custom 250 g electromagnetic force-compensation scale with four provisioned TMC2209/NEMA17 auger channels and a Nextion NX8048T050 HMI.

Read the files in this order:

1. `05_AI_Reference/AI_Reference_Context.md` - compact system context and frozen decisions.
2. `01_Electrical_Design/Electrical_Architecture_and_Component_Review.md` - component review and subsystem design.
3. `01_Electrical_Design/Power_Budget.md` - wall/battery power architecture and sizing assumptions.
4. `01_Electrical_Design/Assembly_and_Reflow_Plan.md` - JLCPCB/PCBWay assembly split, reflow/MSL and hand-install strategy.
5. `01_Electrical_Design/Connector_and_IO_Plan.md` - connectors and I/O assignments.
6. `02_EMFC_Mechanics/250g_EMFC_Design_Guide.md` - EMFC mechanics, flexure/lever/coil/null detector and calibration structure.
7. `03_Altium/Altium_Designer_Step_by_Step_Guide.md` - schematic-to-PCB-to-fabrication workflow for Altium Designer.
8. `04_Firmware/Codex_Firmware_Implementation_Prompt.md` - prompt intended for Codex to build the STM32 firmware.
9. `05_AI_Reference/Component_Datasheet_Index.md` - local-datasheet map and design-critical notes.
10. `01_Electrical_Design/Datasheets/` - manufacturer PDFs/reference documents.

## Major Revision-A choices

- 250 g live capacity; 0.01 grain (0.647989 mg) target displayed increment.
- Approximately 25:1 mechanical force reduction and approximately 2 N/A voice-coil target to preserve a roughly 50-60 mA full-load actuator-current range.
- Analog EMFC position-restoration loop for Revision A.
- ADS1262 + precision four-terminal 10 ohm shunt for restoring-current measurement.
- STM32G491RET6 controller.
- Four BTT TMC2209 module sockets; only one motor active at any instant.
- NX8048T050 Nextion over UART.
- USB-C device/service port.
- External TCAN1042-family CAN and THVD1450 RS-485 transceivers.
- Protected matched 2S Li-ion battery; approximately 70-75 Wh recommended baseline for 2-4 h mixed/intermittent duty, subject to measured prototype power.
- Mean Well IRM-30-12 + IRM-20-5 for wall operation.
- IEC-C14, fuse, double-pole master switch and protective-earth chassis bond.
- Hardware E-stop removes motion independently of software while retaining scale/HMI/controller diagnostics.
- Manual certified-mass calibration plus automatic internal check-mass mechanism.

## Important open items before PCB release

The architecture is frozen enough to start schematic capture, but these values must be resolved before a manufacturing release:

- exact protected 2S battery pack/BMS and connector,
- exact NEMA17 current setting used by each auger,
- final TPS61088 inductor/current-limit/compensation from measured worst-case motor load,
- exact STM32G491 pin assignment validated in CubeMX,
- exact TCAN1042 suffix / VIO arrangement,
- exact VPG 10 ohm shunt ordering suffix and assembly availability,
- exact Susumu matched-network suffix,
- BTT module physical pinout/orientation verified against modules in hand,
- final EMFC coil resistance/inductance/force constant,
- FEA-derived flexure dimensions,
- measured analog servo-loop compensation,
- internal calibration-mass actuator selection,
- certified calibration-weight set,
- formal safety/risk assessment for the actual powder/process environment.

## Safety statement

This project may be used around combustible or energetic powder. The included guidance emphasizes static control, segregation of potential ignition sources, protective-earth bonding, hardware motion shutdown, and separation of mains wiring, but the pack is not a substitute for a competent process-hazard analysis, machinery-safety design, electrical-code review, hazardous-location classification, or certification. Do not treat a successful bench prototype as evidence of safe operation around combustible dust/powder.

## About the datasheets

The local PDF set is intended to make the project usable offline and to support AI-assisted questions. Manufacturer documentation can change; check current revisions immediately before ordering or PCB release. `Component_Datasheet_Index.md` states which document governs each block and highlights relevant requirements such as supply ranges, decoupling, reference filtering, thermal layout, communication-level compatibility, and switching-regulator implementation.
