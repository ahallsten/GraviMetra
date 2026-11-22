# EMFC Scale + Three-Stage Powder Dispenser

## Project Context, Decision Audit, and Next-Agent Handoff

**Status:** Active hobby engineering project; Altium schematic/PCB work
has started, but the existing design guidance is not considered
authoritative enough to manufacture from.\
**Purpose of this file:** Give the next engineering agent a clean
baseline, identify superseded decisions, and force unresolved questions
to remain questions rather than becoming assumptions.

------------------------------------------------------------------------

## 1. Critical instruction to the next agent

Do **not** continue the existing design by assuming the newest document
is correct.

The project has accumulated contradictory design guidance. Before
recommending schematic, PCB, mechanical, firmware, or safety changes:

1.  Audit the relevant architecture from first principles.
2.  Verify claims against current manufacturer datasheets and
    authoritative engineering references.
3.  Clearly distinguish:
    -   **CONFIRMED REQUIREMENT**
    -   **CURRENT CANDIDATE**
    -   **PROPOSED CHANGE**
    -   **UNRESOLVED**
    -   **SUPERSEDED**
4.  Ask the user when a decision depends on preference or an unstated
    requirement.
5.  Do not silently fill gaps.
6.  For PCB guidance, study the applicable mixed-signal, precision-ADC,
    motor-driver, power-conversion, EMC, grounding, creepage/clearance,
    DFM/DFA, and Altium implementation practices before recommending
    layout or schematic changes.
7.  Do not treat ERC/DRC success as proof that a circuit is electrically
    correct.
8.  Prefer manufacturer reference designs and datasheets over generic
    example circuits.
9.  The user is a hobbyist and wants a design that can realistically be
    fabricated and assembled without expensive custom precision
    machining.
10. No PCB should be released for manufacture until the unresolved items
    and release blockers are explicitly reviewed.

------------------------------------------------------------------------

## 2. Product objective

Build a hobbyist-fabricable electromagnetic force compensation (EMFC)
precision balance integrated with a three-stage automated
smokeless-powder dispensing system.

The product is primarily a:

-   precision scale,
-   automatic powder dispenser,
-   coarse/medium/fine powder trickler system,
-   standalone embedded instrument.

It is **not currently intended as a general robotic material-handling
platform**.

Because the machine handles combustible/energetic reloading powder,
safety must be an explicit system requirement rather than an
afterthought.

------------------------------------------------------------------------

## 3. Confirmed metrology requirements

### 3.1 Capacity

**CONFIRMED REQUIREMENT: 520 g live capacity.**

Earlier 50 g and 250 g designs are historical/superseded capacity
targets.

The Torbal AD-520 has been used as a conceptual/commercial reference and
is the preferred reference point for defining reasonable metrological
performance.

### 3.2 Resolution / displayed increment

**CONFIRMED REQUIREMENT: 0.01 grain.**

Conversion:

-   1 grain = 64.79891 mg
-   0.01 grain = 0.6479891 mg

Important: the phrase "one-tenth of a grain" was also used, but that
would equal 6.479891 mg. The user simultaneously specified 0.647989 mg
and has previously specified 0.01 grain. Therefore this handoff treats
**0.01 grain / \~0.648 mg** as the intended requirement.

**NEXT AGENT MUST CONFIRM THIS BEFORE FREEZING THE SPECIFICATION.**

### 3.3 Accuracy

User target:

**approximately ±0.5 mg if realistically achievable.**

This is a desired performance target, not yet a demonstrated capability.

### 3.4 Repeatability, linearity, eccentric-load error, stabilization, drift

**UNRESOLVED.**

User preference: use the Torbal AD-520 / relevant Torbal 520 g EMFC
balance specification as the initial benchmark rather than inventing
arbitrary values.

The next agent should retrieve the authoritative Torbal specifications
and create a metrology requirements table covering at least:

-   capacity,
-   readability,
-   repeatability,
-   linearity,
-   absolute accuracy after calibration,
-   eccentric-load/corner-load behavior,
-   stabilization time,
-   zero drift,
-   span temperature coefficient,
-   zero temperature coefficient,
-   warm-up requirement,
-   overload capacity,
-   calibration behavior.

Do not equate displayed resolution with accuracy.

------------------------------------------------------------------------

## 4. EMFC mechanical architecture

### Status: UNRESOLVED / HIGH-PRIORITY AUDIT

Earlier design work proposed approximately:

-   25:1 mechanical force reduction,
-   \~2 N/A voice-coil force constant,
-   \~50--60 mA full-load restoring current,
-   flexure-based mechanics,
-   dimensions ultimately derived/validated using FEA.

These values are **preliminary**, not frozen.

### User constraint

The user is concerned that the proposed EMFC cell may have become too
dependent on precision-machined parts and sophisticated FEA.

The preferred design should be realistically manufacturable by a
hobbyist using, where practical:

-   3D printing,
-   laser/waterjet-cut sheet parts,
-   PCB fabrication,
-   commercially available bearings/flexures/springs/magnets/fasteners,
-   simple drilling/turning operations,
-   inexpensive outsourced flat parts,

rather than requiring expensive custom precision machining.

### Required next-agent investigation

Before committing to the existing flexure/lever geometry:

1.  Study commercial EMFC/EMFR balance architectures.
2.  Determine which mechanical arrangements are feasible at 520 g and
    \~0.65 mg readability.
3.  Evaluate whether a monolithic machined flexure is actually
    necessary.
4.  Evaluate simpler compliant mechanisms, crossed-flexure pivots,
    leaf-spring arrangements, purchased flexures, or other
    hobbyist-fabricable architectures.
5.  Establish sensitivity to:
    -   friction,
    -   hysteresis,
    -   creep,
    -   thermal expansion,
    -   off-axis loading,
    -   parasitic stiffness,
    -   assembly tolerances,
    -   magnet/coil alignment.
6.  Explain exactly what FEA is needed and what can instead be validated
    empirically.
7.  Design around prototyping and adjustability wherever possible.

The goal is not to eliminate analysis; it is to avoid unnecessary
manufacturing complexity.

------------------------------------------------------------------------

## 5. EMFC sensing and restoration loop

### Optical sensor

**CURRENT CANDIDATE: TT Electronics OPR2100.**

This supersedes the earlier Hamamatsu S3096-02 proposal.

Existing design work uses the OPR2100 center cells C/D as the
differential optical position detector.

The component should remain provisional until the planned sourcing/BOM
audit.

### Existing analog architecture

Existing documentation implements approximately:

OPR2100 differential position sensing\
→ low-noise transimpedance amplifiers\
→ differential error stage\
→ analog PI controller\
→ OPA593 power amplifier\
→ voice coil\
→ mechanical null restoration

The ADS1262 measures restoring current through a precision Kelvin shunt.

The purpose of the restoration servo is to keep the weighing mechanism
near a fixed null position. Rather than allowing pan displacement to
represent mass directly, the system applies electromagnetic force until
the optical position error approaches zero. The required restoring
force/current then represents the applied mass.

Existing PI values are bring-up values only and were never
production-validated.

### Requested architecture investigation: STM32 digital servo

**PROPOSED CHANGE --- NOT YET APPROVED AS TECHNICALLY SUPERIOR.**

The user would prefer to investigate moving the position-restoration
control loop into the STM32 rather than relying on a largely analog PI
loop.

Do not assume the current STM32 can directly replace the analog loop
merely because it has ADC/DAC/timer peripherals.

The next agent must evaluate:

-   optical sensor front-end bandwidth/noise,
-   ADC architecture and sampling latency,
-   whether the ADS1262 is appropriate for the fast position-control
    loop or should remain only the precision force/current measurement
    ADC,
-   STM32 internal ADC suitability for position feedback,
-   STM32 DAC or PWM + reconstruction versus an external precision DAC,
-   loop update frequency,
-   deterministic latency/jitter,
-   plant resonance,
-   control-loop bandwidth,
-   PID/PI/state-space options,
-   anti-windup,
-   startup and null acquisition,
-   coil-current limits,
-   fault behavior if firmware crashes,
-   watchdog behavior,
-   whether an analog inner loop plus digital supervisory/outer loop is
    preferable.

A hybrid architecture may be better than either a fully analog or fully
digital solution. Audit rather than assume.

### MCU

**CURRENT CANDIDATE: STM32G491RET6.**

Retain unless the architecture audit demonstrates a meaningful reason to
change it.

------------------------------------------------------------------------

## 6. Precision measurement electronics

Current candidate architecture includes:

-   ADS1262 precision ADC,
-   precision four-terminal current shunt,
-   low-noise reference,
-   low-noise analog supply,
-   OPA2388-class precision amplifiers,
-   OPA593-class coil power amplifier,
-   OPR2100 optical sensor,
-   temperature sensing around precision/thermal-critical areas.

These are **candidates**, not permission to copy the existing schematic
uncritically.

The next agent should audit:

-   noise budget,
-   required ENOB,
-   shunt value and dissipation,
-   reference noise/drift,
-   amplifier offset/noise/bias current,
-   ADC input/filter topology,
-   thermal gradients,
-   Kelvin routing,
-   analog/digital ground strategy,
-   coil-current measurement range,
-   anti-aliasing,
-   calibration,
-   PCB leakage/contamination effects,
-   achievable system-level uncertainty.

------------------------------------------------------------------------

## 7. Powder dispensing

### Confirmed architecture

**Three trickler channels**, not four.

Functions:

1.  **Coarse**
2.  **Medium**
3.  **Fine**

Only **one motor operates at a time**.

Earlier four-channel/four-auger references are superseded.

### Motor drivers

**CURRENT CANDIDATE:** three bare TMC2209-LA devices, assuming the audit
confirms they remain appropriate.

The previous use of plug-in BTT TMC2209 modules is superseded.

The schematic should be revised from four channels to three unless a
fourth unpopulated expansion channel is deliberately justified and
approved.

### Dosing algorithm

Still requires detailed firmware development.

Expected concept:

-   coarse dispense rapidly,
-   transition to medium as target approaches,
-   fine/trickle to final mass,
-   continuously account for material in flight and settling,
-   prevent simultaneous motor operation,
-   use stable-weight qualification before final acceptance.

Exact thresholds and predictive control are unresolved.

------------------------------------------------------------------------

## 8. HMI and communications

### HMI

**CONFIRMED:** Nextion NX8048T050 5-inch display over UART remains the
intended interface, subject to sourcing audit.

### Communications

The design should retain options for:

-   USB-C service/device connectivity,
-   CAN,
-   RS-485.

CAN and RS-485 need not dominate the design, but the hardware/interface
option should remain available.

Exact transceivers are current candidates:

-   TCAN1044A-family CAN,
-   THVD2450-family RS-485.

These supersede the older TCAN1042H and THVD1450 selections, subject to
the sourcing/BOM audit.

------------------------------------------------------------------------

## 9. Battery and power

### Battery

**CONFIRMED REQUIREMENT:** battery operation remains desired.

**UNRESOLVED:**

-   exact 2S Li-ion pack,
-   capacity,
-   BMS/protection,
-   connector,
-   charging architecture,
-   required runtime.

Earlier \~70--75 Wh / 2--4 hour estimates are preliminary.

### Charger

**CURRENT CANDIDATE:** BQ25792, superseding MAX77960B.

Must be re-audited for complexity, cost, availability, assembly
feasibility, and actual system requirements.

### Mains architecture

**UNRESOLVED / MUST BE AUDITED.**

Existing documentation has proposed:

-   IEC C14 inlet,
-   fuse,
-   double-pole master switch,
-   protective-earth chassis bond,
-   isolated Mean Well IRM modules,
-   segregation of mains from SELV electronics.

A later guide treats the `MainsPSU` PCB as a low-voltage/SELV power-path
PCB and moves mains components to a separate enclosure/harness assembly.

The user explicitly wants the next agent to audit this architecture
rather than accept it automatically.

Safety and code-compliant isolation take priority over PCB-count
convenience.

------------------------------------------------------------------------

## 10. PCB architecture

### Status: UNRESOLVED / AUDIT REQUIRED

Existing documentation conflicts.

One document describes a two-board architecture.

A later hierarchy guide describes:

1.  low-voltage power-path PCB,
2.  controller/motor PCB,
3.  precision sensor PCB,
4.  separate mains/enclosure wiring assembly.

The user's current Altium work already contains approximately:

-   22 schematic documents,
-   3 PCB documents.

Do **not** destroy/restructure this work until the architecture has been
reviewed.

### Next-agent PCB requirement

The current guide is considered confusing and insufficiently useful.

Before giving implementation instructions, the next agent should deeply
review:

-   hierarchical Altium project organization,
-   one-project-per-PCB versus multi-board project organization,
-   mixed-signal partitioning,
-   precision analog layout,
-   ADC/reference placement,
-   motor-driver current loops,
-   switching-regulator layout,
-   return-current control,
-   ground planes and ground partitioning,
-   shielding,
-   connector pinout/return pairing,
-   ESD/EFT/surge protection,
-   CAN/RS-485 physical-layer layout,
-   USB routing,
-   thermal management,
-   creepage/clearance,
-   PCB assembly constraints,
-   JLCPCB/PCBWay capabilities,
-   test points and bring-up access,
-   DFM/DFA,
-   component library/symbol/footprint verification.

Guidance should be incremental and should explain **why** each design
decision is made.

Where diagrams are provided, they should be electrically meaningful
engineering diagrams rather than decorative block diagrams.

------------------------------------------------------------------------

## 11. Current component-change history

Treat these as the current direction, subject to the upcoming full
sourcing audit:

  Older choice                  Current candidate
  ----------------------------- -------------------
  BTT TMC2209 plug-in modules   Bare TMC2209-LA
  MAX77960B                     BQ25792
  TCAN1042H                     TCAN1044A-family
  THVD1450                      THVD2450-family
  Hamamatsu S3096-02            OPR2100

Additionally:

-   Four motor channels → **three motor channels**
-   250 g capacity → **520 g capacity**
-   Analog-only restoration loop → **investigate STM32-controlled
    digital/hybrid restoration**
-   50 g historical concept → **superseded**

------------------------------------------------------------------------

## 12. Required BOM/sourcing audit

This is a major future work item.

Audit **every active component**, not only the ICs.

Optimization priorities:

1.  Electrical suitability
2.  Safety/reliability
3.  Long-term availability
4.  Low cost
5.  Availability from as few distributors as practical
6.  JLCPCB assembly availability
7.  PCBWay assembly availability
8.  Easy hand sourcing from Digi-Key/Mouser or equivalent mainstream
    authorized distributors
9.  Avoid obsolete/NRND/single-source components when a good alternative
    exists
10. Avoid exotic packages unless their performance is actually necessary

For each component record:

-   manufacturer,
-   exact MPN,
-   lifecycle,
-   unit cost at prototype quantity,
-   cost at small production quantity,
-   Digi-Key/Mouser availability,
-   JLCPCB availability/part number if applicable,
-   PCBWay availability if determinable,
-   package,
-   assembly difficulty,
-   approved alternates,
-   reason for selection,
-   whether substitution changes firmware/schematic/layout.

Do not optimize pennies out of the precision analog chain if it
materially compromises metrology.

------------------------------------------------------------------------

## 13. Calibration

**CONFIRMED:** retain both:

1.  manual calibration using certified external masses,
2.  automatic internal check-mass/calibration mechanism.

The exact internal mass, actuator, linkage, repeatability, and
calibration procedure remain unresolved.

The next agent should distinguish between:

-   a traceable external calibration,
-   an internal span/check mechanism,
-   zeroing/taring,
-   temperature compensation.

An internal check mass should not automatically be described as
traceable calibration unless the design and procedure support that
claim.

------------------------------------------------------------------------

## 14. Safety requirements

The instrument may dispense combustible/energetic smokeless powder.

Safety is therefore a system-level requirement.

The design review must consider at minimum:

-   hardware E-stop,
-   independent motor disable,
-   coil-current limiting,
-   watchdog/fail-safe states,
-   power-on state,
-   faulted-MCU state,
-   battery protection,
-   charger faults,
-   mains isolation,
-   protective earth,
-   fuse strategy,
-   static control,
-   bonding,
-   ignition-source segregation,
-   motor/driver temperature,
-   hot components,
-   short-circuit protection,
-   connector miswiring,
-   reverse polarity,
-   powder ingress,
-   dust accumulation,
-   enclosure materials,
-   emergency shutdown behavior,
-   fault logging,
-   safe recovery.

Do not claim hazardous-location compliance without the required formal
engineering/certification.

------------------------------------------------------------------------

## 15. Current Altium implementation state

The project has already been started in Altium.

Existing documentation refers to approximately 22 SchDocs and 3 PcbDocs.

However, the user finds the existing implementation guide confusing and
not sufficiently useful.

Therefore:

-   preserve existing work,
-   audit before editing,
-   map each existing sheet to its actual function,
-   identify obsolete sheets/nets/components,
-   identify changes caused by the 520 g / three-trickler / digital-loop
    direction,
-   propose a migration plan before major restructuring.

Do not instruct the user to recreate the entire project unless there is
a compelling technical reason.

------------------------------------------------------------------------

## 16. Major unresolved decisions

The next agent should explicitly work through these:

### A. Metrology

-   Confirm 0.01 grain versus 0.1 grain wording.
-   Obtain authoritative Torbal AD-520 performance specifications.
-   Freeze repeatability, linearity, eccentricity, drift, stabilization,
    and calibration targets.
-   Determine whether ±0.5 mg absolute accuracy is realistic over 0--520
    g.

### B. EMFC mechanics

-   Select a DIY-fabricable force-cell architecture.
-   Determine required mechanical reduction.
-   Select coil/magnet geometry.
-   Determine actual force constant.
-   Determine whether FEA is required and to what fidelity.
-   Design adjustability and prototype validation procedures.

### C. Control loop

-   Compare analog, digital, and hybrid null-restoration architectures.
-   Verify STM32G491 peripheral suitability.
-   Determine required control ADC/DAC path and loop rate.
-   Retain independent hardware protection.

### D. Precision electronics

-   Recalculate the full noise/error budget for 520 g.
-   Verify ADC, reference, shunt, amplifiers, and thermal architecture.

### E. PCB partition

-   Audit two-board versus three-board architecture.
-   Audit whether a separate low-voltage power-path PCB is justified.
-   Audit mains/SELV partitioning.

### F. Power

-   Select battery/BMS/connector.
-   Determine actual runtime requirement and power budget.
-   Audit BQ25792 and all DC/DC stages.
-   Audit Mean Well supply arrangement.

### G. Dispenser

-   Convert all four-channel assumptions to three channels.
-   Define coarse/medium/fine mechanics and control strategy.
-   Establish motor/current requirements.

### H. Calibration

-   Select internal check-mass mechanism.
-   Define certified external calibration procedure.

### I. Sourcing

-   Perform full BOM availability/cost/assembly audit.

### J. Safety

-   Perform formal system hazard/FMEA-style review before manufacture.

------------------------------------------------------------------------

## 17. Suggested order of work for the next agent

Do not start by drawing new schematics.

Recommended sequence:

1.  **Requirements audit**
2.  **Torbal benchmark/specification research**
3.  **EMFC mechanical feasibility study**
4.  **Analog vs digital vs hybrid servo architecture study**
5.  **System-level error/noise budget**
6.  **System power budget**
7.  **PCB partition/architecture audit**
8.  **Full BOM and sourcing audit**
9.  **Safety/FMEA review**
10. **Update system block diagram**
11. **Update schematic hierarchy**
12. **Review each schematic sheet individually**
13. **Only then proceed to PCB placement/routing guidance**
14. Prototype and characterize the EMFC mechanics/servo before treating
    compensation values as final.

------------------------------------------------------------------------

## 18. Rules for future design discussions

The next agent should use the following vocabulary consistently:

-   **CONFIRMED** --- explicitly approved by user.
-   **CANDIDATE** --- current preferred solution, still subject to
    engineering validation.
-   **PROPOSED** --- an idea being investigated.
-   **UNRESOLVED** --- no decision yet.
-   **SUPERSEDED** --- historical information that must not drive the
    new design.
-   **RELEASE BLOCKER** --- must be resolved before manufacturing
    release.

When a contradiction is discovered, show both versions and ask the user
or resolve it through engineering evidence. Never silently choose one.

------------------------------------------------------------------------

## 19. Current baseline in one paragraph

The current project is a **520 g EMFC precision balance with
approximately 0.01 grain (0.647989 mg) displayed resolution and a
desired \~±0.5 mg accuracy target**, integrated with **three sequential
coarse/medium/fine powder tricklers**, a **Nextion NX8048T050 HMI**,
battery and mains operation, USB-C plus optional CAN/RS-485, external
and internal calibration capability, and safety provisions appropriate
to handling energetic reloading powder. OPR2100, STM32G491RET6, ADS1262,
bare TMC2209-LA drivers, BQ25792, TCAN1044A, and THVD2450 are current
component candidates. The EMFC mechanical architecture, control-loop
architecture, PCB partition, mains/power architecture, exact battery,
metrological error budget, and several precision components remain
subject to engineering audit. Existing Altium work should be preserved
and audited rather than blindly followed or discarded.
