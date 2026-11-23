"""Phase 2 (OI-09, PWR-04): power budget per rail, Mean Well IRM derating, fuses, eFuse settings,
linear-regulator dissipation, USB-only service mode, coil soft-off hold-up, and a heat map by zone that
feeds the thermal-gradient terms of the error budget.

Run:  .venv/Scripts/python analysis/power_budget.py > analysis/power_budget.out.txt

Status of inputs (labels per CLAUDE.md; every number below carries one):
  [DS]       datasheet value; part, revision and date are listed in docs/phase2/POWER_BUDGET.md, "Sources"
  CONFIRMED  project requirement or decision (ENV-01 15-30 C, DSP-06 30 s cycle, D-041 one motor at a time,
             D-065 coil/heater P0, D-070 coil soft-off on SAFE incl. brown-out, ...)
  PROPOSED   a design choice made here (run currents, sense resistors, eFuse resistors, B4 supply option C)
  ASSUMED    everything else. Each ASSUMED value is listed in the doc with the measurement that replaces it.

Methods (outside methods, standard power-electronics practice; none from the mechanical textbook):
  - Chopper-driven stepper: the supply sees power, not phase current.
        P_supply = 2*I_rms^2*(R_wind(T) + R_on,HS + R_on,LS + R_sense) + P_switching + P_iron + P_mech + V*I_q
    (for sine/cosine microstepping R*(I_A^2 + I_B^2) = 2*R*I_rms^2 at every microstep; checked below).
    I_supply = P_supply / V_motor.
  - LDO: P = (V_in - V_out)*I_out + V_in*I_q.  Buck: P_in = P_out/eta + V_in*I_q.  Load switch: I^2*R_on + V*I_q.
  - IRM derating: linear interpolation of the datasheet ambient and input-voltage curves.
  - IRM loss vs load: Mean Well publishes no efficiency-vs-load curve for IRM-20/30, so loss is interpolated
    between ASSUMED efficiency anchors (light load conservative) and the datasheet no-load power.
  - Hold-up: stored bulk-capacitor energy is fixed, so t_hold ~ t_hold,full * P_in,full / P_in.
  - Inrush: first-order RC, I2t = I_pk^2*tau/2, tau = (V_pk/I_pk)*C_bulk with C_bulk ASSUMED (not published).
    Fuse melting I2t must exceed pulse I2t / pulse factor (0.22 for 1e5 pulses, ASSUMED fuse-maker guidance).
  - Air temperature rise of a closed box: dT = P/(U*A), U ASSUMED.
Design factors: module margin >= 25 % against the derated rating at the -2.5 % output edge (PWR-04);
  fuse I_n >= I_in,full-load,low-line / (0.75 * k_T); eFuse ILIM,min >= 1.25 x branch maximum;
  inrush tau x3 (unknown bulk capacitance); soft-off hold-up 3*tau_RC.
Sanity checks print PASS/FAIL at the end.
"""
from __future__ import annotations

import math
from collections import defaultdict
from dataclasses import dataclass

import pint

u = pint.UnitRegistry()
out: list[str] = []
w = out.append
CHECKS: list[tuple[str, bool]] = []


def check(name: str, ok: bool) -> None:
    CHECKS.append((name, bool(ok)))


def interp(pts, x):
    """Piecewise-linear interpolation, clamped at the ends. pts = [(x, y), ...] ascending x."""
    if x <= pts[0][0]:
        return pts[0][1]
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        if x <= x1:
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    return pts[-1][1]


# =====================================================================================================
# 0. Inputs
# =====================================================================================================
V_AC_NOM = 120 * u.V        # PWR-02 (PROPOSED: 120 VAC 60 Hz)
V_AC_LOW = 108 * u.V        # ASSUMED low line (-10 %)
V_AC_DS = 115 * u.V         # [DS] IRM test point for hold-up, inrush and AC current
T_GARAGE_MAX = 30.0         # degC, ENV-01 CONFIRMED
T_INT_DESIGN = 45.0         # degC, ASSUMED local ambient of the B1 modules (checked against the box estimate)
ENCL = (0.450, 0.350, 0.200)  # m: footprint ENV-02 CONFIRMED; height ASSUMED
U_ENCL = 4.0                # W/(m2 K), ASSUMED overall wall coefficient, still air both sides + radiation
BOX = (0.160, 0.110, 0.060)   # m, ASSUMED size of the B1 metal mains compartment
U_BOX = 5.0                 # W/(m2 K), ASSUMED (bare metal box, still air)


@dataclass
class Module:
    name: str
    V: float           # V      [DS]
    I: float           # A      [DS]
    P: float           # W      [DS]
    eta_full: float    # -      [DS] typ, 230 VAC, full load, 25 C
    hold_full: float   # s      [DS] typ, 115 VAC, full load
    inrush: float      # A      [DS] typ cold start, 115 VAC
    i_ac_full: float   # A rms  [DS] typ, 115 VAC, full load
    p_nl: float        # W      [DS] no-load power (< 0.1 W)
    der_T: list        # [DS] (ambient C, % load)
    der_V: list        # [DS] (VAC, % load)
    c_bulk: float      # F      ASSUMED (not published; ~1.5 uF/W for a universal-input flyback)
    ovl: tuple         # [DS] overload trip window (x rated power), hiccup


IRM30 = Module("IRM-30-12", 12.0, 2.5, 30.0, 0.88, 12e-3, 25.0, 0.75, 0.1,
               [(-30, 100), (50, 100), (70, 50), (85, 20)], [(85, 80), (100, 100), (305, 100)],
               47e-6, (1.05, 1.60))
IRM20 = Module("IRM-20-5", 5.0, 4.0, 20.0, 0.79, 8e-3, 20.0, 0.60, 0.1,
               [(-30, 100), (45, 100), (70, 50), (80, 20), (85, 5)], [(85, 80), (115, 100), (305, 100)],
               33e-6, (1.15, 1.60))
MODULE_TOL = 0.025          # [DS] output voltage tolerance (both)
ETA_LOWLINE = 0.015         # ASSUMED efficiency penalty 230 -> 120 VAC
ETA_ANCH = [(0.10, -0.10), (0.25, -0.05), (0.50, -0.02), (1.00, 0.0)]  # ASSUMED (load fraction, d-eta)


def module_loss(mod: Module, p_out: float) -> float:
    eta_f = mod.eta_full - ETA_LOWLINE
    pts = [(0.0, mod.p_nl)] + [(x * mod.P, x * mod.P * (1 / (eta_f + d) - 1)) for x, d in ETA_ANCH]
    if p_out > mod.P:  # beyond rating: extrapolate with the full-load efficiency
        return p_out * (1 / eta_f - 1)
    return interp(pts, p_out)


# ---- rails (nominal V) ------------------------------------------------------------------------------
RAILV = {"AC": 120.0, "12V": 12.0, "12V_MOT": 12.0, "12V_B4": 12.0, "6V_B4": 6.0, "5V_A": 5.0,
         "3V3_D": 3.3, "5V_MAIN": 5.0, "5V_SYS": 5.0, "3V3": 3.3, "5V_DISP": 5.0, "VBUS": 5.0}

# ---- steppers -----------------------------------------------------------------------------------------
ALPHA_CU = 0.00393          # 1/K copper
# Trickler motor, ASSUMED representative small NEMA 17: StepperOnline 17HS08-1004S class
# (1.0 A, 3.5-3.7 ohm listed by two sellers -> 3.7 used, 4.5 mH, 13-16 N.cm). Part not chosen yet.
TRK_I_RATED, TRK_R20 = 1.0, 3.7
TRK_IRUN = {"typ": 0.50, "max": None}   # A rms; typ PROPOSED (50 % of rated: still >50x torque margin);
TRK_RSENSE = 0.22           # ohm PROPOSED -> hardware ceiling (CS=31, vsense=0) from [DS] formula below
VFS, RS_INT = 0.325, 0.020  # [DS] TMC2209 full-scale sense voltage and internal 20 mohm term


def i_ceiling(rs):          # [DS] TMC2209 Rev 1.08 sec. 9: I_rms = (CS+1)/32 * VFS/(Rs+20m) / sqrt2, CS = 31
    return VFS / (rs + RS_INT) / math.sqrt(2)


TRK_IRUN["max"] = i_ceiling(TRK_RSENSE)
# Check-mass motor, ASSUMED: NEMA 8 + planetary gearbox, StepperOnline 8HS15-0604S-PGxx class (0.6 A, 10 ohm, 5.5 mH)
CM_I_RATED, CM_R20 = 0.6, 10.0
CM_RSENSE = 0.68            # ohm PROPOSED -> 0.33 A ceiling (deliberately below the 0.6 A rating: heat in the cell)
CM_IRUN = {"typ": 0.25, "max": i_ceiling(CM_RSENSE)}   # typ PROPOSED
CM_T_MOVE = 4.0             # s per span check (lower + retract + lift), ASSUMED
CM_PERIOD = 600.0           # s between span checks, ASSUMED worst (3 K/h garage swing at the 0.5 K MET-07 trigger)
CM_IRON_MECH = {"typ": 0.03, "max": 0.06}   # W ASSUMED
# TMC2209 [DS Rev 1.08]: RDSon 0.17 typ / 0.21 max ohm (25 C) per FET; VS current 7 typ / 10 max mA (driver disabled),
# 7.5 mA operating (24 V; ASSUMED same at 12 V via the internal regulator); standby 0.16 / 0.30 mA.
RON = {"typ": 0.17 * 1.2, "max": 0.21 * 1.45}  # ohm per FET; x1.2 (Tj ~60 C) / x1.45 (Tj ~110 C) ASSUMED
IQ_DIS = {"typ": 7e-3, "max": 10e-3}
IQ_RUN = {"typ": 7.5e-3, "max": 10e-3}
T_WIND = {"typ": 40.0, "max": 100.0}            # degC winding, ASSUMED
F_CHOP = {"typ": 35e3, "max": 50e3}             # Hz, [DS] test condition / ASSUMED upper
T_SW = {"typ": 120e-9, "max": 300e-9}           # s rise+fall per edge pair, [DS] 60+60 ns at 0.7 A / ASSUMED
IRON_COARSE = {"typ": 0.15, "max": 0.30}        # W at 272 rpm, ASSUMED, scales with speed
MOTOR_RPM = {"coarse": 272.0, "medium": 86.0, "fine": 23.0}  # trickler_sizing.out.txt sec. 5 (upper band)
T_MOTOR = 1.25e-3           # N.m at the motor, trickler_sizing.out.txt sec. 5 (used for all stages, upper bound)
DOSE = {"coarse": 12.0, "medium": 6.0, "fine": 6.0}  # s, TRICKLER_CONCEPT sec. 2 (PROPOSED)
CYCLE = 30.0                # s, DSP-06 CONFIRMED (worst case: back-to-back throws)


def stepper(i_rms, r20, corner, rpm, v=12.0, iron_ref=None, iq=None):
    """Return dict of loss parts (W) for one running motor + its driver."""
    r = r20 * (1 + ALPHA_CU * (T_WIND[corner] - 20))
    i2 = 2 * i_rms ** 2
    iron = (iron_ref if iron_ref is not None else IRON_COARSE[corner] * rpm / MOTOR_RPM["coarse"])
    parts = {
        "copper": i2 * r,
        "iron": iron,
        "mech": T_MOTOR * rpm * 2 * math.pi / 60,
        "fets": i2 * 2 * RON[corner],
        "rsense": i2 * (TRK_RSENSE if iq is None else iq["rs"]),
        "switching": 2 * 0.5 * v * i_rms * math.sqrt(2) * T_SW[corner] * F_CHOP[corner],
        "quiescent": v * IQ_RUN[corner],
    }
    return parts


# ---- B2 controller (3V3 unless noted) ----------------------------------------------------------------
# STM32G491 [DS13122 Rev 4]: Table 21 Run, Range 1 boost, 170 MHz, all peripherals off: 26.5 typ (25 C),
# 33.0 max (85 C) mA. Table 34 peripheral currents (uA/MHz, boost); entries marked * are ASSUMED equal to a sibling.
PERIPH_UA_MHZ = {"bus matrix": 0.56, "DMA1": 3.16, "DMA2": 3.48, "DMAMUX": 6.73, "CRC": 0.90, "GPIOA-F": 6 * 0.60,
                 "ADC12": 6.30, "DAC1": 5.24, "TIM1": 11.26, "TIM8*": 11.26, "TIM20*": 11.26, "TIM2*": 8.30,
                 "TIM3": 8.30, "TIM15*": 8.30, "USART2": 3.93 + 7.56, "USART3": 3.55 + 7.76, "SPI1*": 3.0,
                 "I2C1": 6.35, "USB": 4.07, "SYSCFG/COMP": 1.64, "PWR": 0.88}
I_PERIPH = sum(PERIPH_UA_MHZ.values()) * 170 * 1e-6
I_MCU = {"typ": 26.5e-3 + I_PERIPH, "max": 33.0e-3 + I_PERIPH + 5e-3 + 3e-3}  # + LCD-bus toggling, VDDA (ASSUMED)
# MAX3232E [DS TI SLLS664E]: ICC 0.3 typ / 1 max mA no load; each loaded driver ~2 x V_out/R_L via the doubler.
I_RS232 = {"S_typ": 0.3e-3 + 2 * 5.4 / 5e3, "E_typ": 0.3e-3, "max": 1e-3 + 2 * 2 * 5.4 / 3e3}  # R_L 3-7 kohm
B2_3V3_MISC = {  # name: (typ A, max A, source)
    "TMP117 ambient": (3.5e-6, 5e-6, "[DS] 1 Hz cycle"),
    "TMC2209 VCC_IO (per driver)": (15e-6, 30e-6, "[DS]"),
    "safety logic, TPS3431 WDT, Hall lid switch": (0.1e-3, 0.5e-3, "ASSUMED"),
    "status LED": (2e-3, 5e-3, "ASSUMED"),
    "pull-ups, dividers, NTC": (0.3e-3, 1e-3, "ASSUMED"),
}
I_USB_PHY = {"typ": 2e-3, "max": 5e-3}      # ASSUMED, only while a host is connected
I_BUZZER = 30e-3                             # A at 5 V, ASSUMED magnetic transducer while sounding
LDO_B2_IQ = 0.1e-3                           # ASSUMED (part not chosen)
MUX_RON, MUX_IQ = 0.056, {"typ": 0.2e-3, "max": 0.3e-3}  # [DS] TPS2121 SLVSEA3F (max ASSUMED)
EF_RON = {"typ": 0.0283, "max": 0.045}       # ohm [DS] TPS25947 SLVSFC9C
EF_IQ = {"typ": 0.428e-3, "max": 0.610e-3}   # A [DS]
# ---- B3 display: MCUFRIEND 3.5" RM68140 shield (user's) ---------------------------------------------------
# ASSUMED: typ 1.12 W (LCDwiki MAR3501 3.5" UNO shield), max 400 mA at 5 V (forum.arduino.cc 562286, MCUFRIEND_kbv
# author: "some of these shields take 400 mA and get VERY warm"). Measure the user's shield (OI-21 extension).
I_DISP = {"typ": 1.12 / 5.0, "max": 0.40}

# ---- B4 EMFC module (Config E) -----------------------------------------------------------------------------
R_COIL = 114.0               # ohm force winding, D-065 / SERVO_MEASURAND sec. 2
I_FS = 10.4e-3               # A at 520 g, SERVO_MEASURAND sec. 2
I_EMPTY = 0.02 * I_FS        # +2 % FS at an empty pan (D-051 option U)
I_COIL_MAX = 12.5e-3         # +120 % FS
P0 = {"typ": 18e-3, "max": 35e-3}   # W constant coil+heater power (Recoma 28 / TC16), D-065 CONFIRMED design
ETA_HB = 0.95                # heater H-bridge efficiency, ASSUMED
OPA_IQ = {"typ": 1.9e-3, "max": 2.6e-3}   # A per channel [DS OPA2388 SBOS777D]
B4_5VA = {  # name: (typ A, max A, source)  -- coil, heater and LED handled separately
    "coil driver + return buffer (OPA2388, 2 ch)": (2 * 1.9e-3, 2 * 2.6e-3, "[DS]"),
    "heater H-bridge gate drive": (0.2e-3, 0.5e-3, "ASSUMED"),
    "HEAT_ISENSE amplifier": (0.2e-3, 0.5e-3, "ASSUMED"),
    "soft-off, current limit, COIL_FAULT": (0.5e-3, 1.0e-3, "ASSUMED"),
    "ADS1263 AVDD": (4.3e-3, 6.5e-3, "[DS] SBAS661C"),
    "LTC6655B-2.5": (5.0e-3, 7.0e-3, "[DS]"),
    "TIAs fine A/B + coarse A/B (4 ch)": (4 * 1.9e-3, 4 * 2.6e-3, "[DS] per ch, ASSUMED part"),
    "cross-board line drivers/receiver (4 ch)": (4 * 1.9e-3, 4 * 2.6e-3, "ASSUMED"),
    "LED current-source amplifier": (1.9e-3, 2.6e-3, "[DS] per ch"),
    "misc bias, reference buffer": (1.0e-3, 2.0e-3, "ASSUMED"),
}
I_LED = {"typ": 20e-3, "max": 30e-3}   # A DC, ASSUMED (TSAL6200 rated 100 mA; value set in OI-31)
VF_LED = {"typ": 1.25, "max": 1.35}    # V, ASSUMED from the TSAL6200 VF(IF) curve [DS Rev 2.4]
B4_3V3D = {
    "ADS1263 DVDD": (1.0e-3, 1.25e-3, "[DS]"),
    "TMP117 x2 (magnet, board)": (7e-6, 10e-6, "[DS]"),
    "interface buffers with Ioff": (0.5e-3, 1.0e-3, "ASSUMED"),
}
LT3045_IQ = {"typ": 2.2e-3, "max": 5.5e-3}   # A [DS] GND current (max = 50 mA dropout row, conservative)
LT3045_THJA = 34.0           # C/W [DS] DFN-10 (MSOP-12: 33)
LDO_3V3D_IQ = 0.1e-3         # ASSUMED small LDO
BUCK_ETA = {"typ": 0.85, "max": 0.80}   # TPS62170-class 12 -> 6.0 V at 50-100 mA, ASSUMED from the DS curves
BUCK_IQ = 17e-6              # [DS] TPS6217x SLVSAT8E
V_PREREG = 6.0               # V, option C pre-regulator output (TPS62170 max 6 V [DS])
TAU_SOFTOFF = 0.5            # s, D-070 / SERVO_MEASURAND sec. 5
T_HOLD_REQ = 3 * TAU_SOFTOFF  # s, ASSUMED requirement (current down to e^-3 = 5 %)

STATES_S = ["idle", "dispense_coarse", "dispense_medium", "dispense_fine", "worst"]
STATES_E = ["idle", "dispense_coarse", "dispense_medium", "dispense_fine", "cm_lift", "weigh_full", "worst"]


@dataclass
class Conv:
    name: str
    kind: str       # 'ldo' | 'buck' | 'switch' | 'module'
    inp: str
    outp: str
    zone: str
    iq: float = 0.0
    ron: float = 0.0
    eta: float = 1.0
    mod: Module | None = None


# =====================================================================================================
# Load model
# =====================================================================================================
def coil_current(state, corner):
    return {"idle": I_EMPTY, "dispense_coarse": I_EMPTY + 10 / 520 * I_FS,
            "dispense_medium": I_EMPTY + 10 / 520 * I_FS, "dispense_fine": I_EMPTY + 10 / 520 * I_FS,
            "cm_lift": I_EMPTY + 200 / 520 * I_FS, "weigh_full": I_FS if corner == "typ" else I_COIL_MAX,
            "worst": I_COIL_MAX}[state]


def build(cfg, state, corner, option="C", usb_only=False):
    """Return (loads, convs). loads = [(name, rail, I_A, {zone: W})]. Zones: B1 B2 TRK HMI B4 CELL."""
    L = []

    def add(name, rail, i, zone, split=None):
        p = RAILV[rail] * i
        h = defaultdict(float)
        for z, pz in (split or {}).items():
            h[z] += pz
        rem = p - sum(h.values())
        assert rem > -1e-12, (name, rem)
        h[zone] += rem
        L.append((name, rail, i, dict(h)))

    def addp(name, rail, parts_by_zone):
        p = sum(parts_by_zone.values())
        L.append((name, rail, p / RAILV[rail], dict(parts_by_zone)))

    c = corner
    worst = state == "worst"
    # ---------------- B2 3V3 -----------------------------------------------------------------------
    add("MCU STM32G491", "3V3", I_MCU[c], "B2")
    rs232 = I_RS232["max"] if c == "max" else (I_RS232["S_typ"] if cfg == "S" else I_RS232["E_typ"])
    add("MAX3232E", "3V3", rs232, "B2")
    n_drv = 3 + (1 if (cfg == "E" and option == "C") else 0)
    for k, (ty, mx, _) in B2_3V3_MISC.items():
        n = n_drv if k.startswith("TMC2209") else 1
        add(k, "3V3", n * (ty if c == "typ" else mx), "B2")
    if usb_only or c == "max":
        add("USB FS transceiver", "3V3", I_USB_PHY[c], "B2")
    if c == "max":
        add("buzzer (sounding)", "5V_SYS", I_BUZZER, "B2")
    convs = [Conv("B2 3V3 LDO", "ldo", "5V_SYS", "3V3", "B2", iq=LDO_B2_IQ),
             Conv("TPS2121 mux", "switch", "VBUS" if usb_only else "5V_MAIN", "5V_SYS", "B2",
                  iq=MUX_IQ[c], ron=MUX_RON)]
    if usb_only:
        return L, convs
    # ---------------- B3 display (fed straight from the IRM-20-5, before the mux: dark in USB-only mode) ----
    add("display shield (B3)", "5V_DISP", I_DISP[c], "HMI")
    convs.append(Conv("display branch (wiring)", "switch", "5V_MAIN", "5V_DISP", "HMI"))
    # ---------------- 12 V motor branch ----------------------------------------------------------------
    running = state.split("_")[1] if state.startswith("dispense_") else ("coarse" if worst else None)
    for ch in ("coarse", "medium", "fine"):
        if ch == running:
            parts = stepper(TRK_IRUN[c], TRK_R20, c, MOTOR_RPM[ch])
            addp(f"stepper {ch} (running)", "12V_MOT",
                 {"TRK": parts["copper"] + parts["iron"] + parts["mech"],
                  "B2": parts["fets"] + parts["rsense"] + parts["switching"] + parts["quiescent"]})
        else:
            add(f"TMC2209 {ch} (disabled)", "12V_MOT", IQ_DIS[c], "B2")
    convs.append(Conv("eFuse motor", "switch", "12V", "12V_MOT", "B2", iq=EF_IQ[c], ron=EF_RON[c]))
    convs.append(Conv(IRM20.name, "module", "AC", "5V_MAIN", "B1", mod=IRM20))
    if cfg == "S":
        convs.append(Conv(IRM30.name, "module", "AC", "12V", "B1", mod=IRM30))
        return L, convs
    # ---------------- Config E: check-mass driver ------------------------------------------------------
    cm_rail, cm_zone = ("12V_MOT", "B2") if option == "C" else ("12V_B4", "B4")
    if state in ("cm_lift", "worst"):
        parts = stepper(CM_IRUN[c], CM_R20, c, 0.0, iron_ref=CM_IRON_MECH[c], iq={"rs": CM_RSENSE})
        addp("check-mass stepper (running)", cm_rail,
             {"CELL": parts["copper"] + parts["iron"] + parts["mech"],
              cm_zone: parts["fets"] + parts["rsense"] + parts["switching"] + parts["quiescent"]})
    else:
        add("TMC2209 check-mass (disabled)", cm_rail, IQ_DIS[c], cm_zone)
    # ---------------- B4 5V_A ------------------------------------------------------------------------------
    for k, (ty, mx, _) in B4_5VA.items():
        add(k, "5V_A", ty if c == "typ" else mx, "B4")
    ic = coil_current(state, c)
    add("coil current |I| (A+B winding)", "5V_A", abs(ic), "B4", split={"CELL": ic ** 2 * R_COIL})
    p_h = max(P0[c] - ic ** 2 * R_COIL, 0.0)
    add("heater (C+D hairpin)", "5V_A", p_h / (RAILV["5V_A"] * ETA_HB), "B4", split={"CELL": p_h})
    add("LED (TSAL6200 class, DC)", "5V_A", I_LED[c], "B4", split={"CELL": VF_LED[c] * I_LED[c]})
    for k, (ty, mx, _) in B4_3V3D.items():
        add(k, "3V3_D", ty if c == "typ" else mx, "B4")
    b4_in = "6V_B4" if option == "C" else "12V_B4"
    convs += [Conv("LT3045 5V_A", "ldo", b4_in, "5V_A", "B4", iq=LT3045_IQ[c]),
              Conv("LDO 3V3_D", "ldo", b4_in, "3V3_D", "B4", iq=LDO_3V3D_IQ)]
    if option == "C":
        convs.append(Conv("TPS62170 pre-reg", "buck", "12V_B4", "6V_B4", "B2", iq=BUCK_IQ, eta=BUCK_ETA[c]))
    convs.append(Conv("eFuse B4", "switch", "12V", "12V_B4", "B2", iq=EF_IQ[c], ron=EF_RON[c]))
    convs.append(Conv(IRM30.name, "module", "AC", "12V", "B1", mod=IRM30))
    return L, convs


def solve(loads, convs):
    """Bottom-up through the (ordered) converter list. Returns rail currents, converter table, heat by zone."""
    rail_i = defaultdict(float)
    heat = defaultdict(float)
    for name, rail, i, h in loads:
        rail_i[rail] += i
        assert abs(sum(h.values()) - RAILV[rail] * i) < 1e-12, name
        for z, p in h.items():
            heat[z] += p
    rows = []
    p_ac = 0.0
    for cv in convs:
        i_out = rail_i[cv.outp]
        v_in, v_out = RAILV[cv.inp], RAILV[cv.outp]
        p_out = v_out * i_out
        if cv.kind == "ldo":
            i_in = i_out + cv.iq
            p_in = v_in * i_in
        elif cv.kind == "switch":
            p_in = p_out + i_out ** 2 * cv.ron + v_in * cv.iq
            i_in = p_in / v_in
        elif cv.kind == "buck":
            p_in = p_out / cv.eta + v_in * cv.iq
            i_in = p_in / v_in
        else:  # module
            p_in = p_out + module_loss(cv.mod, p_out)
            i_in = p_in / v_in
            p_ac += p_in
        loss = p_in - p_out
        heat[cv.zone] += loss
        rail_i[cv.inp] += i_in
        rows.append((cv, i_in, i_out, p_in, p_out, loss))
    return rail_i, rows, heat, p_ac


ZONES = ["B1", "B2", "TRK", "HMI", "B4", "CELL"]
ZONE_TXT = {"B1": "B1 mains box (rear, metal)", "B2": "B2 controller (rear)",
            "TRK": "trickler motors (frame, rear of tubes)", "HMI": "display + B3 (front panel)",
            "B4": "B4 EMFC module (beside cell; location TBD)", "CELL": "cell in draft shield (coil, LED, CM motor)"}

# =====================================================================================================
# 1. Stepper supply power (not phase current)
# =====================================================================================================
w("1. Stepper supply power from motor power (TMC2209 chopper at 12 V; idle motors are DISABLED, ENN high via")
w("   the 2-to-4 decoder (D-041): zero phase current, only the driver's VS quiescent 7 typ / 10 max mA each)")
w(f"   Trickler motor ASSUMED 17HS08-1004S class: {TRK_I_RATED:.1f} A rated, {TRK_R20:.1f} ohm at 20 C. Rsense "
  f"{TRK_RSENSE*1e3:.0f} mohm (PROPOSED) -> hardware ceiling {TRK_IRUN['max']:.3f} A rms (CS = 31) [DS formula]")
w(f"   Run current PROPOSED {TRK_IRUN['typ']:.2f} A rms typ (torque needed 1.25 mN.m vs ~130 mN.m holding at rated).")
w(f"   {'stage':<8}{'corner':<6}{'I_rms':>7}{'copper':>8}{'iron':>7}{'mech':>7}{'FETs':>7}{'Rs':>7}{'sw':>7}"
  f"{'Iq':>7}{'P_sup W':>9}{'I_sup A':>9}{'I_sup/I_ph':>11}")
STEP_P = {}
for ch in ("coarse", "medium", "fine"):
    for c in ("typ", "max"):
        p = stepper(TRK_IRUN[c], TRK_R20, c, MOTOR_RPM[ch])
        tot = sum(p.values())
        STEP_P[(ch, c)] = (p, tot)
        w(f"   {ch:<8}{c:<6}{TRK_IRUN[c]:>7.3f}{p['copper']:>8.3f}{p['iron']:>7.3f}{p['mech']:>7.3f}{p['fets']:>7.3f}"
          f"{p['rsense']:>7.3f}{p['switching']:>7.3f}{p['quiescent']:>7.3f}{tot:>9.2f}{tot/12:>9.3f}"
          f"{tot/12/TRK_IRUN[c]:>11.2f}")
        check(f"stepper {ch}/{c}: supply current < phase rms current (chopper energy balance)", tot / 12 < TRK_IRUN[c])
p_cm = {c: stepper(CM_IRUN[c], CM_R20, c, 0.0, iron_ref=CM_IRON_MECH[c], iq={"rs": CM_RSENSE}) for c in ("typ", "max")}
w(f"   Check-mass motor ASSUMED NEMA 8 + planetary gearbox (0.6 A, 10 ohm). Rsense {CM_RSENSE*1e3:.0f} mohm "
  f"(PROPOSED) -> ceiling {CM_IRUN['max']:.3f} A rms, run {CM_IRUN['typ']:.2f} A typ:")
for c in ("typ", "max"):
    p = p_cm[c]
    w(f"      {c}: motor (copper+iron) {p['copper']+p['iron']:.2f} W in the CELL zone, driver "
      f"{p['fets']+p['rsense']+p['switching']+p['quiescent']:.2f} W, supply {sum(p.values()):.2f} W = "
      f"{sum(p.values())/12:.3f} A at 12 V; energy per span check ({CM_T_MOVE:.0f} s of motion) "
      f"{(p['copper']+p['iron'])*CM_T_MOVE:.1f} J into the cell zone")
# copper-loss identity for sine/cosine microstepping
ms = [(math.cos(2 * math.pi * k / 64), math.sin(2 * math.pi * k / 64)) for k in range(64)]
ident = max(abs((a * a + b * b) - 1.0) for a, b in ms)
check("microstep copper loss R*(IA^2+IB^2) = 2*R*I_rms^2 at every microstep", ident < 1e-12)
sens = [(i, sum(stepper(i, TRK_R20, "typ", MOTOR_RPM["coarse"]).values())) for i in (0.3, 0.5, 0.7)]
w("   Supply power ~IRUN^2: " + ", ".join(f"{i:.1f} A -> {p:.1f} W" for i, p in sens) +
  " (typ corner, coarse). Run the trickler motors at the lowest reliable current.")
w("")

# =====================================================================================================
# 2. Load table per rail, configuration and state
# =====================================================================================================
RAIL_ROWS = [("12V_MOT", "12 V motor branch (behind eFuse M)", "A"),
             ("12V_B4", "12 V B4 branch (behind eFuse E)", "A"),
             ("6V_B4", "6.0 V pre-reg out -> B4 (option C)", "A"),
             ("5V_A", "B4 5V_A analog (LT3045)", "mA"),
             ("3V3_D", "B4 3V3_D digital", "mA"),
             ("12V", "IRM-30-12 output", "A"),
             ("5V_DISP", "5 V display branch (B3)", "A"),
             ("5V_SYS", "5 V system (after mux)", "A"),
             ("3V3", "B2 3V3 (MCU, logic)", "mA"),
             ("5V_MAIN", "IRM-20-5 output", "A")]
RES = {}
for cfg, states in (("S", STATES_S), ("E", STATES_E)):
    for opt in (("A", "C") if cfg == "E" else ("-",)):
        for st in states:
            for c in ("typ", "max"):
                loads, convs = build(cfg, st, c, option=opt if opt != "-" else "C")
                RES[(cfg, opt, st, c)] = solve(loads, convs)

w("2. Rail currents by configuration and state (typ / max). Max = every IC at its max, motors at the hardware")
w("   current ceiling, buzzer sounding, coil at +120 % FS, TC16 heater power, LED 30 mA, display 400 mA.")
w("   States: idle = SAFE armed, motors disabled, display on, (E) servo at an empty pan. dispense_x = that trickler")
w("   motor running. cm_lift = check-mass motor running. weigh_full = 520 g on the pan. worst = coarse motor AND")
w("   check-mass motor at their ceilings simultaneously (only possible if the CM driver is not on the decoder).")
for cfg, opt, states in (("S", "-", STATES_S), ("E", "A", STATES_E), ("E", "C", STATES_E)):
    title = {"S": "Config S (B1+B2+B3)", "A": "Config E, option A as drawn (B4 LDOs from 12 V, CM driver on B4)",
             "C": "Config E, option C PROPOSED (6.0 V pre-reg on B2, CM driver on B2)"}[opt if cfg == "E" else "S"]
    w(f"   -- {title}")
    hdr = "".join(f"{s.replace('dispense_', 'disp_'):>14}" for s in states)
    w(f"   {'rail':<38}{'unit':>5}{hdr}")
    for rail, txt, unit in RAIL_ROWS:
        if cfg == "S" and rail in ("12V_B4", "6V_B4", "5V_A", "3V3_D"):
            continue
        if opt == "A" and rail == "6V_B4":
            continue
        cells = ""
        for st in states:
            vals = []
            for c in ("typ", "max"):
                ri = RES[(cfg, opt, st, c)][0][rail]
                vals.append(ri * (1e3 if unit == "mA" else 1))
            fmt = "{:.0f}/{:.0f}" if unit == "mA" else "{:.3f}/{:.3f}"
            cells += f"{fmt.format(*vals):>14}"
        w(f"   {txt:<38}{unit:>5}{cells}")
    pac = "".join(f"{RES[(cfg, opt, st, 'typ')][3]:.1f}/{RES[(cfg, opt, st, 'max')][3]:.1f}".rjust(14) for st in states)
    w(f"   {'AC input power (W)':<38}{'W':>5}{pac}")
w("   Fault (not a sizing case): decoder failed, all three trickler motors at the ceiling -> see eFuse section.")
w("")
w("   B2 3V3 detail (typ/max mA): MCU %.1f/%.1f (Table 21 + %.1f mA peripherals from Table 34 at 170 MHz), "
  "MAX3232E %.1f(S)/%.1f(E)/%.1f" % (I_MCU["typ"] * 1e3, I_MCU["max"] * 1e3, I_PERIPH * 1e3,
                                      I_RS232["S_typ"] * 1e3, I_RS232["E_typ"] * 1e3, I_RS232["max"] * 1e3))
w("")

# =====================================================================================================
# 3. Module derating and margin (PWR-04)
# =====================================================================================================
w("3. Mean Well IRM derating and margin (PWR-04: >= 25 % on each module, derated per datasheet)")
a_encl = 2 * (ENCL[0] * ENCL[1] + ENCL[0] * ENCL[2] + ENCL[1] * ENCL[2])
a_box = 2 * (BOX[0] * BOX[1] + BOX[0] * BOX[2] + BOX[1] * BOX[2])
p_all_max = max(RES[(cfg, opt, st, "typ")][3] for (cfg, opt, st, cc) in RES if cc == "typ")
p_b1_max = max(RES[k][2]["B1"] for k in RES if k[3] == "typ")
dT_encl = p_all_max / (U_ENCL * a_encl)
dT_box = p_b1_max / (U_BOX * a_box)
t_box = T_GARAGE_MAX + dT_encl + dT_box
w(f"   Air-rise estimate (typ, highest-power state): enclosure {p_all_max:.1f} W / ({U_ENCL} W/m2K x {a_encl:.2f} m2) "
  f"= +{dT_encl:.1f} K; mains box {p_b1_max:.2f} W / ({U_BOX} x {a_box:.3f} m2) = +{dT_box:.1f} K")
w(f"   -> module ambient ~{t_box:.1f} C at a {T_GARAGE_MAX:.0f} C garage; design value T_int = {T_INT_DESIGN:.0f} C "
  f"(ASSUMED, +{T_INT_DESIGN - t_box:.1f} K allowance)")
check("estimated B1 module ambient below the design T_int", t_box < T_INT_DESIGN)
MARGIN = {}
for mod, rail in ((IRM30, "12V"), (IRM20, "5V_MAIN")):
    kT = interp(mod.der_T, T_INT_DESIGN) / 100
    kV_nom = interp(mod.der_V, V_AC_NOM.m) / 100
    kV_low = interp(mod.der_V, V_AC_LOW.m) / 100
    i_avail = mod.I * kT * min(kV_nom, kV_low)
    worst_key = max((k for k in RES if k[3] == "max"), key=lambda k: RES[k][0][rail])
    i_load = RES[worst_key][0][rail] / (1 - MODULE_TOL)   # constant-power bound at the -2.5 % edge
    margin = 1 - i_load / i_avail
    MARGIN[mod.name] = margin
    # ambient at which margin would fall to 25 %
    need = i_load / 0.75 / (mod.I * min(kV_nom, kV_low)) * 100
    t_lim = None
    for t in [x / 10 for x in range(-300, 851)]:
        if interp(mod.der_T, t) < need:
            t_lim = t
            break
    t_txt = f"{t_lim:.0f} C" if t_lim is not None else "the end of the curve (85 C)"
    w(f"   {mod.name}: rating {mod.I:.1f} A; k_T({T_INT_DESIGN:.0f} C) = {kT:.2f}; k_V(120 VAC) = {kV_nom:.2f}, "
      f"k_V({V_AC_LOW.m:.0f} VAC) = {kV_low:.3f} -> available {i_avail:.2f} A")
    w(f"      worst load {RES[worst_key][0][rail]:.3f} A ({worst_key[0]}/{worst_key[1]}/{worst_key[2]}, max corner) "
      f"/ 0.975 = {i_load:.3f} A -> margin {margin:.0%} (need >= 25 %); 25 % holds up to {t_txt} module ambient")
    check(f"{mod.name} margin >= 25 % at {T_INT_DESIGN:.0f} C and {V_AC_LOW.m:.0f} VAC", margin >= 0.25)
w("   Universal input (PWR-02): both modules accept 85-305 VAC, 47-440 Hz [DS]; IRM-20 derates below 115 VAC "
  "(80 % at 85 VAC), IRM-30 below 100 VAC.")
w("")

# =====================================================================================================
# 4. Hold-up, inrush, fuses
# =====================================================================================================
w("4. Hold-up, cold-start inrush and time-lag fuses")
for mod, rail in ((IRM30, "12V"), (IRM20, "5V_MAIN")):
    p_in_full = mod.P / (mod.eta_full - ETA_LOWLINE)
    lines = []
    for key, lab in ((("E", "A", "idle", "typ"), "E idle (A)"), (("E", "C", "idle", "typ"), "E idle (C)"),
                     (("S", "-", "dispense_coarse", "typ"), "dosing peak")):
        p_out = RES[key][0][rail] * mod.V
        p_in = p_out + module_loss(mod, p_out)
        lines.append(f"{lab} {p_out:.1f} W -> {mod.hold_full * p_in_full / p_in * 1e3:.0f} ms")
    w(f"   {mod.name} hold-up {mod.hold_full*1e3:.0f} ms at full load, 115 VAC [DS]; at our loads: " + "; ".join(lines))
# inrush and fuse sizing
PULSE_FACTOR = 0.22          # ASSUMED: melting I2t derate for ~1e5 inrush pulses (fuse-maker guidance)
TAU_FACTOR = 3.0             # design factor on the ASSUMED bulk capacitance
K_FUSE = 0.75 * 0.95         # continuous derating 75 % x temperature rerating at ~45 C (ASSUMED)
# Bel Fuse 5HT (ceramic, time-lag, IEC 60127-2 sheet 5, 1500 A @ 250 VAC) [DS Rev. June 2026]: In: (I2t<10ms A2s, R_cold ohm)
FUSE_5HT = {0.5: (0.46, 1.03), 0.63: (0.70, 0.63), 0.8: (1.1, 0.39), 1.0: (1.7, 0.20), 1.25: (3.0, 0.14),
            1.6: (5.0, 0.095), 2.0: (10.0, 0.069), 2.5: (17.0, 0.047), 3.15: (31.0, 0.031)}
v_pk = V_AC_NOM.m * math.sqrt(2)
inr = {}
for mod in (IRM30, IRM20):
    ipk = mod.inrush * V_AC_NOM.m / V_AC_DS.m
    tau = (v_pk / ipk) * mod.c_bulk
    i2t = ipk ** 2 * tau / 2
    inr[mod.name] = (ipk, tau)
    i_in_low = mod.i_ac_full * V_AC_DS.m / V_AC_LOW.m
    need_in = i_in_low / K_FUSE
    need_i2t = i2t * TAU_FACTOR / PULSE_FACTOR
    pick = min(k for k, (it, _) in FUSE_5HT.items() if k >= need_in and it >= need_i2t)
    w(f"   {mod.name}: inrush {ipk:.1f} A pk at 120 VAC, tau ~{tau*1e3:.2f} ms (C_bulk {mod.c_bulk*1e6:.0f} uF ASSUMED)"
      f" -> I2t {i2t:.3f} A2s; x{TAU_FACTOR:.0f}/{PULSE_FACTOR} -> fuse melting I2t >= {need_i2t:.2f} A2s")
    w(f"      full-load input {mod.i_ac_full:.2f} A at 115 VAC -> {i_in_low:.2f} A at {V_AC_LOW.m:.0f} VAC; "
      f"/ {K_FUSE:.3f} -> I_n >= {need_in:.2f} A  => T{pick:g}A 250 V ceramic (5HT class: {FUSE_5HT[pick][0]} A2s)")
    check(f"{mod.name} fuse T{pick:g}A: rating and I2t both cover the need", pick >= need_in and FUSE_5HT[pick][0] >= need_i2t)
    inr[mod.name] = (ipk, tau, pick)
(ip1, t1, f1), (ip2, t2, f2) = inr[IRM30.name], inr[IRM20.name]
i2t_inlet = ip1 ** 2 * t1 / 2 + ip2 ** 2 * t2 / 2 + 2 * ip1 * ip2 * t1 * t2 / (t1 + t2)
need_in = (IRM30.i_ac_full + IRM20.i_ac_full) * V_AC_DS.m / V_AC_LOW.m / K_FUSE
need_i2t = i2t_inlet * TAU_FACTOR / PULSE_FACTOR
pick_in = min(k for k, (it, _) in FUSE_5HT.items() if k >= need_in and it >= need_i2t)
w(f"   Inlet (both modules start together): I2t {i2t_inlet:.3f} A2s -> need melting I2t >= {need_i2t:.2f} A2s and "
  f"I_n >= {need_in:.2f} A => T{pick_in:g}A 250 V ceramic in the C14 drawer")
sel = min(FUSE_5HT[pick_in][0] / FUSE_5HT[f1][0], FUSE_5HT[pick_in][0] / FUSE_5HT[f2][0])
w(f"   Selectivity indicator: inlet / module melting I2t >= {sel:.1f} (a module fault usually clears its own fuse; "
  f"not guaranteed, both are inside the same enclosure anyway)")
check("inlet fuse rating and I2t cover both modules", pick_in >= need_in and FUSE_5HT[pick_in][0] >= need_i2t)
pdc = math.sqrt(2) * V_AC_NOM.m
pf = (IRM30.P / (IRM30.eta_full - ETA_LOWLINE)) / (V_AC_DS.m * IRM30.i_ac_full)
w(f"   Cross-check: IRM-30 full-load P_in {IRM30.P / (IRM30.eta_full - ETA_LOWLINE):.1f} W at 115 VAC / 0.75 A [DS] "
  f"-> implied power factor {pf:.2f} (capacitor-input, no PFC: 0.35-0.65 expected)")
check("IRM-30 datasheet AC current implies a plausible PF (0.35-0.65)", 0.35 < pf < 0.65)
w("   Why ceramic high-breaking (1500 A): a US 15/20 A branch can deliver far more than the 35 A breaking capacity")
w("   of a glass 5x20 fuse. 250 V fuses cover 100-240 VAC, not 277 VAC.")
w("")

# =====================================================================================================
# 5. TPS25947 eFuse settings
# =====================================================================================================
w("5. TPS25947 eFuse settings [DS SLVSFC9C May 2026: R_ILM = 3334/I_LIM, R_ILM 549-6650 ohm, +-10 % above 1 A,")
w("   ~+-15 % at 0.5-1 A; fast trip at 2 x I_LIM; dV/dt: C(pF) = 2000/SR(V/ms); t_ITIMER(ms) = V(V)*C(nF)/I(uA)]")
E96 = [1.00, 1.02, 1.05, 1.07, 1.10, 1.13, 1.15, 1.18, 1.21, 1.24, 1.27, 1.30, 1.33, 1.37, 1.40, 1.43, 1.47, 1.50,
       1.54, 1.58, 1.62, 1.65, 1.69, 1.74, 1.78, 1.82, 1.87, 1.91, 1.96, 2.00, 2.05, 2.10, 2.15, 2.21, 2.26, 2.32,
       2.37, 2.43, 2.49, 2.55, 2.61, 2.67, 2.74, 2.80, 2.87, 2.94, 3.01, 3.09, 3.16, 3.24, 3.32, 3.40, 3.48, 3.57,
       3.65, 3.74, 3.83, 3.92, 4.02, 4.12, 4.22, 4.32, 4.42, 4.53, 4.64, 4.75, 4.87, 4.99, 5.11, 5.23, 5.36, 5.49,
       5.62, 5.76, 5.90, 6.04, 6.19, 6.34, 6.49, 6.65, 6.81, 6.98, 7.15, 7.32, 7.50, 7.68, 7.87, 8.06, 8.25, 8.45,
       8.66, 8.87, 9.09, 9.31, 9.53, 9.76]


def e96_below(r):
    dec = 10 ** math.floor(math.log10(r))
    cands = [v * dec for v in E96 if v * dec <= r * 1.000001]
    return max(cands) if cands else E96[-1] * dec / 10


def efuse(name, i_branch, c_load, i_inrush):
    tol = 0.10 if i_branch * 1.25 / 0.9 > 1.0 else 0.15
    i_nom_need = max(1.25 * i_branch / (1 - tol), 3334 / 6650)
    r = min(e96_below(3334 / i_nom_need), 6650.0)
    i_nom = 3334 / r
    sr = i_inrush / c_load / 1e3            # V/ms
    c_dvdt = 2000 / sr                      # pF
    t_rise = 12.0 / sr
    w(f"   {name}: branch max {i_branch:.3f} A -> R_ILM {r:.0f} ohm (E96) -> I_LIM {i_nom:.2f} A nominal "
      f"({i_nom*(1-tol):.2f}-{i_nom*(1+tol):.2f} A), fast trip ~{2*i_nom:.1f} A; C_load {c_load*1e6:.0f} uF at "
      f"{i_inrush:.2f} A inrush -> SR {sr:.3f} V/ms, C_dVdt ~{c_dvdt/1e3:.1f} nF, rise {t_rise:.0f} ms")
    return i_nom, tol


# motor branch: single trickler motor at its ceiling (decoder) + CM on the same branch (option C)
i_mot_dec = max(RES[("E", "C", st, "max")][0]["12V_MOT"] for st in STATES_E if st != "worst")
i_mot_wst = RES[("E", "C", "worst", "max")][0]["12V_MOT"]
w(f"   Motor branch max: {i_mot_dec:.3f} A with the CM driver on decoder code 3 (PROPOSED: one motor of four);")
w(f"   {i_mot_wst:.3f} A if the CM motor can run together with a trickler motor.")
i_lim_m, tol_m = efuse("eFuse M, CM on the decoder (PROPOSED)", i_mot_dec, 4 * 100e-6, 0.20)
check("eFuse M: I_LIM,min >= 1.25 x branch max (CM on the decoder)", i_lim_m * (1 - tol_m) >= 1.25 * i_mot_dec)
i_lim_m2, tol_m2 = efuse("eFuse M, CM not interlocked (alternative)", i_mot_wst, 4 * 100e-6, 0.20)
check("eFuse M alternative: I_LIM,min >= 1.25 x branch max (CM simultaneous)",
      i_lim_m2 * (1 - tol_m2) >= 1.25 * i_mot_wst)
i_fault = 3 * sum(STEP_P[("coarse", "max")][0].values()) / 12
w(f"   Decoder-failure fault (3 motors at the ceiling): {i_fault:.2f} A > I_LIM,max {i_lim_m*(1+tol_m):.2f} A -> the "
  f"eFuse trips. At typ run current (3 x {sum(STEP_P[('coarse','typ')][0].values())/12:.2f} A) it does NOT: the eFuse")
w("   is a fault limit, not a one-motor interlock (the decoder is).")
check("eFuse M trips on the 3-motor decoder fault at the ceiling", i_fault > i_lim_m * (1 + tol_m))
w("   C_dVdt > 10 nF gets a 100 ohm series resistor [DS note]. ITIMER: ~12 nF -> ~10 ms blanking (~1.6 V / 1.8 uA)")
w("   for acceleration transients. Variant: TPS259474L")
w("   (circuit breaker, latch-off) with FLT into the SAFE latch reset, so a motor-rail fault needs the ARM button.")
i_b4_a = max(RES[("E", "A", st, "max")][0]["12V_B4"] for st in STATES_E)
i_b4_c = max(RES[("E", "C", st, "max")][0]["12V_B4"] for st in STATES_E)
efuse("eFuse E (B4, option A, incl. CM motor)", i_b4_a, 47e-6, 0.10)
efuse("eFuse E (B4, option C, feeds hold-up C + pre-reg)", i_b4_c, 6800e-6, 0.10)
w("   Option C: the 0.5 A floor is ~10x the load, so the LT3045 programmable current limits are the tight layer.")
w("   EN of eFuse E from its own UVLO divider (not from logic), so B4 stays up while the 5 V domain resets.")
w("")

# =====================================================================================================
# 6. Linear regulators and the intermediate-buck question
# =====================================================================================================
w("6. Linear-regulator dissipation (typ / max load; LDO loss at high line 12.3 V for the max column)")


def ldo_loss(v_in, v_out, i_out, iq):
    return (v_in - v_out) * i_out + v_in * iq


i5a = {c: RES[("E", "C", "idle" if c == "typ" else "worst", c)][0]["5V_A"] for c in ("typ", "max")}
i3d = {c: RES[("E", "C", "idle" if c == "typ" else "worst", c)][0]["3V3_D"] for c in ("typ", "max")}
w(f"   B4 loads: 5V_A {i5a['typ']*1e3:.1f} typ / {i5a['max']*1e3:.1f} max mA; 3V3_D {i3d['typ']*1e3:.1f} / "
  f"{i3d['max']*1e3:.1f} mA")
opts = {}
for lab, vin_t, vin_m in (("A: LT3045 from 12 V on B4", 12.0, 12.3), ("C: LT3045 from 6.0 V pre-reg", 6.0, 6.0)):
    pt = ldo_loss(vin_t, 5.0, i5a["typ"], LT3045_IQ["typ"]) + ldo_loss(vin_t, 3.3, i3d["typ"], LDO_3V3D_IQ)
    pm = ldo_loss(vin_m, 5.0, i5a["max"], LT3045_IQ["max"]) + ldo_loss(vin_m, 3.3, i3d["max"], LDO_3V3D_IQ)
    tj = T_GARAGE_MAX + 5 + ldo_loss(vin_m, 5.0, i5a["max"], LT3045_IQ["max"]) * LT3045_THJA
    opts[lab[0]] = (pt, pm)
    w(f"   {lab:<32} LDO loss on B4 {pt:.3f} / {pm:.3f} W; LT3045 Tj ~{tj:.0f} C (theta_JA {LT3045_THJA:.0f} C/W)")
    check(f"B4 LT3045 Tj < 125 C ({lab[0]})", tj < 125)
# option B: dropping resistor on B2
r_drop = (11.7 - 6.0) / (i5a["max"] + i3d["max"] + LT3045_IQ["max"])
r_drop = math.floor(r_drop)
vb_t = 12.0 - r_drop * (i5a["typ"] + i3d["typ"] + LT3045_IQ["typ"])
pb_t = ldo_loss(vb_t, 5.0, i5a["typ"], LT3045_IQ["typ"]) + ldo_loss(vb_t, 3.3, i3d["typ"], LDO_3V3D_IQ)
w(f"   B: {r_drop:.0f} ohm dropping resistor on B2 (no switcher): B4 input {vb_t:.1f} V typ, LDO loss on B4 "
  f"{pb_t:.3f} W; resistor {r_drop*(i5a['typ']+i3d['typ']+LT3045_IQ['typ'])**2:.2f} W on B2")


def b4_heat_option_b(state):
    """Option B = option A with the dropping resistor on B2 (CM driver still on B4). B4 heat = analog loads
    (taken from option C, minus C's LDO losses) + LDO losses at the resistor-fed input + TMC2209 #4 quiescent."""
    ri, rows, heat, _ = RES[("E", "C", state, "typ")]
    ldo_c = sum(r[5] for r in rows if r[0].name in ("LT3045 5V_A", "LDO 3V3_D"))
    loads_b4 = heat["B4"] - ldo_c
    ia, id_ = ri["5V_A"], ri["3V3_D"]
    v_in = 12.0 - r_drop * (ia + id_ + LT3045_IQ["typ"] + LDO_3V3D_IQ)
    return loads_b4 + ldo_loss(v_in, 5.0, ia, LT3045_IQ["typ"]) + ldo_loss(v_in, 3.3, id_, LDO_3V3D_IQ) + 12.0 * IQ_DIS["typ"]


hb0, hb1 = b4_heat_option_b("idle"), b4_heat_option_b("weigh_full")
w(f"      option B B4 heat {hb0:.3f} W at an empty pan, {hb1:.3f} W at 520 g (delta {(hb1-hb0)*1e3:+.0f} mW)")
pre = (V_PREREG * (i5a["typ"] + i3d["typ"] + LT3045_IQ["typ"] + LDO_3V3D_IQ))
w(f"   C costs a TPS62170-class buck on B2: {pre/BUCK_ETA['typ'] - pre:.3f} W loss at the rear (eta "
  f"{BUCK_ETA['typ']:.2f} ASSUMED), 2.25 MHz ripple rejected by the LT3045 plus an LC at B4 entry.")
i3 = {c: RES[("E", "C", "idle", c)][0]["3V3"] for c in ("typ", "max")}
p3 = {c: ldo_loss(5.0 if c == "typ" else 5.125, 3.3, i3[c], LDO_B2_IQ) for c in ("typ", "max")}
w(f"   B2 3V3 LDO from 5 V: {i3['typ']*1e3:.0f} / {i3['max']*1e3:.0f} mA -> {p3['typ']:.2f} / {p3['max']:.2f} W "
  f"(SOT-223-class theta ~60 C/W -> +{p3['max']*60:.0f} K): no buck needed")
check("B2 3V3 LDO dissipation < 0.3 W at max", p3["max"] < 0.3)
pdisp = (5.0 - 3.3) * I_DISP["max"]
w(f"   Display 3V3 (if the shield feeds its backlight from the header 3.3 V pin, as newer MCUFRIEND boards do):")
w(f"   up to {I_DISP['max']*1e3:.0f} mA -> {pdisp:.2f} W in an LDO at the front panel; a 3.3 V buck there cuts "
  f"~{pdisp - (3.3*I_DISP['max']/0.9 - 3.3*I_DISP['max']):.2f} W. It must NOT load the B2 MCU 3V3 rail.")
w("")

# =====================================================================================================
# 7. USB-only service mode
# =====================================================================================================
w("7. USB-only service mode (mains off; VBUS -> TPS2121 -> 3V3 LDO; display, 12 V, motors, B4 unpowered)")
USB = {}
for c in ("typ", "max"):
    loads, convs = build("E", "idle", c, option="C", usb_only=True)
    ri, rows, heat, _ = solve(loads, convs)
    USB[c] = ri["VBUS"]
w(f"   VBUS current {USB['typ']*1e3:.0f} typ / {USB['max']*1e3:.0f} max mA (max incl. buzzer {I_BUZZER*1e3:.0f} mA)")
w(f"   vs USB 2.0 / Type-C Default: 100 mA before configuration (1 unit load), 500 mA after; 1.5/3.0 A only if the")
w(f"   source advertises it on CC (not sensed: CC has bare 5.1 kohm Rd, UCPD unused).")
check("USB-only mode <= 500 mA (configured, USB 2.0 / Type-C Default)", USB["max"] <= 0.5)
check("USB-only mode typ <= 100 mA (pre-enumeration unit load)", USB["typ"] <= 0.1)
w(f"   Pre-enumeration: typ {'PASS' if USB['typ'] <= 0.1 else 'FAIL'} vs 100 mA; max {USB['max']*1e3:.0f} mA "
  f"exceeds it only with the buzzer on -> no buzzer before enumeration.")
w("   Back-powering rule: with V5_MAIN absent the firmware holds the LCD bus (PE8-15, WR, RS, RD, CS) and every")
w("   B4 line low or Hi-Z; otherwise current flows through the unpowered 74HC245/ADS1263 clamp diodes.")
w("")

# =====================================================================================================
# 8. Coil soft-off hold-up on mains loss (D-070 includes brown-out)
# =====================================================================================================
w("8. Coil soft-off on mains loss / brown-out (D-070 CONFIRMED: SAFE incl. brown-out -> RC ramp ~0.5 s)")
hold_12 = {}
for opt in ("A", "C"):
    p_out = RES[("E", opt, "idle", "typ")][0]["12V"] * 12
    p_in = p_out + module_loss(IRM30, p_out)
    hold_12[opt] = IRM30.hold_full * (IRM30.P / (IRM30.eta_full - ETA_LOWLINE)) / p_in
p_out5 = RES[("E", "C", "idle", "typ")][0]["5V_MAIN"] * 5
hold_5 = IRM20.hold_full * (IRM20.P / (IRM20.eta_full - ETA_LOWLINE)) / (p_out5 + module_loss(IRM20, p_out5))
w(f"   Module hold-up at E idle: 5 V {hold_5*1e3:.0f} ms, 12 V {hold_12['A']*1e3:.0f} ms (A) / "
  f"{hold_12['C']*1e3:.0f} ms (C), vs {T_HOLD_REQ:.1f} s needed (3 x tau {TAU_SOFTOFF} s, ASSUMED) -> without local")
w("   hold-up a mains dropout is a HARD coil cut at whatever load is on the pan.")
check(f"finding reproduced: unbuffered 12 V hold-up ({max(hold_12.values())*1e3:.0f} ms) < {T_HOLD_REQ:.1f} s "
      f"soft-off need", max(hold_12.values()) < T_HOLD_REQ)
# energy: coil path decays exponentially; kept-alive parts run for T_HOLD_REQ; everything else shed by PWR_FAIL
e_coil = RAILV["5V_A"] * I_COIL_MAX * TAU_SOFTOFF
p_keep = RAILV["5V_A"] * (2 * OPA_IQ["max"] + 1.0e-3 + 0.5e-3)
e_5va = e_coil + p_keep * T_HOLD_REQ
v_uv = 10.5      # V, ASSUMED 12 V-UV trip (PWR_FAIL) that starts the soft-off and sheds loads
for opt, v_b4, v_min, extra in (("A", 12.0, 5.55, IQ_DIS["max"] * 12.0), ("C", V_PREREG, 5.8, 0.0)):
    e_in = e_5va * v_b4 / 5.0 + v_b4 * LT3045_IQ["max"] * T_HOLD_REQ + extra * T_HOLD_REQ
    if opt == "C":
        e_in /= BUCK_ETA["max"]
    c_req = 2 * e_in / (v_uv ** 2 - v_min ** 2)
    # without load shedding: full B4 max load for the whole interval
    p_full = RES[("E", opt, "worst", "max")][0]["12V_B4"] * 12.0
    if opt == "A":
        p_full -= sum(p_cm["max"].values())
    c_full = 2 * p_full * T_HOLD_REQ / (v_uv ** 2 - v_min ** 2)
    w(f"   Option {opt}: energy {e_in:.3f} J with load shedding -> C_hold >= {c_req*1e3:.1f} mF at 12 V "
      f"(window {v_uv} -> {v_min} V); without shedding {c_full*1e3:.0f} mF")
    if opt == "C":
        C_HOLD_C = c_req
i_b_avg = e_5va / (5.0 * T_HOLD_REQ) + LT3045_IQ["max"] + IQ_DIS["max"]   # option B keeps TMC2209 #4 on B4
v_min_b = 5.55 + r_drop * i_b_avg
c_b = 2 * ((v_uv + v_min_b) / 2 * i_b_avg * T_HOLD_REQ) / (v_uv ** 2 - v_min_b ** 2)
w(f"   Option B: {i_b_avg*1e3:.0f} mA through the {r_drop:.0f} ohm resistor -> window {v_uv} -> {v_min_b:.2f} V -> "
  f"C_hold >= {c_b*1e3:.1f} mF")
check("option C hold-up fits one 6.8 mF / 16 V electrolytic with >= 25 % margin", 6.8e-3 >= 1.25 * C_HOLD_C)
w("   PROPOSED: PWR_FAIL = 12 V-UV comparator on B2 (before eFuse E), sent to B4 and to an MCU EXTI; on PWR_FAIL B4")
w("   starts the soft-off in hardware and sheds LED, ADS1263 (PWDN), TIAs. 6.8 mF / 16 V after eFuse E (reverse")
w("   blocking), before the pre-reg (option C). Alternative: elastomer stops <= 2 kN/m (D-067), which survive a")
w("   hard cut; the OI-33 rig impact test decides.")
w("")

# =====================================================================================================
# 9. Heat map by zone (typ) for the error budget
# =====================================================================================================
w("9. Heat map by zone (typ corner, W). Peak = coarse motor running. Average over the 30 s dosing cycle =")
w(f"   coarse {DOSE['coarse']:.0f} s + medium {DOSE['medium']:.0f} s + fine {DOSE['fine']:.0f} s + "
  f"{CYCLE - sum(DOSE.values()):.0f} s motors off (back-to-back throws, DSP-06 worst case).")


def zone_heat(cfg, opt, st):
    return RES[(cfg, opt, st, "typ")][2]


def dose_avg(cfg, opt):
    hs = {st: zone_heat(cfg, opt, st) for st in ("dispense_coarse", "dispense_medium", "dispense_fine", "idle")}
    wts = {"dispense_coarse": DOSE["coarse"], "dispense_medium": DOSE["medium"], "dispense_fine": DOSE["fine"],
           "idle": CYCLE - sum(DOSE.values())}
    return {z: sum(hs[s].get(z, 0) * wts[s] for s in hs) / CYCLE for z in ZONES}


cols = [("S idle", "S", "-", "idle"), ("S peak", "S", "-", "dispense_coarse"), ("S avg", "S", "-", None),
        ("E-A idle", "E", "A", "idle"), ("E-A peak", "E", "A", "dispense_coarse"), ("E-A avg", "E", "A", None),
        ("E-C idle", "E", "C", "idle"), ("E-C peak", "E", "C", "dispense_coarse"), ("E-C avg", "E", "C", None)]
w(f"   {'zone':<46}" + "".join(f"{c[0]:>9}" for c in cols))
tot = defaultdict(float)
for z in ZONES:
    row = ""
    for lab, cfg, opt, st in cols:
        v = (dose_avg(cfg, opt)[z] if st is None else zone_heat(cfg, opt, st).get(z, 0.0))
        tot[lab] += v
        row += f"{v:>9.3f}"
    w(f"   {ZONE_TXT[z]:<46}" + row)
w(f"   {'total = AC input':<46}" + "".join(f"{tot[c[0]]:>9.2f}" for c in cols))
w("   Per trickler motor in the dosing cycle (motor body, TRK zone; driver heat is on B2):")
for ch in ("coarse", "medium", "fine"):
    p, _ = STEP_P[(ch, "typ")]
    pm = p["copper"] + p["iron"] + p["mech"]
    w(f"      {ch:<7} {pm:.2f} W while running ({DOSE[ch]:.0f} s of {CYCLE:.0f} s = {DOSE[ch]/CYCLE:.0%} duty) -> "
      f"{pm*DOSE[ch]/CYCLE:.2f} W average")
# B4 and cell detail
for opt in ("A", "C"):
    h0, h1 = zone_heat("E", opt, "idle"), zone_heat("E", opt, "weigh_full")
    w(f"   Option {opt}: B4 {h0['B4']:.3f} W at an empty pan, {h1['B4']:.3f} W at 520 g (delta "
      f"{(h1['B4']-h0['B4'])*1e3:+.0f} mW); cell {h0['CELL']*1e3:.1f} -> {h1['CELL']*1e3:.1f} mW")
hc = zone_heat("E", "C", "idle")
w(f"   Cell (constant): coil + heater P0 {P0['typ']*1e3:.0f} mW (35 with TC16) + LED {VF_LED['typ']*I_LED['typ']*1e3:.0f} mW"
  f" (ASSUMED 20 mA) = {hc['CELL']*1e3:.0f} mW. The LED is as large as P0: fix its current in OI-31 and keep it DC.")
e_cm = (p_cm["typ"]["copper"] + p_cm["typ"]["iron"]) * CM_T_MOVE
w(f"   Check-mass lift: {e_cm:.1f} J into the cell zone per span check (typ), {e_cm/CM_PERIOD*1e3:.0f} mW averaged at one "
  f"check per {CM_PERIOD/60:.0f} min; it lands just before the span reading.")
d_load_a = zone_heat("E", "A", "weigh_full")["B4"] - zone_heat("E", "A", "idle")["B4"]
d_load_c = zone_heat("E", "C", "weigh_full")["B4"] - zone_heat("E", "C", "idle")["B4"]
w(f"   Error-budget hook: B4 heat rises with load by {d_load_a*1e3:.0f} mW (A) / {d_load_c*1e3:.0f} mW (C) from empty "
  f"to 520 g. With an ASSUMED 10-20 K/W local coupling to the reference/ADC and the ~2.5 ppm/K electronics span")
w(f"   tempco (SERVO_MEASURAND sec. 7), that is {d_load_a*10*2.5:.1f}-{d_load_a*20*2.5:.1f} ppm (A) / "
  f"{d_load_c*10*2.5:.1f}-{d_load_c*20*2.5:.1f} ppm (C) at full load, a slow load-dependent bow vs MET-04 3.85 ppm.")
w("")

# =====================================================================================================
# 10. Sanity checks
# =====================================================================================================
for k, (ri, rows, heat, p_ac) in RES.items():
    check(f"energy balance {k}: sum of zone heat = AC input", abs(sum(heat.values()) - p_ac) < 1e-9)
eb = [k for k in CHECKS if k[0].startswith("energy balance")]
others = [k for k in CHECKS if not k[0].startswith("energy balance")]
w("10. Sanity checks")
w(f"   - energy balance (zone heat = AC input) in all {len(eb)} config/option/state/corner cases: "
  f"{'PASS' if all(ok for _, ok in eb) else 'FAIL'}")
for name, ok in others:
    w(f"   - {name}: {'PASS' if ok else 'FAIL'}")
n_fail = sum(1 for _, ok in CHECKS if not ok)
w(f"   {len(CHECKS) - n_fail}/{len(CHECKS)} PASS")
print("\n".join(out))
