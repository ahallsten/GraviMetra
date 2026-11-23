# Power budget, IRM derating and heat map (Phase 2, OI-09)

Rev 0 · 2026-11-22 · Numbers: `analysis/power_budget.py` → `analysis/power_budget.out.txt` (section numbers in brackets, e.g. [§3], point to it). Everything here is **PROPOSED for Gate B** unless marked. Every unmeasured input is ASSUMED and listed in §11. Methods are standard power-electronics practice (outside methods), listed in the script header.

## 1. Bottom line

- **PWR-04 is met with room to spare.** Derated at an ASSUMED 45 °C module ambient (estimate: 39 °C in a 30 °C garage) and 108 VAC low line [§3]:

  | Module | Derated rating | Worst load (all at max, simultaneous) | Margin | 25 % margin holds up to |
  |---|---|---|---|---|
  | IRM-30-12 | 2.50 A | 1.34 A (coarse motor + check-mass motor at their ceilings, Config E) | **45 %** | 61 °C ambient |
  | IRM-20-5 | 3.81 A (0.953 at 108 VAC) | 0.51 A (display 0.40 A worst) | **86 %** | 81 °C ambient |

- **Steppers draw power, not phase current.** At 0.5 A rms run current a trickler motor takes 2.6 W, i.e. **0.22 A at 12 V**. At the 0.96 A hardware ceiling it takes 11 W (0.93 A). Idle motors are disabled (ENN high through the decoder, D-041). Each idle TMC2209 still draws 7 mA (10 mA max) from 12 V [§1].
- **Fuses** (5×20 mm, time-lag, ceramic, 1500 A breaking, Bel 5HT class) [§4]:
  - inlet **T2.5A** in the C14 drawer;
  - IRM-30-12 **T1.25A**;
  - IRM-20-5 **T1A**.
- **eFuses (TPS25947)** [§5]:
  - motor branch: R_ILM **2.49 kΩ** → 1.34 A (1.21–1.47 A), C_dVdt 3.9 nF, C_ITIMER 12 nF, latch-off variant;
  - B4 branch: **6.65 kΩ** → 0.50 A, which is the part's floor.
- **B4 supply: change to a 6.0 V pre-regulator on B2 (option C).** It cuts the heat beside the cell from **0.76 to 0.32 W**, and halves the load-dependent part (91 → 46 mW from empty to 520 g) [§6, §9].
- **Gap in D-070.** The coil soft-off must also work after a mains dropout, but the IRM-30-12 holds 12 V for only **0.29–0.39 s** at our load, against ~1.5 s needed. Two fixes:
  - a 12 V-UV PWR_FAIL signal plus a **6.8 mF** hold-up capacitor;
  - or soft stops (≤ 2 kN/m), so a hard cut is survivable.

  See §8.
- **Heat by zone** (typ, W) [§9]. Config E column = option C.

  | Zone (SYSTEM_ARCHITECTURE §6) | Config S idle | S dosing peak / 30 s average | Config E idle | E dosing peak / 30 s average |
  |---|---|---|---|---|
  | B1 mains box (rear) | 0.87 | 1.56 / 1.41 | 1.00 | 1.67 / 1.53 |
  | B2 controller (rear) | 0.52 | 0.88 / 0.81 | 0.67 | 1.02 / 0.95 |
  | Trickler motors (frame) | 0 | 2.18 / 1.69 | 0 | 2.18 / 1.69 |
  | Display + B3 (front panel) | 1.12 | 1.12 / 1.12 | 1.12 | 1.12 / 1.12 |
  | B4 EMFC module (beside the cell) | — | — | 0.32 | 0.32 / 0.32 |
  | Cell (coil + heater + LED) | — | — | 0.043 | 0.043 / 0.043 |
  | **Total = AC input** | **2.5** | **5.7 / 5.0** | **3.2** | **6.4 / 5.6** |

- **USB-only service mode: 53 mA typical, 113 mA maximum** (the maximum includes the buzzer). Both are within the 500 mA USB 2.0 / Type-C Default limit [§7].
- **The display is the largest heat source near the cell.** It runs at 1.1 W, up to 2.0 W, and the user's shield is unmeasured, so measure it (OI-21).

## 2. Power tree with currents (option C; typical / maximum)

```
120 VAC ─ C14 + T2.5A ─┬─ T1.25A ─ IRM-30-12 ── 12 V  0.06/0.10 A idle · 0.28/1.02 A coarse · 1.30 A worst
                       │      ├─ eFuse M (EN = SAFE) ── 12V_MOT ─ 4× TMC2209 (3 trickler + check-mass)  0.24/0.96 A
                       │      └─ eFuse E (UVLO) ─ 6.8 mF hold-up ─ TPS62170 → 6.0 V ── B4  0.06/0.10 A
                       │                                                ├─ LT3045 → 5V_A 56/92 mA
                       │                                                └─ LDO → 3V3_D 2/2 mA
                       └─ T1A ─── IRM-20-5 ── 5 V  0.28/0.51 A
                              ├─ B3: shield 5 V + own 3.3 V regulator  0.22/0.40 A
                              └─ TPS2121 (5 V ⟷ USB VBUS) ── 5V_SYS 0.05/0.11 A ── LDO → 3V3 51/82 mA (MCU 48/63)
```

Option A (as drawn in SYSTEM_ARCHITECTURE §3) feeds B4 from 12 V and keeps the check-mass driver on B4. In option A, B4 draws 0.067/0.102 A at 12 V, rising to 0.20/0.38 A during a check-mass lift.

## 3. Loads per configuration and state [§2]

The maximum column has every IC at its maximum, the motors at their hardware ceilings, the buzzer on, the coil at +120 % FS, the TC16 heater power, a 30 mA LED and a 400 mA display.

| AC input, W (typical / maximum) | Idle | Coarse running | Medium | Fine | Check-mass lift | 520 g on the pan | Worst simultaneous |
|---|---|---|---|---|---|---|---|
| Config S | 2.5 / 4.3 | 5.7 / 17.4 | 5.6 / 17.1 | 5.5 / 17.0 | — | — | 5.7 / 17.4 |
| Config E, option A | 3.5 / 5.8 | 6.7 / 18.8 | 6.6 / 18.5 | 6.5 / 18.4 | 5.5 / 9.9 | 3.6 / 6.0 | 8.6 / 22.6 |
| Config E, option C | 3.2 / 5.3 | 6.4 / 18.3 | 6.2 / 18.0 | 6.2 / 18.0 | 5.1 / 9.3 | 3.2 / 5.4 | 8.2 / 22.1 |

- **The B2 3V3 rail draws 51–53 mA typical and 82 mA maximum.** The MCU is 48.2 / 62.7 mA: DS13122 Table 21 at 170 MHz plus 21.7 mA of peripherals from Table 34. The MAX3232E draws 2.5 mA with a balance attached and 8.2 mA maximum.
- **The B4 5V_A rail draws 56 mA typical and 92 mA maximum.** The largest loads:
  - LED 20 mA;
  - TIAs and line drivers, 15 mA;
  - LTC6655, 5 mA;
  - ADS1263, 4.3 mA;
  - coil |I| up to 12.5 mA.

## 4. Steppers [§1]

The TMC2209 runs at 12 V. Supply power is 2·I_rms²·(R_wind + R_on,HS + R_on,LS + R_sense) plus switching, iron, mechanical and quiescent power. The copper term is exact for sine/cosine microstepping (checked in the script).

| Motor (ASSUMED part) | Sense resistor (PROPOSED) | Ceiling (DS formula) | Run current (PROPOSED) | Supply, typical | Supply at the ceiling | Heat location |
|---|---|---|---|---|---|---|
| Trickler: 17HS08-1004S class (1.0 A, 3.7 Ω) | 0.22 Ω | 0.96 A rms | 0.50 A rms | 2.6 W = **0.22 A** | 11.1 W = 0.93 A | Motor 2.2 W in the frame, driver 0.44 W on B2 |
| Check-mass: NEMA 8 + gearbox (0.6 A, 10 Ω) | 0.68 Ω | 0.33 A rms | 0.25 A rms | 1.6 W = 0.14 A | 3.4 W = 0.28 A | **Motor 1.4 W in the cell zone** while moving |

- **Heat scales with run current squared:** 0.3 A → 1.1 W, 0.5 A → 2.6 W, 0.7 A → 4.9 W. The trickler needs 1.25 mN·m against ~130 mN·m of holding torque, so set IRUN at the lowest reliable value found on the rig.
  - Why: the motor heat sits between the rear electronics and the cell, and it switches on and off with every throw.
- **The check-mass sense resistor caps the current at 0.33 A, below the motor's 0.6 A rating.**
  - Why: at 0.6 A the motor would dissipate 8–9 W inside the cell zone.
- **A span check puts 5.5 J into the cell zone** (11.6 J maximum), just before the span reading. That is 4 s of motion (ASSUMED) at 1.4 W.

## 5. Modules: derating, hold-up, inrush, fuses [§3, §4]

Datasheets checked on meanwell.com on 2026-11-22. They are byte-identical to the local copies: IRM-30-SPEC 2026-04-03 and IRM-20-SPEC 2025-11-21.

| | IRM-30-12 | IRM-20-5 |
|---|---|---|
| Ambient derating [DS] | 100 % to 50 °C, then −2.5 %/K to 50 % at 70 °C | 100 % to 45 °C (3.3/5 V curve), then −2 %/K to 50 % at 70 °C |
| Input derating [DS] | 100 % at ≥ 100 VAC (80 % at 85 VAC) | 100 % at ≥ 115 VAC; **95.3 % at 108 VAC** |
| Efficiency [DS], 230 VAC, full load | 88 % | 79 % |
| Our load | 1–11 % (0.25–3.3 W) → 0.2–1.0 W loss (ASSUMED curve, §11) | ~7 % (1.4 W) → ~0.7 W loss |
| Hold-up | [DS] 12 ms at full load, 115 VAC. At our load (scaled by stored energy): 0.29 s (E idle, option A) / 0.39 s (option C) / 0.11 s while dosing | [DS] 8 ms at full load. At our load: **~0.10 s**, so the 5 V rail (MCU, SAFE) drops first on a mains loss: good, the motors stop first |
| Cold-start inrush [DS] | 25 A at 115 VAC → 26 A at 120 VAC. I²t 0.10 A²s (C_bulk 47 µF ASSUMED) | 20 A → 21 A; 0.06 A²s (33 µF ASSUMED) |
| Fuse sizing rule | full-load input at 108 VAC = 0.80 A; ÷ (0.75 × 0.95) → I_n ≥ 1.12 A; melting I²t ≥ 3 × 0.10 / 0.22 = 1.4 A²s | 0.64 A → I_n ≥ 0.90 A; I²t ≥ 0.8 A²s |
| **Fuse** | **T1.25A**, 250 V ceramic, 3.0 A²s | **T1A**, 1.7 A²s |

- **Inlet fuse: T2.5A** (17 A²s). It needs I_n ≥ 2.02 A, and melting I²t ≥ 4.3 A²s for both modules starting together.
- **Selectivity:** the inlet-to-module I²t ratio is ≥ 5.7, so a module fault usually clears its own fuse.
- **Why ceramic with 1500 A breaking:** a US 15/20 A branch circuit can deliver far more fault current than the 35 A breaking capacity of a glass fuse. The glass Bel 5ST series is also EOL.
- **Cross-check:** the datasheet's 0.75 A at 115 VAC implies PF 0.40, which is plausible for a capacitor-input supply without PFC.
- **PWR-02:** both modules accept 85–305 VAC at 47–440 Hz. With 250 V fuses the unit is good for 100–240 VAC. It is not good for 277 VAC.
- **Enclosure air-rise estimate (U ASSUMED):** the enclosure rises +3.4 K at 8.6 W, and the mains box another +5.9 K at 2.0 W. That gives ~39 °C at the modules, against the 45 °C design value. The modules need no vents.

## 6. eFuse settings (TPS25947, SLVSFC9C May 2026) [§5]

R_ILM = 3334 / I_LIM, over 549–6650 Ω; accuracy ±10 % above 1 A, ±15 % at 0.5–1 A. Fast trip is at 2 × I_LIM. C_dVdt (pF) = 2000 / SR (V/ms).

| Branch | Max load | R_ILM | I_LIM (tolerance band) | C_dVdt (inrush) | Variant and enable |
|---|---|---|---|---|---|
| **Motor** (4 drivers, check-mass on decoder code 3) | 0.96 A | **2.49 kΩ** | 1.34 A (1.21–1.47) | 3.9 nF for 400 µF of bulk → 0.2 A, 24 ms | **TPS259474L** (circuit breaker, latch-off); EN = SAFE_CHAIN_OK; **FLT → SAFE latch reset**; C_ITIMER 12 nF ≈ 10 ms blanking |
| Motor, if check-mass is not interlocked | 1.23 A | 1.91 kΩ | 1.75 A (1.57–1.92) | same | same |
| **B4, option C** (feeds 6.8 mF + pre-regulator) | 0.06 A | **6.65 kΩ** | 0.50 A (floor) | **150 nF + 100 Ω** → ~0.09 A, ~0.9 s ramp | TPS259474A; EN from its own UVLO divider |
| B4, option A (B4 + check-mass motor at 12 V) | 0.38 A | 5.90 kΩ | 0.57 A (0.48–0.65) | ~1 nF | same |

- **The motor eFuse is a fault limit, not a one-motor interlock.** A failed decoder running three motors at their ceiling draws 2.8 A and trips it. Three motors at the typical current (0.66 A) do not trip it. The decoder enforces one motor at a time (D-041).
- **Why latch-off with FLT into the SAFE latch:** after a motor-rail fault the user has to press ARM before anything moves again (SAF-01 spirit).
- **Why the B4 eFuse enable comes from UVLO, not logic:** B4 has to stay powered while the 5 V domain resets (§8).

## 7. Linear regulators: is an intermediate buck warranted? [§6]

| B4 supply option | LDO loss on B4, typ / max | Total B4 heat (idle) | Load-dependent B4 heat, empty → 520 g | Cost |
|---|---|---|---|---|
| A. LT3045 from 12 V on B4 (as drawn) | 0.43 / 0.76 W (Tj ~60 °C) | **0.76 W** (includes 84 mW of check-mass driver idle) | **+91 mW** | — |
| B. 57 Ω dropping resistor on B2 (check-mass driver stays on B4) | 0.23 W | 0.56 W (0.47 W if the check-mass driver also moves) | **+36 mW** (the resistor takes the extra drop) | 0.20 W in the resistor on B2; no switcher; hold-up 11.5 mF |
| **C. TPS62170 12 → 6.0 V on B2 + check-mass driver on B2** | **0.07 / 0.13 W** (Tj ~39 °C) | **0.32 W** | **+46 mW** | buck loss 0.06 W at the rear; 2.25 MHz ripple rejected by the LT3045 plus an LC at the B4 entry (rig check) |

- **Recommend C.** Why: it moves ~0.44 W of constant heat and half of the load-dependent heat away from the cell. The switcher goes on B2, which is already the noisy board. It also makes the hold-up capacitor 2.4× smaller (§8).
- **B is the fallback** if the rig shows any 2.25 MHz on the ADS1263 readings. B has the smallest load-dependent term (+36 mW), because the resistor absorbs the extra drop as the current rises. It pays for that with 0.15–0.24 W more constant heat on B4 than C, and a 2.4× larger hold-up capacitor.
- **The error budget decides whether +46 mW (C) is small enough.** If it isn't, two fixes are open:
  - add a small series resistor ahead of the C pre-regulator output;
  - CANDIDATE: a complementary current sink, so B4 draws constant current whatever the load. This is the electronics analogue of D-058.
- **B2 3V3:** an LDO from 5 V dissipates 0.09 / 0.15 W, so **no buck is needed**.
- **Display 3.3 V:** if the shield takes its backlight from the header 3.3 V pin (newer MCUFRIEND boards do, with no series resistor), an LDO would burn up to 0.68 W at the front panel. Use a 3.3 V buck on B3, which saves ~0.5 W, and **never feed it from the B2 MCU rail.**

## 8. Mains loss, USB-only mode, back-powering [§7, §8]

- **D-070 lists brown-out among the SAFE events that soft-off the coil, but the supplies don't hold up long enough.**
  - On a mains loss the 5 V rail falls after ~0.1 s, so SAFE drops and the motors stop. That part is good.
  - The 12 V rail falls after 0.29–0.39 s. The coil current, which still needs ~3τ = 1.5 s to ramp down, is then **cut hard at whatever load is on the pan**.
- **Proposed fix (option C):**
  - A 12 V-UV comparator on B2, ahead of eFuse E, raises **PWR_FAIL**. It goes to B4 and to an MCU EXTI.
  - On PWR_FAIL, B4 starts the soft-off in hardware and sheds the LED, the ADS1263 (PWDN) and the TIAs.
  - **6.8 mF / 16 V** sits after eFuse E (which blocks reverse current) and before the pre-regulator. The requirement is 4.8 mF over a 10.5 → 5.8 V window.
  - Without load shedding it would need 29 mF. Option A would need 12 mF (45 mF without shedding), and option B 11.5 mF.
- **Alternative:** elastomer stops ≤ 2 kN/m at the coil radius survive a hard cut (D-067). The OI-33 impact test decides which way to go.
- **USB-only service mode:** VBUS → TPS2121 → 3V3. The display, 12 V, motors and B4 stay dark.
  - It draws 53 mA typical and 113 mA maximum (buzzer included). The limits are 500 mA after configuration and 100 mA before.
  - Rule: no buzzer before enumeration.
  - The CC pins carry plain Rd resistors, so a higher Type-C current can't be detected and isn't used.
- **Back-powering rule (firmware plus hardware):** with V5_MAIN absent, the firmware holds the LCD bus (PE8–15, WR, RS, RD, CS) and every B4 line low or Hi-Z. The B4 interface buffers have Ioff.
  - Why: otherwise current flows into the unpowered 74HC245 and ADS1263 through their clamp diodes.

## 9. Heat map: inputs to the error budget [§9]

The full table, including option A and Config S, is in `power_budget.out.txt` §9. These are the terms the error budget needs:

| Source | Zone | Value (typ) | Time behaviour |
|---|---|---|---|
| Coil + heater (P0) | Cell | 18 mW (35 mW with TC16) | Constant by construction (D-065: heater = P0 − I²R every 10 ms). The script shows 43.0 mW in the cell at both an empty pan and 520 g |
| **Position-sensor LED** | Cell | **25 mW at 20 mA (ASSUMED)** | Constant DC. **As large as P0** and missing from the servo study's heat accounting → set its current in OI-31 |
| Check-mass lift | Cell | 5.5 J per span check (1.4 W for 4 s); 9 mW averaged at one check per 10 min | **Pulse just before the span reading** → needs a settle time, or the motor off the cell base |
| B4 module (option C / A) | Beside the cell | 0.32 / 0.76 W | Constant, except **+46 / +91 mW from empty to 520 g**. With an ASSUMED 10–20 K/W local coupling × ~2.5 ppm/K electronics span tempco, that is 1.1–2.3 ppm (C) or 2.3–4.6 ppm (A) at full load, a slow load-dependent bow against MET-04's 3.85 ppm |
| Display + B3 | Front panel | 1.12 W (up to 2.0 W) | Constant if brightness is fixed. **Don't use backlight timeouts during a session**: steps in heat are what drift |
| Trickler motors | Frame (rear of the tubes) | 2.2 W while running. 30 s cycle average 1.69 W: coarse 0.87 (40 % duty), medium 0.41, fine 0.40 | On/off every throw |
| B2 controller | Rear | 0.67 W idle, 1.02 W peak | Steps with the motor driver |
| B1 mains box | Rear | 1.0 W idle, 1.7 W peak | Follows the motor load |

Energy balance: the zone heats sum to the AC input in all 38 configuration/state/corner cases (PASS).

## 10. Contradictions and changes to log

1. **SYSTEM_ARCHITECTURE §2/§3 says B4 has "local LT3045-class LDOs from 12 V".** This study proposes a 6.0 V pre-regulator on B2 instead (option C, §7), so the B2–B4 cable carries 6 V and PWR_FAIL, not 12 V.
2. **SYSTEM_ARCHITECTURE §2 puts the "check-mass actuator driver" on B4.** This study proposes TMC2209 #4 on B2, on the SAFE-gated motor eFuse, as **decoder code 3**. D-041 currently says "code 3 selects no driver"; the replacement for "no driver" is EN_REQ low.
3. **SYSTEM_ARCHITECTURE §3 says the "display shield 5 V pin (its own 3.3 V regulator, per OI-21)".** In a 2018 Arduino-forum thread, the MCUFRIEND_kbv library author says newer MCUFRIEND boards have **no regulator** and feed the backlight from the header 3.3 V pin with no series resistor, and that some draw ~400 mA. The user's board is unknown, so B3 must supply its 3.3 V pin from its own regulator.
4. **D-070 (soft-off includes brown-out) can't be met on a mains loss with the modules alone** (§8). It needs PWR_FAIL plus hold-up, or soft stops.
5. **The archived Aug-2026 `Power_Budget.md` budgets 8–15 W per NEMA 17 and 5 W for a Nextion.** Both are superseded: 2.6 W typical at a 0.5 A run current (11 W only at the hardware ceiling), and 1.1–2.0 W for the MCUFRIEND display.
6. **SERVO_MEASURAND / EMFC_MECHANICS heat accounting:** the cell also receives the LED (~25 mW) and the check-mass motor pulse (~5.5 J per check). Neither is in those studies.
7. **PWR-02 ("universal input if the module datasheets allow"):** the modules allow 85–305 VAC, but the IRM-20-5 derates below 115 VAC and the fuses are rated 250 V. Proposed wording: 120 VAC primary, 100–240 VAC capable.

## 11. Assumptions to replace with measurements

| ASSUMED | Value used | Replace by |
|---|---|---|
| Trickler motor | 17HS08-1004S class, 3.7 Ω (sellers list 3.5 or 3.7), 1.0 A | Choosing the motor; Fluke 287 resistance |
| Run current, winding temperature, iron loss | 0.5 A rms, 40 °C, 0.15 W at 272 rpm | Rig: lowest reliable IRUN per stage, case temperature, supply current on the bench supply |
| Check-mass motor and duty | NEMA 8 + gearbox (0.6 A, 10 Ω); 4 s per check; one check per 10 min | OI-07 design; log the MET-07 trigger rate in the garage |
| Display | 1.12 W typical (LCDwiki MAR3501), 400 mA maximum (forum) | **Measure the user's shield:** 5 V and 3.3 V pin currents separately, regulator present or not (OI-21) |
| MCU peripherals | TIM2/8/15/20 and SPI1 taken equal to siblings | 3V3 current at bring-up |
| B4 analog | 4 TIA + 4 line-driver channels at 1.9 mA, LED 20 mA, bias 1 mA | B4 schematic (Phase 4); OI-31 for the LED |
| IRM efficiency vs load | No published curve; anchors at η_full −10/−5/−2 points at 10/25/50 % load, −1.5 points for 120 VAC | Wall plug-in power meter vs a known DC load |
| IRM bulk capacitance (inrush τ) | 47 / 33 µF (×3 design factor) | Power-cycle test (no nuisance blows); optional current-probe capture |
| Module ambient | 45 °C design (39 °C estimated: U 4–5 W/m²K, 200 mm enclosure height) | TMP117 next to B1 after a 2 h dosing soak |
| Fuse derating | 0.75 continuous × 0.95 at 45 °C; pulse factor 0.22 for 10⁵ pulses | Fuse temperature derating curve (5HT datasheet p.3) at selection |
| Pre-regulator efficiency | 0.85 typical / 0.80 maximum | TPS62170 datasheet curve at 6 V, 60–100 mA |
| Soft-off requirement | 3τ = 1.5 s of B4 power | OI-33 impact test |
| B4 thermal coupling | 10–20 K/W from the load-dependent heat to the reference/ADC | B4 TMP117 + IR camera, empty vs 520 g, 60 min each |
| Low line | 108 VAC (−10 %) | — (standard) |

## 12. Rig and bring-up checks

1. **Wall power per state** with a plug-in meter. Compare against §3: Config S idle 2.5 W, coarse running 5.7 W.
2. **12 V motor-branch current per stage and run current** on the bench supply ammeter, then find the lowest reliable IRUN. Check the motor case temperature after 30 min of back-to-back throws.
3. **Display current split** (5 V pin and 3.3 V pin) on the bench supply (OI-21).
4. **eFuse trip test:** load the motor branch through a power resistor above I_LIM. Confirm latch-off, the SAFE reset, and the ARM press needed to recover.
5. **Twenty power-cycles at the switch:** no nuisance fuse blows; scope 12 V and 5 V start-up order.
6. **USB-only mode:** USB power meter, before and after enumeration (≤ 100 / ≤ 500 mA).
7. **Mains pull at full load on the Config E rig:** the coil current decays over ≥ 1.5 s, and the stop impact is recorded (OI-33).
8. **B4 thermal map:** empty vs 520 g, 60 min each. Feeds the error budget's load-dependent span term.
9. **Ripple:** with option C, check the 2.25 MHz content on 5V_A and in the ADS1263 noise floor.

## Sources

| Part | Document | Used for |
|---|---|---|
| Mean Well IRM-30 / IRM-20 | IRM-30-SPEC 2026-04-03, IRM-20-SPEC 2025-11-21 (meanwell.com, checked 2026-11-22; identical to the local copies) | Ratings, derating curves, hold-up, inrush, AC current, efficiency, no-load power |
| Mean Well PCB installation manual | PCB_EN.pdf (2023-11-21) | Clearances; no fuse guidance given |
| TPS25947 | SLVSFC9C (May 2026), local | R_ILM, dV/dt and ITIMER equations, R_ON, I_Q, variants |
| TMC2209 | Rev 1.08 (2022-05-25), local | R_DSon, VS current, current-scaling formula, sense-resistor table |
| LT3045 | Rev D, local | Dropout, GND current, θJA |
| STM32G491 | DS13122 Rev 4, local | Tables 21 and 34 |
| ADS1263, LTC6655, OPA2388, TMP117, TSAL6200, TPS2121, TPS6217x | local copies (SBAS661C, —, SBOS777D, —, 81010 Rev 2.4, SLVSEA3F, SLVSAT8E) | Supply currents, LED VF, mux R_ON |
| MAX3232E | TI SLLS664E (June 2021), via TI's online datasheet | I_CC 0.3 typical / 1 mA maximum |
| Bel Fuse 5HT | Rev. 5HT June 2026 | Fuse I²t, cold resistance, 1500 A breaking |
| MCUFRIEND 3.5" shield | forum.arduino.cc thread 562286 (2018); LCDwiki "3.5inch Arduino Display-UNO" | Display current (ASSUMED until measured) |
| Steppers | StepperOnline listings: 17HS08-1004S; NEMA 8 + 19:1 planetary gearbox | Motor R and I (ASSUMED) |
| USB | USB 2.0 §7.2.1; USB Type-C Default current | 100 / 500 mA limits |
