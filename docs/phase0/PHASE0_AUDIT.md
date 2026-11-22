# EMFC Powder Dispenser — Phase 0 Audit (2026-11-21)

Status: Phase 0 audit and research complete. Waiting on answers to the 15 questions in §6.
Inputs reviewed: `EMFCScale.pdf` (Altium Smart PDF, 26 pp., 23 SchDocs) and `EMFC_Scale_Octopart_BOM_1.xlsx` (25 lines, sourcing snapshot dated 2026-08-17).

---

## 1. Altium audit: what exists

**Overall state:** the design is at the *architecture + part selection + MCU pin plan* stage. Hierarchy, sheet symbols and port lists exist, and key ICs are placed. **Almost no circuitry is wired:** no supporting passives, no power connections, no decoupling. Ten sheets are blank. Every designator is unannotated (`U?`, `J?`), and pages 24–26 are empty.

| Pg | Sheet | What's on it | Verdict | Notes / change |
|---|---|---|---|---|
| 1 | 01_TOP_LEVEL_BOARD_ASSY | 3 board sheet symbols (LV power path, precision sensor, controller/motor) | MODIFY | Board set changes: mains board (new, IEC C14 + Mean Well), controller/motor, EMFC precision module (Config E only). Whether the LV power path is a separate PCB is decided at Gate B. |
| 2 | MainsPSU (LV power-board top) | Sheet symbols for input mux, charger, rails, interface | MODIFY | Despite the name, this holds no mains. Child sheets are named CTRL_06/07/08 but live on the power board; rename them. |
| 3 | CTRL_06_POWER_INPUT_MUX | TPS259470 eFuse + LT3045 placed, unwired | MODIFY | The LT3045 is the precision analog LDO, but it's placed on the power-input sheet while the BOM assigns it to the precision PCB. Move it to the EMFC module. |
| 4 | CTRL_07_BATTERY_CHARGER | BQ25792, TPS61088, TPS62135, TPS62172 placed, unwired | UNKNOWN | Depends on whether a battery is kept, plus its size and runtime (Q8). |
| 5 | CTRL_08_DC_DC_RAILS | Blank | MODIFY | Rails get re-derived from the power budget. |
| 6 | PWR_09_CONTROLLER_INTERFACE | Blank | MODIFY | Board-to-board power/return pairing. |
| 7 | Controller_Motor_LowVoltage (top) | 10 sheet symbols with port lists | KEEP structure, MODIFY content | Sheet symbols overlap each other and the text is illegible in places. Port names don't match the MCU sheet (see §2). |
| 8 | CTRL_01_MCU | STM32G491RET6 with ~45 net labels (pin plan) | MODIFY | Most valuable sheet. Fix the issues in §2. |
| 9 | CTRL_02_USB | USB4105 placed, unwired | MODIFY | Add CC resistors, ESD and VBUS sense. |
| 10 | CTRL_03_HMI | DF11-4 connector only | UNRESOLVED | Waits on the display choice (Q13). |
| 11 | CTRL_04_TMC2209_X4 | Blank | MODIFY | ×4 becomes ×3 bare TMC2209. |
| 12 | CTRL_05_CAN_RS485 | TCAN1044A + THVD2450 placed, partial ports | MODIFY | Keep as a DNP option. **Add RS-232** for the Config S scale link (new sheet). |
| 13 | CTRL_09_ESTOP_SAFETY_IO | Blank | MODIFY | Hardware E-stop/lid interlock/motor-power cut is required. |
| 14 | CTRL_10_CAL_ACTUATOR_IO | Blank | KEEP intent | Internal calibration mass actuator. |
| 15 | CTRL_11_SENSOR_BOARD_INTERFACE | Blank (port list defined on parent) | MODIFY | Only populated for Config E. |
| 16 | Sensors_Analog (top) | SNS_01..07 sheet symbols with ports | KEEP structure | Becomes the EMFC module top. |
| 17 | SNS_01_ADS1262 | ADS1262 placed, unwired | MODIFY | Its role (force-measurement ADC only, or also in the loop) is decided in the Phase 2 servo study. |
| 18 | SNS_02_REFERENCE_POWER | LTC6655-2.5 placed, unwired | MODIFY | On production hold for stock; alternates being researched. |
| 19 | SNS_03_PHOTODIODE_TIA | Blank | MODIFY | OPR2100 differential TIA. |
| 20 | SNS_04_ANALOG_SERVO | Blank | UNKNOWN | Exists only if the analog or hybrid servo wins in Phase 2. |
| 21 | SNS_05_OPA593_COIL | Blank | MODIFY | OPA593 has zero stock; alternates being researched. |
| 22 | SNS_06_TEMPERATURE_DIAGNOSTICS | 1 × TMP117 placed | KEEP, extend | 3 × TMP117 cover **magnet (SmCo), coil and ambient**, as now required. |
| 23 | SNS_07_CONTROLLER_INTERFACE | Blank | MODIFY | |
| 24–26 | (none) | Blank pages, no PCB content | DROP | |

**Recommendation (PROPOSED):** don't import the Altium files into KiCad. There's very little wired circuitry to preserve, and an import would carry over the unannotated parts, the overlapping symbols and the mismatched net names. Instead, rebuild natively in KiCad and carry over the hierarchy (reorganized), the port lists, the corrected MCU pin plan and the part selections. Estimated work lost: under an hour.

## 2. MCU pin-plan issues (CTRL_01_MCU)

1. **`AUGER1_STEP` is on both PA1 and PB14**, a duplicate net that would short two outputs.
2. There are four motor channels (AUGER1–4, TMC1–4_EN); this becomes three.
3. **The MCU sheet's net names don't match the parent sheet entries.** For example, `AUGERn_STEP` vs. `Mn_STEP`, `NEXTION_TX/RX` vs. `HMI_TX/RX`, `ESTOP_OK` vs. `ESTOP_STATUS_N`, `GLOBAL_MOTION_ENABLE` vs. `MOTION_PWR_PERMIT`, and `CAL_HOME/CAL_END` vs. `CAL_LIMIT_HOME_N/END_N`. As drawn, the hierarchy wouldn't connect.
4. Missing: VDD/VDDA/VREF+ supply nets and decoupling (only VSS→GND is drawn), an HSE crystal (PF0/PF1 are unused), the NRST circuit, BOOT0 (PB8) handling, and an SWO/trace option.
5. No UART is allocated for the **Config S scale RS-232 link**.
6. The peripheral choices look plausible but must be validated in CubeMX: USB on PA11/12, SPI1 (PA5–7) for the ADS1262, FDCAN2 on PB12/13, LPUART1 on PC0/1 for RS-485, USART1 on PB6/7 for the HMI, I2C3 on PC8/9, and SWD on PA13/14.

## 3. BOM findings

- 25 lines, 33 units; 23 TBD items in "TBD and Notes" (all passives, inductors, connectors, the battery, motors, the magnet/coil and the safety components).
- **Production holds:** OPA593DNTT (0 stock), LTC6655BHMS8-2.5 (25 in stock), and the VPG Z-foil 10 Ω shunt (0 stock, minimum order 50, 39-week lead). An agent is researching alternates.
- Changes forced by the confirmed decisions:
  - TMC2209 ×4 → ×3, and NEMA17 ×4 → ×3.
  - Nextion → TBD.
  - NdFeB → SmCo.
  - Add an RS-232 transceiver.
  - Add the counterweight and the SendCutSend cell parts.
- The BOM targets Digi-Key only and has no LCSC/JLC part numbers, so it needs re-sourcing for JLC assembly (agent running).
- The battery is listed as 2S, about 10 Ah / 74 Wh. That's large, with matching charger and safety cost, so it needs a runtime justification (Q8).
- The Mean Well IRM-30-12 + IRM-20-5 are listed as Active with good stock. Are these the modules you own (Q7)?

## 4. Pre-check findings

- **AD520 targets** (AD-series manual and retailer data): 520 g; readability 0.001 g; repeatability SD 0.001 g; linearity ±0.002 g; about 3 s stabilization; 15–30 °C; external calibration. The AD520's own sensor type isn't confirmed (the protocol agent is checking); this doesn't change the targets.
- **What AD520-class means numerically:** 520 g / 1 mg = 520,000 divisions, and ±2 mg linearity ≈ ±3.8 ppm of full scale. It's about 4× easier than the old target, but still a precision instrument. SmCo still drifts roughly 300–450 ppm/K (to verify per grade), so magnet temperature compensation and the counterweight remain essential, not optional.
- **SendCutSend (from their catalog):**
  - 304 stainless, laser cut, 0.030–0.500 in.
  - 316 stainless, 0.060–0.250 in.
  - 5052/6061 aluminum up to 0.500 in (6061 up to 0.750 in).
  - 7075-T6, 0.125–0.250 in.
  - **MIC-6 cast tooling plate, 0.250–0.500 in**, a strong candidate for a flat, stable cell base.
  - **1075 blue-temper spring steel, 0.015 in only.** That one thickness is the flexure stock option, with thin 304 as the alternative. Flexure geometry has to be designed around these few gauges.
- **Bambu X2D:** 256 × 256 × 260 mm build volume, 300 °C nozzle, 65 °C actively heated chamber, so ASA, ABS and nylon are practical. The largest printed part, likely an enclosure panel, must fit 256 mm or be split.
- **OpenTrickler:** licensed **GPL-3.0**, built on an RP2040 (Raspberry Pi Pico) with a motor expansion board, and supports the A&D FX. Borrowing ideas is fine. **Copying its code would make your firmware GPL-3.0 too.** (Q14)

## 5. Agent findings (all complete; details in the project files)

- **`EMFC_scale_search.md`:** best in budget is a new A&D GF-200P (about $495 delivered); also a used FX-300iN (about $389) and a used Mettler PG203-S (about $222, no power supply). A used FX-120i sells for about $400–450.
- **`EMFC_scale_protocols.md`:** the AD520 frame has **no stable flag**, and its stream rate and pinout aren't documented, so it's a poor bought-scale target. Driver priority: **A&D (FX-i/FZ-i/GF), then Mettler MT-SICS, then Shimadzu.** Sartorius and G&G have no documented stream; U.S. Solid manages at most about 2 readings/s with no stable flag.
- **`EMFC_bom_audit.md`** (stock read 2026-11-21):
  - **STM32G491RET6:** now 0 at Digi-Key, 53 at LCSC.
  - **TMC2209-LA:** only 21 at LCSC.
  - **OPA593:** every suffix is out of stock (Digi-Key expects 250 in May 2027). The best fallback is a precision op amp + BUF634A with an external current clamp.
  - **LTC6655B:** less scarce than the BOM said. The LTC6655LNB-2.5 is a strong option.
  - **Shunt:** VCS1625Z is the same-footprint drop-in (stock unverified). The VCS1625P is a test-build stand-in only.
  - **RS-232:** MAX3232IPWR (LCSC C113205).
  - Almost all ICs are JLC Extended.
  - **Hand-source:** OPA593, LTC6655B, VPG shunt, OPR2100, TMUX1101 and TSAL6200.
  - MCU and driver availability feeds into the Phase 2 MCU decision.

## 6. Phase 0 questions (15). Each has a default used if left unanswered.

1. **Counterweight.** Did "kind of wait for the pan and tray" mean *counterweight*? *Default: yes.*
2. **Altium.** OK to rebuild in KiCad, using the Altium work as a spec (not an import)? *Default: yes.*
3. **Bought scale and build order.** Which commercial scale do you own or plan to buy? OK to build the bought-scale dispenser first while the DIY cell is characterized? *Default (updated after the protocol check): design for A&D FX-i/FZ-i/GF first, then Mettler MT-SICS; dispenser first.*
4. **Powder and speed.** Powder forms (spherical, stick incl. long-kernel, flake), charge range in grains, target seconds per charge. *Default: all forms, 5–150 gr, ≤30 s.*
5. **Hopper.** One hopper feeding all three tricklers, or separate? Maximum powder held in the machine? *Default: one hopper, ≤1 lb, quick-empty.*
6. **Pan.** Removable pan you pour from, with auto-detect and auto-tare? *Default: yes, conductive pan.*
7. **Mean Well.** Are the IRM-30-12 and IRM-20-5 the modules you own? List any others. *Default: yes; grounded metal mains compartment from SendCutSend.*
8. **Battery.** Why (range portability vs. ride-through), and what runtime? The current BOM's 74 Wh pack is big. *Default: drop the battery from Rev A and leave a connector/footprint for later.*
9. **Budget.** All-in prototype budget for each configuration, and how many units ever? *Default: 1–2 units; report cost per configuration and per prototype round.*
10. **Filament and tools.** Filaments you stock or will buy (PETG/ASA/PC/CF-nylon/ESD-safe), plus your tools (drill press, taps, reamers, heat-set tool, calipers, indicator). *Default: PETG/ASA plus ESD-safe for powder contact; drill press, taps, heat-set tool, calipers.*
11. **PCB assembly.** All SMT done by JLC? What's the smallest part you'll hand-solder? *Default: JLC does all SMT; you hand-solder through-hole parts and connectors.*
12. **Test gear and masses.** Scope, DMM (digits), bench supply, certified masses (values and class). *Default: 2-ch scope, 4.5-digit DMM; budget OIML F1 200 g + 500 g.*
13. **Displays.** List the model numbers, or send photos, of the displays you have.
14. **Firmware and comms.** Toolchain and debug probe? Original firmware (MIT-style) or OK to borrow GPL code? Keep CAN/RS-485 as unpopulated options? *Default: CMake + CubeMX HAL/LL, ST-Link V3, original code, CAN/RS-485 unpopulated.*
15. **Environment and CAD.** Garage temperature range and whether it's climate-controlled; drafts/vibration; maximum footprint; your native CAD for STEP. *Default: 15–30 °C garage, ≤450 × 350 mm, STEP + STL + PDF.*
