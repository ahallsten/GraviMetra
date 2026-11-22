# EMFC Scale + Three-Stage Powder Dispenser — Master Design Prompt (v2)

## Part A — Setup (do this once, before pasting Part B)

1. Install on your desktop: **KiCad 10.0.x** (make sure `kicad-cli` is on PATH), **Python 3.11+**, **git**, **Java 17+** (for Freerouting), **arm-none-eabi-gcc + CMake**. The agent installs Python packages itself (build123d, pint, numpy, scipy, matplotlib).
2. Make the repo:
   ```
   mkdir emfc-dispenser && cd emfc-dispenser && git init
   mkdir -p docs reference/altium_legacy
   ```
   Copy `EMFC_Project_Context_and_Design_Audit.md` into `docs/`. Optional: copy your textbook index files (00_INDEX.md, TOPIC_*.md, BOOK_*.md) into `reference/textbooks/`.

   **Export your Altium work (about 15 minutes).** Altium's native files are binary, so Claude reads the exports below cheaply instead.
   1. Copy the whole Altium project folder (`.PrjPcb`, every `.SchDoc`, `.PcbDoc`, `.SchLib`, `.PcbLib`, `.IntLib`) into `reference/altium_legacy/raw/`. KiCad can import these if needed.
   2. **Schematic PDF:** File → Smart PDF → current project → all schematic sheets, with "Include No-ERC markers" and "Net information" on. Save it to `reference/altium_legacy/schematics.pdf`.
   3. **Netlist:** Design → Netlist For Project → Protel (or EDIF). Put the file in `reference/altium_legacy/`.
   4. **BOM:** Reports → Bill of Materials. Include Designator, Comment, Value, Footprint, Manufacturer, Manufacturer Part Number, Description and Sheet. Export as CSV to `reference/altium_legacy/bom.csv`.
   5. **PCB snapshot** (if any layout exists): File → Smart PDF of each PcbDoc (top/bottom layers plus assembly), or a screenshot of each board.
   6. Copy any older design docs, notes or handoff files into `reference/altium_legacy/docs/`.

   You don't need to clean anything up first. Incomplete and outdated sheets are fine; the audit exists to sort them out.
3. Launch Claude Code in that folder. The model and effort are set by this command, not by the prompt:
   ```
   claude --model opus --effort xhigh
   ```
4. Paste everything under **Part B** as the first and only prompt. You never write another prompt.
5. Every later session: run the same launch command, then type **"Continue per CLAUDE.md and docs/00_STATE.md."**
6. Type **"checkpoint"** at any time and the agent saves everything immediately. Do this when you see your usage running low.
7. **Effort switches:** Claude can't change its own effort. When a hard study is next, it stops and tells you to type `/effort max`. When the study is done, it tells you to type `/effort xhigh` again. The sourcing subagent's cheaper model (Sonnet) is set in a file the agent writes itself, so you do nothing for that.

---

## Part B — Prompt (paste everything below this line)

# ROLE AND GOAL

You are the lead engineer on my garage hobby project, covering systems, precision analog/mixed-signal, power, embedded firmware, flexure/precision mechanics, EMC and DFM. Design, end to end, a **three-stage (coarse / medium / fine) smokeless-powder dispenser** that weighs with either:

- **Config S (bought scale):** a commercial high-accuracy balance read over RS-232/USB. Examples: Torbal AD520, A&D FX-120i / FX-300i, G&G JJ-series, or similar AD520-class balances.
- **Config E (DIY scale):** my own **EMFC balance built to reverse-engineer Torbal AD520 performance**.

Both configurations share the same dispenser mechanics, controller board, firmware and enclosure family. The EMFC cell and its precision electronics are a separate module.

Deliverables: CAD, dimensioned drawings, all PCBs, BOM, firmware, test/bring-up/calibration procedures and a safety analysis.

About me: I'm a mechanical engineer. Be technical and concise. I have plenty of time, want high quality, and need **low cost**.

# FABRICATION CONSTRAINTS (CONFIRMED)

- **Printer:** Bambu Lab X2D with AMS 2 Pro, sliced in Bambu Studio. Verify its build volume, nozzle options and printable materials from Bambu's site.
- **3D print as much as possible.** Buy only what can't sensibly be made: electronics, motors, magnets, bearings, fasteners, belts/pulleys, springs, calibration masses, and flexure stock if needed. Keep the purchased mechanical part count small and justify each one.
- **SendCutSend is welcome** for laser/waterjet-cut sheet parts and their bending, tapping and hardware insertion. Check their current material/thickness catalog and design rules, and design flat parts to their DXF requirements.
- **No CNC machining.** I have a CNC at work but won't use it for a home project. List any part where CNC would materially help as OPTIONAL, with a non-CNC alternative always designed and costed.
- **PCBs:** JLCPCB or PCBWay fabrication and assembly. Prefer JLC basic parts where performance allows.
- **CONFIRMED: no printed parts in the scale cell or anything that supports it.** That covers the base/frame, lever, pivots/flexures, parallel guide, pan support, counterweight, coil and magnet mounts, optical sensor mount, and the structure between the cell and the bench. All of it is **metal from SendCutSend, designed as flat parts**: laser- or waterjet-cut plate and sheet, flat-blade flexures in spring steel or stainless, stacked or bolted plates, and SendCutSend bends, taps and hardware where needed. Joints are bolted or pinned. Use reamed or dowel-pin locating features cut into the flats, so alignment comes from the cut geometry rather than from machining. Purchased parts are allowed only where they're clearly better, such as the magnet assembly, precision dowel pins, and the coil former if needed. Choose materials for thermal expansion matching and stability. Prefer one material throughout the cell, and document any mix with its thermal error. Printing is fine for everything outside the cell: enclosure, draft shield, hoppers, trickler bodies, motor mounts, covers and harness guides.

# INPUTS

- `docs/EMFC_Project_Context_and_Design_Audit.md` is the prior handoff. Its status labels and its rules in §1 and §18 are binding **except where this prompt supersedes them** (specs, mains, HMI, fabrication, Config S).
- `reference/altium_legacy/` holds my partial Altium project (~22 SchDocs, 3 PcbDocs). It is a read-only reference, not the source of truth. Extract circuits, nets and intent from it. KiCad can import Altium files if that helps extraction.
- `reference/textbooks/` (if present) is my textbook index. Use it for mechanical methods and cite as [BOOK_CODE p.###]. Label any method from outside the index as an outside method.

# CONFIRMED DECISIONS (supersede the handoff)

1. **Metrology spec = Torbal AD520-class.** This replaces the earlier 0.01 gr / ±0.5 mg target, which is SUPERSEDED. Starting values, from retailer data and the AD-series manual (verify against Torbal's own manual):

   | Parameter | Value |
   |---|---|
   | Capacity | 520 g |
   | Readability | 0.001 g |
   | Repeatability (SD) | 0.001 g |
   | Linearity | ±0.002 g |
   | Stabilization | ~3 s |
   | Operating temperature | 15–30 °C |

   Add internal calibration, which goes beyond the AD520. Fill in the rest from the manual or by reasoned derivation, and label each item: eccentric-load, zero/span tempco, warm-up, overload. Display grams and grains; at 0.001 g, show grains at 0.02 gr steps.
2. **Magnets: samarium-cobalt (SmCo), with magnet temperature measurement** and firmware span compensation. Also measure coil temperature and ambient temperature. Evaluate a constant-power coil scheme.
3. **Counterweight for the pan and tray.** I said "kind of wait" in a voice note; I believe I meant counterweight. Confirm this in Phase 0. The intent is that a mechanical counterweight carries the pan/tray tare so the coil carries mostly the net load. Size it against the 520 g capacity requirement and show the tradeoff.
4. **Config S is a first-class product, not an afterthought.** It needs:
   - A firmware scale-driver layer with a common interface (weight, stable flag, tare, units, update rate).
   - Drivers for each supported scale's protocol, taken from its manual.
   - An RS-232 port (and USB-host if justified) on the controller board.
   - Mechanical adapters so the trickler assembly positions its outlets over each supported scale's pan.
   - A support matrix: model, protocol, update rate, cable/pinout, adapter part.
   - Verification that each candidate streams continuously at an adequate rate and reports a stable flag.
5. **Mains = internal.** Use an IEC C14 inlet (fused and switched) feeding PCB-mount Mean Well modules I own; I'll give the model numbers. Mains goes on its own board and in its own compartment, with a PE bond to the chassis and to powder-contact metal, and a mains/SELV barrier. No external power brick.
6. **HMI = UNRESOLVED.** I have many Adafruit-style displays. Ask me for model numbers or photos, compare them with the Nextion NX8048T050, and **don't start any HMI design until I pick one**. The choice must be made before the MCU pinout is frozen.
7. **Three trickler channels** (coarse/medium/fine), belt-driven steppers turning tubes/shafts that feed from the hopper(s) into the pan. Only one motor runs at a time. Current driver candidate: bare TMC2209.
8. **Prior art:** study OpenTrickler (github.com/eamars/OpenTrickler, open source, 3D-printed, supports the A&D FX-120/300i and G&G JJ100B) and the AutoTrickler concept. Reuse ideas where its license allows, credit it, and say what you changed and why.

# END-STATE DELIVERABLES

1. **Requirements spec** with a status label and a verification method on each line.
2. **Analyses as runnable Python with units (pint):**
   - EMFC lever ratio and force chain.
   - SmCo magnet and coil design: force constant, current, dissipation.
   - Flexure stiffness, stress and fatigue.
   - Counterweight sizing.
   - Thermal model with temperature compensation.
   - Full error and noise budget vs. AD520 specs.
   - Servo loop model: plant + controller, Bode and step response, margins, saturation and anti-windup.
   - Power budget.
   - Motor torque, speed and belt sizing.
   - Creep and hysteresis assessment for any printed part in the force path.

   Each analysis states its assumptions and design factors and includes a sanity check.
3. **Mechanical CAD in build123d**, parametric with one params file per assembly. Cover:
   - The EMFC cell.
   - The counterweight.
   - The draft shield and the pan/tray.
   - The hopper(s), the three tricklers and their belt drives, and the motor mounts.
   - A trickler frame isolated from the scale.
   - The Config S scale adapters.
   - The enclosure with its mains compartment, plus board mounts and harness routing.

   Outputs:
   - STEP files (assembly and parts).
   - **STL/3MF files with Bambu Studio print notes:** material, orientation, supports, wall count/infill, hole compensation and heat-set inserts.
   - **DXF flat patterns for every SendCutSend part:** material, thickness, bends, hardware and quantity, in an order list.
   - **Dimensioned 2D drawings (PDF)** for every made part and every assembly, including assembly order.
   - An interference check against board STEPs exported from KiCad.
4. **Electronics in KiCad 10**: one KiCad project per PCB in this repo, a shared verified project library, and hierarchical schematics. Boards: at least controller/motor, EMFC precision module (Config E only) and mains. Justify the final count. ERC/DRC must be clean, or each waiver must be justified. Provide JLCPCB outputs (Gerber, drill, BOM and CPL in JLC format) and PCBWay outputs, plus a STEP per board, an assembly drawing, and a test-point map.
5. **BOM per handoff §12**: MPN, lifecycle, price at qty 1 and qty 10, Digi-Key/Mouser stock, LCSC/JLC part number and basic/extended status, PCBWay availability, package, assembly difficulty, approved alternates, selection reason, and substitution impact. It also covers mechanical buys, SendCutSend parts with quotes, and filament use. Roll up cost per configuration (S and E) and per prototype round.
6. **Firmware** for the STM32 that builds headless (CMake + arm-none-eabi-gcc). It covers:
   - The scale-driver layer: EMFC internal plus each commercial protocol.
   - The restoration servo (if digital or hybrid), with magnet-temperature span compensation.
   - The coarse/medium/fine dosing state machine with in-flight and settling compensation, and stable-weight qualification.
   - A safety supervisor.
   - Calibration (external masses plus internal check mass) and tare.
   - The HMI protocol, fault logging and USB service.

   Include host-side unit tests and a scale simulator, so the dosing logic can be tested without hardware.
7. **Bring-up and test plan** for each board, an EMFC characterization plan, a calibration procedure (no traceability claims the design doesn't support), and an FMEA/hazard analysis.
8. **Review package at each gate**: schematic PDFs, PCB layer renders, CAD renders, budgets, and open items.

# TOOLCHAIN (text-first, to keep token cost low)

- **KiCad 10.** Edit `.kicad_sch`, `.kicad_sym` and `.kicad_mod` as S-expression files through Python scripts; KiCad 10's IPC API has no schematic commands. Use `kicad-cli` for ERC, DRC, netlist, BOM, Gerbers, drill, position files, STEP and PDF/SVG. Use scripted pcbnew or kicad-python for placement. Use Freerouting only for non-critical digital nets. Define the critical routes yourself with explicit geometry: TIA inputs, Kelvin shunt, reference, coil, motor phases, switcher loops, returns and the mains barrier. I'll review and touch up in the GUI.
- **Mechanical.** Use build123d, with drawings from build123d's 2D projection export or FreeCAD TechDraw scripted from STEP. Use closed-form methods first. Use FEA only with open tools (gmsh + CalculiX), and only where closed-form isn't adequate; say why when you do. Design in adjustability (coil position, sensor gap, counterweight trim, level) so it absorbs printing and assembly tolerances.
- **SPICE.** Use ngspice for the TIA, the coil driver and the analog loop.
- **Context discipline.** Never read a whole `.kicad_pcb`, `.kicad_sch` or STEP file into context; query them with scripts. Render images only for gate reviews. Every output must be regenerable through `make` targets.
- **Sourcing subagent.** Create `.claude/agents/sourcing.md` (model: sonnet, effort: medium) for bulk datasheet, price and stock lookups. It writes results to files, and you review them.
- **Effort switching.** You can't change your own effort level, so I switch it. Before the EMFC mechanics study, the servo architecture study and the error/noise budget, stop and say: **"Type `/effort max`, then 'continue'."** Do the same in reverse (`/effort xhigh`) once those studies are done.

# SAVING AND RESUMING (critical: my sessions end when credits run out)

Create these on the first run:

```
CLAUDE.md                 standing rules (condense RULES + CONFIRMED DECISIONS; ≤100 lines)
docs/00_STATE.md          phase, done, in-progress (with exact file/step), next 3 actions, effort needed (≤60 lines)
docs/01_REQUIREMENTS.md   docs/02_DECISIONS.md (ADR log)   docs/03_OPEN_ITEMS.md (incl. RELEASE BLOCKERs)
docs/04_BUDGETS.md        docs/05_FMEA.md   docs/datasheets.md   docs/SCALE_SUPPORT.md
analysis/  mech/  hw/<board>/  hw/lib/  fw/  test/  reference/ (read-only)
```

- **Checkpoint after every completed sub-deliverable** (one analysis, one CAD part, one schematic sheet, one firmware module). Don't wait for the end of a session. A checkpoint means: update `00_STATE.md` with exactly where to resume, append any decisions to `02_DECISIONS.md`, and `git commit`.
- **Don't let a single uncommitted task run long.** Break big tasks so no more than about 20 minutes of work is ever uncommitted.
- If I type **"checkpoint"**, stop, save as above, and confirm in one line.
- At the start of every session, read only `CLAUDE.md` and `docs/00_STATE.md`, then whatever the next action needs.

# RULES

- Use the status vocabulary consistently: **CONFIRMED / CANDIDATE / PROPOSED / UNRESOLVED / SUPERSEDED / RELEASE BLOCKER / ASSUMED**. ASSUMED marks a default applied because I didn't answer.
- When you find a contradiction, show both versions and resolve it with evidence or ask me. Never pick one silently.
- Take every vendor spec from the current manufacturer datasheet or manual, and take prices and stock from distributors. Never work from memory. Record the sources.
- Prefer manufacturer reference designs and layout guidelines.
- A passing ERC/DRC is not proof of correctness. Review each sheet against intent, the datasheets and the budgets.
- Explain *why* for every design decision, in one or two sentences.
- Don't equate readability with accuracy, don't call an internal check mass traceable calibration, and don't claim hazardous-location compliance.
- The SUPERSEDED items in handoff §11 (4 channels, 250 g, 50 g, plug-in TMC modules, MAX77960B, TCAN1042H, THVD1450, S3096-02), the 0.01 gr / ±0.5 mg spec and the external-brick idea must not drive the design.

# PHASE 0 — AUDIT, THEN ASK (one message, at most 15 questions)

1. Read the handoff. Then **audit the Altium reference** using the PDF, netlist and BOM exports in `reference/altium_legacy/`. Open the raw files only if an export is missing; KiCad can import them.
   - Produce `docs/ALTIUM_AUDIT.md` with one row per schematic sheet: sheet name → what it actually does (in one line) → key parts → verdict. Verdicts:
     - **KEEP:** a sound circuit, still needed.
     - **MODIFY:** needed, but changed by this prompt (e.g., 4 → 3 motor channels, BTT modules → bare TMC2209, old transceivers, mains changes, AD520 spec, SmCo, RS-232 scale port) — say what changes.
     - **DROP:** obsolete or superseded.
     - **UNKNOWN:** intent unclear — I'll explain.
   - Also list: nets or blocks that look wrong or incomplete, parts that are obsolete/NRND (check lifecycle), and any circuit worth reusing as-is.
   - End with a proposed mapping from the old sheets to the new KiCad hierarchy. Treat everything as unproven unless I say otherwise.
   - If no Altium exports are present, skip this step and say so.
2. Verify the AD520 specs; whether the AD520 itself is EMFC or a strain gauge (it doesn't change the targets); the protocols and stream rates of the Config S candidates; SendCutSend materials; X2D materials; OpenTrickler's license and architecture.
3. Send me **one message with at most 15 numbered questions**, grouped by topic. For each, give one line on why it matters, the options, and **your recommended default**. Use the candidate list below, but replace any question with a higher-value one if your audit finds it.
4. After that, ask nothing more except at gates, or when a decision is expensive or irreversible and evidence can't settle it. If I leave a question unanswered, apply your default, log it as ASSUMED, and continue.

## Candidate questions (total ≤ 15)

1. **Counterweight.** Confirm that "kind of wait for the pan and tray" means counterweight. *Default: yes.*
2. **Commercial scales.** Which do you own or plan to buy first for Config S? *Default: design for the A&D FX-120i/300i and Torbal AD520 first.*
3. **Build order.** Build Config S first (working dispenser soonest) while the EMFC rig is characterized in parallel? *Default: yes.*
4. **Powders and throughput.** Powder forms (spherical, stick incl. long-kernel, flake), charge range in grains, target time per charge. *Default: all three forms, 5–150 gr, ≤30 s.*
5. **Hopper.** One hopper feeding all three tricklers, or separate hoppers? Maximum resident powder? *Default: one hopper, ≤1 lb, quick-empty.*
6. **Pan handling.** Removable pan you pour from? Auto-detect and auto-tare? *Default: removable conductive pan, detection plus auto-tare.*
7. **Power.** Exact Mean Well model numbers and quantities. Metal or plastic mains compartment? Battery purpose and runtime? *Default: grounded metal mains compartment from SendCutSend; protected 2S pack, 2 h runtime.*
8. **Budget.** All-in prototype budget per configuration, and the number of units. *Default: 1–3 units; report cost per configuration and per prototype round.*
9. **Printing and shop.** Filaments you stock or will buy (PLA/PETG/ASA/PC/CF-nylon/ESD-safe), and your hand tools (drill press, taps, reamers, hot-air station for inserts). *Default: PETG/ASA plus ESD-safe for powder contact; drill press, taps, insert tool.*
10. **PCB assembly.** Factory SMT, or your hand-solder capability, and the smallest package you'll hand-place. *Default: factory SMT for all fine pitch; 0402 minimum; no BGA.*
11. **Test gear and masses.** Scope, DMM digits, bench supply, and any certified masses (values and class). *Default: 2-ch scope, 4.5-digit DMM, and I budget for OIML F1-class 200 g and 500 g masses.*
12. **Displays.** List the model numbers, or send photos, of the displays you have.
13. **Firmware.** Toolchain and debug probe. *Default: CMake with CubeMX HAL/LL, ST-Link V3.*
14. **Legacy design and comms.** Is the full Altium project in `reference/`? Any blocks you consider proven? Any concrete use for CAN or RS-485 beyond the scale's RS-232? *Default: all legacy unproven; USB-C populated; CAN and RS-485 footprints DNP.*
15. **Environment and CAD.** Room temperature range, drafts, bench vibration, maximum footprint, and your native CAD for STEP import. *Default: 18–26 °C garage bench (climate-controlled?), ≤450 × 350 mm footprint.*

## Known issues to address in Phase 2

- **All-metal flat-part cell:** design the lever, flexures and parallel guide as SendCutSend flat parts. Quantify how cut-edge quality, kerf taper, flatness, bolted-joint slip and hysteresis, and thermal expansion affect the 0.001 g repeatability and ±0.002 g linearity, and design in the adjustments that absorb them.
- **SmCo span tempco:** with magnet temperature sensing and compensation, show the residual is within budget. Show the effect of coil I²R heating.
- **Restoration measurand:** compare analog current via Kelvin shunt into the ADS1262, a PWM-switched precision current source (duty cycle as the measurand; check the STM32G491 timers against the G474 HRTIM), and a DAC-driven coil. If this justifies an MCU change, propose it.
- **Servo partition:** analog inner loop + digital outer loop vs. fully digital vs. fully analog. Cover latency, plant resonance, null acquisition at startup, and crash behavior.
- **Electrostatics:** powder-contact parts conductive or static-dissipative and bonded; this also prevents force errors on the pan.
- **Vibration:** isolate the trickler frame from the scale, and enforce a motors-off settle window before final acceptance.
- **Plant parameters** (stiffness, resonance, damping, sensor gain, force constant) must be measured on the characterization rig.

# PHASES AND GATES

At each gate, give a one-screen summary, the review package, and at most 5 decisions for me. Then stop and wait for my approval.

- **Phase 1 — Requirements freeze**, AD520-class. → **GATE A**
- **Phase 2 — Architecture.** EMFC mechanics study ranking printable / SendCutSend / purchased options; servo study; error budget; counterweight; power budget; board partition; safety architecture; Config S interface design; block diagram. → **GATE B**
- **Phase 3 — De-risking.**
  - **Config S path:** controller board Rev A, plus the trickler mechanics and an RS-232 link to a bought scale, gives a working dispenser early.
  - **Config E path:** a minimum **EMFC characterization rig** (cell + minimal sensor/driver board), measured before the final EMFC module is committed. Rev A boards get generous test points and 0 Ω loop-break options.

  → **GATE C**
- **Phase 4 — Detailed design.** CAD, drawings, schematics board by board with **sheet-by-sheet written review notes**, and the BOM audit. → **GATE D**
- **Phase 5 — Layout and integration.** Stackup, placement rationale, critical routes, DFM against JLC/PCBWay's current published rules, and enclosure interference checks. → **GATE E**
- **Phase 6 — Release package.** Fab and SendCutSend order files, print files, final BOM, firmware, and test, calibration and FMEA documents. **No release while any RELEASE BLOCKER is open.** → **GATE F**
- **Phase 7 — Characterize and iterate.** Finalize compensation, the temperature model and the dosing parameters from measured data.

# PCB REQUIREMENTS

- Document the partition rationale (noise, return currents, thermal, mains/SELV). Keep the EMFC precision analog physically and thermally separated from the motor drivers, switchers and the coil driver.
- Use solid ground planes and draw the return path for every high-di/dt loop. No split planes without justification. Define board-to-board grounding explicitly, with every power pin paired with its return. Define the termination of chassis, shields and PE.
- Precision analog: Kelvin shunt routing, guarded TIA inputs, low-leakage practice, reference placement and filtering, ADC input/anti-alias per the datasheet, and thermal symmetry.
- Mains board: creepage/clearance and a slot or barrier per the module datasheet and IEC 62368-1 practice, fusing, inrush/EMI filter if needed, and thermal derating.
- Lay out switchers and the TMC2209 per their datasheet layout guidance.
- Use keyed, polarized connectors, unique per function. Put ESD/TVS protection at every external connector, including RS-232.
- Default to a 4-layer stackup for any mixed-signal board; justify any 2-layer board.
- Put test points on every rail and key signal, plus SWD and bring-up jumpers.
- Library: verify every symbol and footprint against the datasheet, log it in `hw/lib/VERIFIED.md`, and prefer KiCad official libraries.

# SAFETY BASELINE (hardware-enforced, independent of firmware)

- A hardware E-stop and a lid interlock remove motor drive power or force driver enable off without going through the MCU. Restart requires a deliberate user action.
- Coil current is limited in hardware, and the coil driver is disabled on a fault. Every output pin has a defined safe state through power-on, reset, brown-out and an MCU crash. An external watchdog can disable the motors and the coil.
- One-motor-at-a-time is enforced in firmware. Evaluate also enforcing it in hardware (PROPOSED).
- Mains: a fused, switched IEC C14 inlet, PE bonding, isolated Mean Well modules, and mains confined to its own board and compartment.
- Reverse-polarity protection, per-rail fusing or eFuses, a protected battery pack with a thermal sensor, and handling of charger faults.
- **Powder zone:** no exposed switching nodes, arcing contacts, brushed motors or hot surfaces. Electronics sealed off from the powder path. Conductive powder-contact parts bonded to ground. Choose materials for static behavior, minimize resident powder, provide a way to empty it, and analyze dust.
- Motor and driver temperature monitoring, fault logging, and safe recovery.

# START

Begin Phase 0 now. Your first reply: one sentence of plan, then the audit, then the single question message. Checkpoint before sending the questions.
