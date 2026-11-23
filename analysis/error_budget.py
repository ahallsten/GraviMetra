"""Phase 2: EMFC error and noise budget (Config E).

Run:  .venv/Scripts/python analysis/error_budget.py   (writes analysis/error_budget.out.txt; ~20 s)

Scope: combines the Phase 2 studies into budgets for the Config E metrology requirements:
  MET-03 repeatability, MET-04 linearity, MET-06/07 span vs temperature (static and with
  auto-adjust), MET-08 zero stability, MET-09 eccentric load (summary), MET-10 warm-up and DSP-04
  dosing accuracy.  Re-scopes OI-02 (span tempco) and settles the lever ratio r = 15 vs 20.

Methods (outside methods, not from the user's mechanical textbook):
  - GUM-style budgets: independent standard uncertainties (k = 1) combined root-sum-square and
    compared with the limits at k = 2 (~95 %); MET-03 is itself an SD, so it is compared at k = 1.
  - Noise: one-sided PSDs propagated through the closed loop and the readout filter, re-using the
    plant, controller and noise model of analysis/servo_measurand.py (imported; its main() is guarded).
  - Linearity: Monte Carlo of the N-point linearity adjustment with randomised ADC INL shapes (sine
    series, smooth and rough), the deterministic mechanical/magnetic nonlinearities, calibration-mass
    errors (OIML R111 classes) and repeatability noise.
  - Temperature: first-order lag models, least-squares learning of the span-temperature model from
    check-mass adjustments, and the auto-adjust (MET-07) interval arithmetic.

Tags: [MECH] docs/phase2/EMFC_MECHANICS.md + emfc_mechanics.out.txt, [MC] MAGNET_COIL.md +
magnet_coil.out.txt, [SM] SERVO_MEASURAND.md + servo_measurand.out.txt, [DS] vendor datasheet
(docs/datasheets.md), [REQ] docs/01_REQUIREMENTS.md, [D-030] masses plan, [ASSUMED] engineering
assumption to replace with rig data, [EB] derived in this study.
"""
from __future__ import annotations

import math
import sys
import time
from pathlib import Path

import numpy as np
import pint

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis"))
import servo_measurand as sm  # noqa: E402  (model only; sm.main() is not run)

u = pint.UnitRegistry()
lines: list[str] = []
checks: list[tuple[str, bool]] = []


def w(s: str = "") -> None:
    lines.append(s)
    print(s, flush=True)


def rss(*xs: float) -> float:
    return math.sqrt(sum(x * x for x in xs))


G0 = sm.G0

# ===================================================================================== inputs
# --- requirements [REQ]
W_FS = 520.0               # g, MET-01
MET03_SD = 1.0             # mg, SD of 10 loadings at ~10 g and ~500 g
MET04_LIM = 2.0            # mg, +/- over 0..520 g (3.85 ppm FS)
MET05_REQ, MET05_GOAL = 3.0, 1.5   # s
MET06_LIM = 2.0            # ppm/K after firmware compensation, without recalibration
MET08_LIM = 1.0            # mg over a 60 s dosing cycle at stable temperature
MET09_LIM = 2.0            # mg at 175 g, e = 25 mm (mechanics allocation 1.0 mg)
DSP04_3S = 3.0             # mg, the 3-sigma figure DSP-04 assumes for an AD520-class scale
GRAIN_MG = 64.79891        # mg per grain (CLAUDE.md firmware invariant)

# --- operating point (from the servo/measurand study) [SM]
TRIM_G = sm.TRIM * W_FS    # g carried by the coil with an empty pan (counterweight trim, D-051)
R_LEV = sm.R_LEV
R_S = 150.0                # ohm shunt [SM D-064]
V_REF = 2.5                # V LTC6655B-2.5 [SM]
FSR = 2 * V_REF            # V, ADS126x full-scale range at gain 1 (INL is quoted in ppm of this)
_d37 = sm.awg_d(37)
_n_wire = sm.FILL * (sm.R_CO - sm.R_CI) * sm.H_C / (4 * math.pi * _d37**2 / 4)
KF = 2 * _n_wire * sm.KF_TURN          # N/A, A+B force winding (33.3) [SM §1]

# --- mechanics [MECH]
ZERO_PER_100NM = {"baseline": 0.42, "light": 0.20}   # mg per 100 nm of optical-null drift at r = 15 [MECH §2b]
HINGE_LABEL = {"baseline": "baseline hinges (pivots 0.05 x 10, guide 0.10)",
               "light": "light hinges (pivots 0.04 x 8, guide 0.075, truss lever)"}
ZERO_PER_100NM_R20 = 0.33  # mg, baseline hinges at r = 20 [MECH §2]
NULL_DRIFT_60S = 100e-9    # m per 60 s cycle [ASSUMED, D-050 allocation, treated as 1 sigma]
NULL_DRIFT_TC = 0.5e-6     # m/K optical null vs vane, thermal [EB requirement; ASSUMED until the sensor mount exists]
L_G, H_G, W_G = 0.070, 0.060, 0.060   # m guide length, hinge spacing, blade-pair spacing [MECH §3]
DH_FINE, DY_FINE = 0.96e-6, 0.41e-6   # m corner residuals after the fine adjust (MET-09 level) [MECH §3]
DH_DOSE, DY_DOSE = 25e-6, 10e-6       # m coarse adjust; DH from [MECH §3], DY [EB]
DY_UNADJ = 4.5e-3          # m roll-stiffness centre offset from +/-5 % shim tolerance [MECH §1b]
THETA0_NL = 0.36           # mg end-point nonlinearity at theta0 = 0.5 mrad [MECH §4]
LEVER_NL = 0.08            # mg lever-bending nonlinearity [MECH §6]
K_EMPTY, K_FULL = 59.0, 66.1          # mN m/rad tension stiffening [MECH §10] (shape of the theta0 term)
SPAN_PER_THETA0 = 127.4e-6 / 1e-3     # per rad of theta0 [MECH §4]
S_VANE = sm.S_VANE         # m vane radius
COIL_SIDE_GMM, TARE_GMM, M_CW, X_CW = 8271.0, 900.0, 187.0, 40.0   # g mm, g mm, g, mm [MECH §7]
RHO_COIL_SIDE = 3.5        # g/cm^3 mean density of coil-side parts (Al lever + Cu coil) [ASSUMED]
RHO_AL, RHO_STEEL = 2.70, 7.90
A_LOAD_MM = sm.A_LOAD * 1e3

# --- magnet, coil, thermal [MC], [SM]
ALPHA = {"Recoma 28": -336e-6, "TC16": +4e-6}      # /K net Kf tempco [MC §7]
DALPHA_DT = {"Recoma 28": 1.0e-6, "TC16": 0.3e-6}  # /K^2 curvature of alpha near room temperature [ASSUMED]
TAU_MAG = {"two-sided, bonded faces [EB]": 9.0, "one-sided, sensor on base [MC]": 19.0}   # s
LAG_RESID = 0.3            # fraction of the magnet lag left after model-based compensation [ASSUMED]
R_POT = 8.0                # K/W pot to ambient [MC ASSUMED]
C_POT = 265 * 0.49 + 83 * 0.35        # J/K steel 265 g + magnet 83 g [MC §5; c from handbook]
P0 = {"Recoma 28": 18e-3, "TC16": 35e-3}           # W constant coil power [SM §1]
P_ERR = 0.02               # fraction power-regulation error [SM §8]
P_UNCOMP = 0.3             # fraction of a power-error temperature change not caught by the magnet sensor [ASSUMED]
COIL_AXIAL_TC = 0.9e-6     # m/K coil axial shift vs the gap (Al sensor post ~50 mm vs steel pot) [ASSUMED geometry]
KF_CURV = 0.0877e6         # 1/m^2, d2Kf/dz2 / Kf = -0.0877 /mm^2 [MC §4]; dKf/dz / Kf = -KF_CURV * x0
COIL_SET = {"+/-50 um (MC)": 50e-6, "+/-20 um (EB)": 20e-6}   # coil axial setting from the Kf maximum
NONTRACK = 0.2             # fraction of a part's temperature change not tracking the magnet sensor [ASSUMED]

# --- temperature sensing [DS TMP117 SBOS740; ADS1263 SBAS661C]
TMP_LSB = 7.8125e-3        # K
TMP_SIG = 0.5 * TMP_LSB    # K per reading with 8 averages ("+/-1 LSB" read as ~2 sigma) [DS + ASSUMED reading]
TMP_RATE = 8.0             # Hz: continuous conversion, AVG = 8 (8 x 15.5 ms = 124 ms per result) [DS]
EST_WIN = 120.0            # s ramp-fit estimator window [EB]
TMP_HYST = 1 * TMP_LSB     # K over a 10-30 C cycle (DS: +/-2 LSB for a wide cycle) [ASSUMED]
NTC_SENS = 2.5 * 0.044 / 4 # V/K, 10k NTC (-4.4 %/K) in a divider at R25 from 2.5 V [handbook]
ADC2_NOISE = 7.34e-6       # V rms ADC2 gain 1 at 10 SPS [DS Table 8-3 via SM]
NTC_RATE = 2.5             # Hz effective (ADC2 shared with coil V and 2 more channels) [EB]

# --- electronics [DS]
ADC_GAIN_TC = (0.5e-6, 4e-6)    # /K typ, max (ADS126x ADC1, PGA bypassed)
REF_TC = (1.0e-6, 2.0e-6)       # /K LTC6655B typ, max (box method)
SHUNT_TC = (0.05e-6, 0.2e-6)    # /K VPG foil family
AE_CHAR = 0.1e-6           # /K residual after a B4-only heat test at commissioning [EB proposal]
REF_AGING = 20e-6          # first 1000 h, LS8 [DS], sqrt(t) law [ASSUMED shape]
BURN_IN_H = 100.0          # h burn-in before commissioning [EB]
ADJ_TIME_H = 4.0           # h time trigger for auto-adjust [EB]
MAG_HYST = 0.3e-6          # span shift after FS / pan-absent excursions [ASSUMED, rig check]
PCR_FS = 0.33e-6           # quadratic shunt self-heating at FS (SM: 0.47 ppm at I_max) [SM §1]
PCR_UNSETTLED = 0.8        # fraction of PCR not settled at the read time (shunt tau ~5 s) [ASSUMED]
INL_TC = 0.007             # /K relative change of the INL amplitude [ASSUMED; DS Fig 7-22 shows a mild T dependence]
REL_FS = 144e-6            # reluctance-force quadratic at FS, relative to FS force [MC §5]
REL_TC = 0.0005 + 336e-6   # /K: steel permeability 0.05 %/K [ASSUMED] + relative to Kf (alpha)
DT_LIN = 7.5               # K max distance from the linearity-adjust temperature (15-30 C, adjusted at 22.5 C)
KFX_QUAD_FS = 1.75e-6      # Kf(x) quadratic at FS: c*x0*dz with ~1 um coil shift at FS, x0 20 um [EB from MC/SM]

# --- check mass and calibration masses [MECH §8, D-030, OIML R111]
W_CM = 200.0               # g internal check mass
N_ADJ = 10                 # readings per auto-adjust
U_E2 = {500: 0.125, 200: 0.05}        # mg k = 1: certificate U(k=2) ~ 1/3 MPE [ASSUMED from R111 practice]
F1_MPE = {20: 0.08, 50: 0.10, 100: 0.16}   # mg; F1 used at nominal value (rectangular) [R111]
N_REP_CAL = 5              # repeats per linearity-adjust point [EB]
HYST_LIN = 0.2             # mg ascending/descending hysteresis (flexure anelasticity, clamp microslip) [ASSUMED]
ADJ_DURATION = 45.0        # s per auto-adjust (lower, settle, 10 readings, lift, re-zero) [EB estimate]

# --- environment [ASSUMED; refine from docs/phase2/research/ENV_INPUTS.md and the rig]
RAMP = {"nominal": 1.0, "design": 2.0, "bad": 5.0}   # K/h at the cell
STABLE_RAMP = 0.5          # K/h: "stable temperature" for MET-08
SIG_PLACE = 1.5e-3         # m hand placement of a test weight about the pan centre (each axis)
SIG_CENTROID = 2e-3        # m powder-pile centroid scatter between throws (each axis)
DRAFT = 0.05               # mg closed draft shield, no heat sources inside
DRAFT_DOSING = 0.10        # mg during dosing (motor heat nearby, outlet openings)
ESD = 0.10                 # mg charged powder in a grounded conductive pan
RHO_AIR = 1.2              # kg/m^3
VIB_CASES = {"x1 (SM quiet slab)": 1.0, "x3": 3.0, "x10 (SM bad case)": 10.0}

# =============================================================== derived conversions
I_PER_MG = 1e-6 * G0 / R_LEV / KF          # A per mg on the pan
V_PER_MG = I_PER_MG * R_S                  # V per mg at the ADC
MG_PER_PPM_FSR = 1e-6 * FSR / V_PER_MG     # mg per ppm of the ADC FSR


def v_of(w_g):
    """ADC1 input normalised to +VREF for a pan load in g (includes the coil-carried trim)."""
    return (np.asarray(w_g) + TRIM_G) * 1e-3 * G0 / R_LEV / KF * R_S / V_REF


# ===================================================================== noise model (SM)
fn = np.logspace(-3, np.log10(4000), 8000)
sn = 2j * np.pi * fn
_pl = sm.Plant(sm.J_EMPTY, sm.K_EMPTY, False)
_ctrl = sm.Controller(25, _pl)
_G = _ctrl.G(sn)
_PF, _Pd = _pl.tf(sn)
_L = _G * _PF
T_n = _G / (1 + _L)
T_d = -_G * _Pd / (1 + _L)
S_sens = np.abs(T_n * sm.SENS_N * np.sqrt(1 + sm.SENS_CORNER / fn)) * R_LEV / G0 * 1e6   # mg/rtHz
S_ads = np.full_like(fn, sm.ADS_EN * KF / R_S * R_LEV / G0 * 1e6)
_ref_dens = sm.REF_PP / 6.6 / math.sqrt(9.9)
_vib = sm.vib_density(fn)


def S_ref(w_g):
    return np.full_like(fn, _ref_dens * (w_g + TRIM_G) * 1e3)


def S_vib(w_g, mult=1.0):
    return np.abs(T_d) * _vib / G0 * (w_g + TRIM_G) * 1e3 * mult


def sd_noise(w_g, T, k, vib=1.0):
    """SD (mg) of one reading in a sinc^k x T readout profile."""
    H2 = sm.sinc_k(fn, T, k) ** 2
    return math.sqrt(sum(np.trapezoid(a**2 * H2, fn) for a in (S_sens, S_ads, S_ref(w_g), S_vib(w_g, vib))))


def drift_noise(w_g, T, k, tau, vib=1.0):
    """SD (mg) of the change between two readings tau apart (same profile)."""
    H2 = sm.sinc_k(fn, T, k) ** 2 * 2 * (1 - np.cos(2 * np.pi * fn * tau))
    return math.sqrt(sum(np.trapezoid(a**2 * H2, fn) for a in (S_sens, S_ads, S_ref(w_g), S_vib(w_g, vib))))


# settling time of a sinc^3 x T profile: straight line through the SM §6 simulated points
_settle_fit = np.polyfit([0.2, 0.33, 0.5], [0.84, 1.14, 1.57], 1)


def settle_time(T, big_step=False):
    return float(np.polyval(_settle_fit, T)) + (0.20 if big_step else 0.0)   # +0.2 s for >= 100 g steps [SM §5]


def ecc_sens(dh, dy):
    """(x, y) eccentric sensitivity dW/W per metre of load offset [MECH §4 models]."""
    return dh / (L_G * H_G), 2 * dy / W_G**2


def main() -> None:
    t_run = time.time()
    rng = np.random.default_rng(20261122)
    w("EMFC error and noise budget (Config E).  Tags [MECH]/[MC]/[SM]/[DS]/[REQ]/[ASSUMED]/[EB].")
    w("k = 1 standard uncertainties, combined RSS; limits compared at k = 2 unless stated.")
    w("")

    # ------------------------------------------------------------------ 1. operating point
    w("1. Operating point and conversions")
    w(f"   force winding Kf {KF:.2f} N/A; 1 mg on the pan = {I_PER_MG * 1e9:.2f} nA = {V_PER_MG * 1e6:.3f} uV on "
      f"{R_S:.0f} ohm; 1 ppm of ADC FSR = {MG_PER_PPM_FSR:.3f} mg")
    w(f"   ADC1 input (normalised to VREF): empty pan {float(v_of(0)):.4f}, 520 g {float(v_of(W_FS)):.4f} "
      f"({float(v_of(W_FS)) * V_REF:.3f} V); 1 ppm of FS = {W_FS * 1e-3:.2f} mg")
    w("")

    # ------------------------------------------------------------------ 2. noise / readout
    w("2. Noise per reading (SM loop + noise model; sinc^k readout after the ADS at 1200 SPS)")
    profiles = [("dispensing sinc^2 x 0.10 s", 2, 0.10), ("settling sinc^3 x 0.33 s", 3, 0.33),
                ("high-load sinc^3 x 0.66 s", 3, 0.66), ("high-load sinc^3 x 0.90 s", 3, 0.90)]
    loads = (0.0, 10.0, 100.0, 200.0, 500.0)
    w(f"   {'profile':<28}{'vibration':<20}" + "".join(f"{x:>8.0f} g" for x in loads) + "   settle 10 g / 500 g")
    SD = {}
    for name, k, T in profiles:
        for vname, vm in VIB_CASES.items():
            vals = [sd_noise(x, T, k, vm) for x in loads]
            for x, v in zip(loads, vals):
                SD[(name, vname, x)] = v
            st = f"{settle_time(T):.2f} / {settle_time(T, True):.2f} s" if k == 3 else "0.59 s (SM)"
            w(f"   {name:<28}{vname:<20}" + "".join(f"{v:>10.3f}" for v in vals) + f"   {st}")
    w("   (mg SD).  MET-05: settling profile 1.14 s at 10 g (goal 1.5 s); a 0.90 s window settles a 500 g step in")
    w(f"   ~{settle_time(0.90, True):.1f} s (requirement 3 s), so a load-adaptive window is allowed at high load.")
    zd60 = drift_noise(0.0, 0.33, 3, 60.0)
    w(f"   zero change over 60 s from noise alone (settling profile, empty pan): {zd60:.3f} mg SD")
    checks.append(("SM reproduced: settling SD 10 g 0.032 mg, 500 g 0.172 mg (within 5 %)",
                   abs(SD[("settling sinc^3 x 0.33 s", "x1 (SM quiet slab)", 10.0)] / 0.032 - 1) < 0.05 and
                   abs(SD[("settling sinc^3 x 0.33 s", "x1 (SM quiet slab)", 500.0)] / 0.172 - 1) < 0.05))
    w("")

    # ------------------------------------------------------------------ 3. temperature estimation
    w("3. Magnet temperature estimate (feeds every span-temperature term)")
    n_tmp = TMP_RATE * EST_WIN
    sig_tmp = 2 * TMP_SIG / math.sqrt(n_tmp)
    n_ntc = NTC_RATE * EST_WIN
    sig_ntc = 2 * (ADC2_NOISE / NTC_SENS) / math.sqrt(n_ntc)
    a28 = abs(ALPHA["Recoma 28"])
    w(f"   TMP117, continuous, AVG 8 ({TMP_RATE:.0f} results/s, {TMP_SIG * 1e3:.1f} mK each), {EST_WIN:.0f} s ramp-fit "
      f"estimator (value at the window end, no ramp lag): {sig_tmp * 1e3:.2f} mK -> {a28 * sig_tmp * 1e6:.2f} ppm "
      f"({a28 * sig_tmp * W_FS * 1e3:.3f} mg at 520 g)")
    w(f"   10k NTC on ADS1263 ADC2 (shared, {NTC_RATE} SPS effective), same estimator: {sig_ntc * 1e3:.3f} mK "
      f"-> {a28 * sig_ntc * 1e6:.3f} ppm.  TMP117 noise ~2 LSB p-p dithers its 7.8 mK quantisation; the NTC is the")
    w("   upgrade path if the rig shows quantisation steps.  Both are self-heated by a constant amount (tracked offset).")
    w("   Lag of the magnet volume behind a sensor on the pot steel, ramp r (K/h) -> span error |alpha| r tau:")
    w(f"   {'tau':<34}{'r':>6}{'uncompensated':>16}{'compensated (x' + str(LAG_RESID) + ')':>22}")
    LAG = {}
    for tname, tau in TAU_MAG.items():
        for rname, r in RAMP.items():
            e = a28 * r / 3600 * tau
            LAG[(tname, rname)] = e
            w(f"   {tname + ' ' + str(tau) + ' s':<34}{r:>6.1f}{e * 1e6:>13.2f} ppm{e * LAG_RESID * 1e6:>19.2f} ppm")
    w("   Two-sided: in an ambient ramp the steel pot is ~isothermal and heats the magnet through both bonded")
    w("   faces (volume lag L^2/12a = 4 s plus ~0.32 K/W bond lines x 29 J/K / 2 = 5 s).  Keep the sensor on the pot")
    w("   base directly under the magnet (or on the magnet face through a laser-cut hole in the base).")
    tau_chk = 4 * (12.7e-3) ** 2 / (math.pi**2 * (10 / (8400 * 350)))
    checks.append(("lag model reproduces MC §7: 3.54 ppm at 2 K/h with tau = 4h^2/(pi^2 a) = 19 s",
                   abs(a28 * 2 / 3600 * tau_chk * 1e6 - 3.54) < 0.05))
    w("")

    # ------------------------------------------------------------------ 4. MET-03
    w("4. MET-03 repeatability (SD of 10 loadings; zero re-checked between loadings [ASSUMED test practice])")
    sxf, syf = ecc_sens(DH_FINE, DY_FINE)
    sxd, syd = ecc_sens(DH_DOSE, DY_DOSE)
    w(f"   eccentric sensitivity after the fine corner adjust (MET-09 level): x {sxf * 1e3:.3f}, y {syf * 1e3:.3f} ppm/mm; "
      f"after the coarse (dosing) adjust: x {sxd * 1e3:.2f}, y {syd * 1e3:.2f} ppm/mm")
    w(f"   placement scatter {SIG_PLACE * 1e3:.1f} mm per axis [ASSUMED]")
    met03 = {}
    hdr = f"   {'load / corners / vibration / profile':<60}{'noise':>7}{'place':>7}{'temp':>7}{'draft':>7}{'total':>8}  vs 1 mg"
    w(hdr)
    for W in (10.0, 500.0):
        for corners, (sx, sy) in (("fine", (sxf, syf)), ("coarse", (sxd, syd))):
            for vname in VIB_CASES:
                for pname in ("settling sinc^3 x 0.33 s", "high-load sinc^3 x 0.90 s"):
                    if W < 100 and pname.startswith("high"):
                        continue
                    noise = math.sqrt(SD[(pname, vname, W)] ** 2 + SD[(pname, vname, 0.0)] ** 2)
                    place = W * 1e3 * math.hypot(sx, sy) * SIG_PLACE
                    temp = a28 * sig_tmp * W * 1e3
                    tot = rss(noise, place, temp, DRAFT)
                    met03[(W, corners, vname, pname)] = tot
                    if W < 100 and (corners == "coarse" or vname != "x1 (SM quiet slab)"):
                        continue
                    if corners == "coarse" and (vname != "x1 (SM quiet slab)" or pname.startswith("high")):
                        continue
                    tag = "OK" if tot <= MET03_SD else "FAIL"
                    w(f"   {f'{W:.0f} g / {corners} / {vname} / {pname[:21]}':<60}{noise:>7.3f}{place:>7.3f}{temp:>7.3f}"
                      f"{DRAFT:>7.3f}{tot:>8.3f}  {tag}")
    VIB_LIMIT = {}
    for pname, k_, T_ in (("settling sinc^3 x 0.33 s", 3, 0.33), ("high-load sinc^3 x 0.66 s", 3, 0.66),
                          ("high-load sinc^3 x 0.90 s", 3, 0.90)):
        lo, hi = 0.1, 100.0
        fixed = rss(500e3 * math.hypot(sxf, syf) * SIG_PLACE, a28 * sig_tmp * 500e3, DRAFT)
        for _ in range(60):
            mid = math.sqrt(lo * hi)
            tot = rss(math.hypot(sd_noise(500.0, T_, k_, mid), sd_noise(0.0, T_, k_, mid)), fixed)
            lo, hi = (mid, hi) if tot < MET03_SD else (lo, mid)
        VIB_LIMIT[pname] = lo
        w(f"   floor vibration at which the 500 g SD reaches {MET03_SD:.0f} mg ({pname}): {lo:.1f} x the SM spectrum")
    w("   -> 10 g: large margin.  500 g: needs the FINE corner adjust (placement scatter alone is "
      f"{500e3 * math.hypot(sxd, syd) * SIG_PLACE:.1f} mg with the dosing-level adjust)")
    w(f"      and a floor within {VIB_LIMIT['settling sinc^3 x 0.33 s']:.1f}x of the SM spectrum, or "
      f"{VIB_LIMIT['high-load sinc^3 x 0.90 s']:.1f}x with a 0.9 s window above ~100 g (load-adaptive, MET-05 allows 3 s).")
    w("")

    # ------------------------------------------------------------------ 5. MET-08
    w("5. MET-08 zero stability over a 60 s dosing cycle (stable temperature = |dT/dt| <= 0.5 K/h at the cell)")
    vx_coil = COIL_SIDE_GMM / RHO_COIL_SIDE
    vx_load = TARE_GMM / RHO_AL + M_CW * X_CW / RHO_STEEL
    vx_net = (vx_coil - vx_load) * 1e-9          # m^4 (cm^3 mm = 1e-9 m^4)
    buoy_per_rho = vx_net / (A_LOAD_MM * 1e-3) * 1e6   # mg per (kg/m^3)
    buoy_tc = buoy_per_rho * RHO_AIR / 293.15        # mg/K
    w(f"   air buoyancy of the unbalanced lever volumes: net volume moment {vx_net * 1e9:.0f} cm^3 mm -> "
      f"{buoy_per_rho * 0.012:.2f} mg per 1 % air density, {buoy_tc:.2f} mg/K [ASSUMED densities]; tared every throw")
    MET08 = {}
    for hk in ("baseline", "light"):
        z100 = ZERO_PER_100NM[hk]
        null = z100 * NULL_DRIFT_60S / 100e-9
        zero_tc = buoy_tc + z100 * NULL_DRIFT_TC / 100e-9
        therm = zero_tc * STABLE_RAMP / 3600 * 60
        tare = SD[("settling sinc^3 x 0.33 s", "x1 (SM quiet slab)", 0.0)]
        tot = rss(null, zd60, therm, DRAFT_DOSING, tare)
        MET08[hk] = (tot, null, zero_tc)
        w(f"   {HINGE_LABEL[hk]:<58} null {null:.3f}, noise {zd60:.3f}, thermal {therm:.3f} "
          f"(zero tempco {zero_tc:.2f} mg/K), dosing drafts {DRAFT_DOSING:.2f}, tare {tare:.3f}")
        w(f"   {'':<58} total {tot:.3f} mg (k=1), {2 * tot:.2f} mg (k=2) vs {MET08_LIM:.0f} mg "
          f"{'OK' if 2 * tot <= MET08_LIM else 'FAIL'}")
    w(f"   -> the optical-null drift dominates.  Requirement on the sensor: <= {NULL_DRIFT_60S * 1e9:.0f} nm per 60 s "
      f"(rig, OI-31) and <= {NULL_DRIFT_TC * 1e6:.1f} um/K thermal path.")
    w("")

    # ------------------------------------------------------------------ 6. MET-04 linearity (Monte Carlo)
    w("6. MET-04 linearity after the N-point linearity adjustment (Monte Carlo, 4000 draws per case)")
    Wg = np.arange(0.0, W_FS + 0.5, 2.0)
    vW = v_of(Wg)
    vv = np.linspace(-1, 1, 801)
    K_SER = 8
    ks = np.arange(1, K_SER + 1)

    def basis(v):
        return np.sin(np.outer(ks, np.pi * (np.asarray(v) + 1) / 2))   # K x n, zero at v = +/-1 (endpoint INL)

    B_full, B_W = basis(vv), basis(vW)
    # deterministic shapes
    beta = ((K_FULL / K_EMPTY) ** 2 - 1) / W_FS
    hsq = np.sqrt(1 + beta * Wg) - 1
    dev = hsq - hsq[-1] * Wg / W_FS
    shape_th = dev / np.max(np.abs(dev))                     # tension-stiffening (theta0 and lever) shape
    rel = -REL_FS * (Wg + TRIM_G) ** 2 / (W_FS + TRIM_G) * 1e3   # mg, reluctance (quadratic in coil force)
    rel -= rel[0]
    kfx = KFX_QUAD_FS * (Wg + TRIM_G) ** 2 / (W_FS + TRIM_G) * 1e3
    kfx -= kfx[0]
    pcr = PCR_FS * (Wg + TRIM_G) ** 2 / (W_FS + TRIM_G) * 1e3
    pcr -= pcr[0]
    cal_sets = {
        "3-point (0, 250, 520 g)": ([0.0, 250.0, 520.0], [[], ["E2_200", "F1_50"], ["E2_500", "F1_20"]], 2),
        "5-point (0, 120, 250, 370, 520 g)": ([0.0, 120.0, 250.0, 370.0, 520.0],
                                              [[], ["F1_100", "F1_20"], ["E2_200", "F1_50"],
                                               ["E2_200", "F1_100", "F1_50", "F1_20"], ["E2_500", "F1_20"]], 3),
        "9-point (0..520 g, D-030 combinations)": ([0.0, 70.0, 120.0, 170.0, 250.0, 320.0, 370.0, 500.0, 520.0],
                                                   [[], ["F1_50", "F1_20"], ["F1_100", "F1_20"],
                                                    ["F1_100", "F1_50", "F1_20"], ["E2_200", "F1_50"],
                                                    ["E2_200", "F1_100", "F1_20"], ["E2_200", "F1_100", "F1_50", "F1_20"],
                                                    ["E2_500"], ["E2_500", "F1_20"]], 5),
    }

    def sd_point(W):
        noise = math.sqrt(sd_noise(W, 0.33, 3) ** 2 + sd_noise(0.0, 0.33, 3) ** 2)
        place = W * 1e3 * math.hypot(sxf, syf) * SIG_PLACE
        return rss(noise, place, DRAFT) / math.sqrt(N_REP_CAL)

    def fit_map(Wc, deg):
        Wc = np.asarray(Wc)
        V_c = np.vander(Wc / W_FS, deg + 1)
        V_g = np.vander(Wg / W_FS, deg + 1)
        return V_g @ np.linalg.pinv(V_c)                       # grid values of the fitted polynomial

    N_MC = 4000
    SDP = {x: sd_point(x) for cs in cal_sets.values() for x in cs[0] if x > 0}

    def draw_inl(pw, A):
        c = rng.normal(0, 1, (N_MC, K_SER)) / ks**pw
        norm = np.max(np.abs(c @ B_full), axis=1)
        inl = (c @ B_W) / norm[:, None] * A * MG_PER_PPM_FSR   # mg at each grid load
        return inl - inl[:, :1]

    def draw_mech():
        return ((rng.choice([-1, 1], N_MC) * THETA0_NL)[:, None] * shape_th
                + (rng.choice([-1, 1], N_MC) * LEVER_NL)[:, None] * shape_th
                + rel[None, :] + rng.choice([-1, 1], N_MC)[:, None] * kfx[None, :] + pcr[None, :])

    def lin_mc(e_true, cname, masses_on=True, noise_on=True):
        Wc, masses, deg = cal_sets[cname]
        errs = {"E2_500": rng.normal(0, U_E2[500], N_MC), "E2_200": rng.normal(0, U_E2[200], N_MC),
                "F1_100": rng.uniform(-F1_MPE[100], F1_MPE[100], N_MC),
                "F1_50": rng.uniform(-F1_MPE[50], F1_MPE[50], N_MC),
                "F1_20": rng.uniform(-F1_MPE[20], F1_MPE[20], N_MC)}
        cols = []
        for x, ms in zip(Wc, masses):
            col = e_true[:, int(round(x / 2.0))].copy()
            if masses_on:
                col += sum((errs[m] for m in ms), np.zeros(N_MC))
            if noise_on and x > 0:
                col += rng.normal(0, SDP[x], N_MC)
            cols.append(col)
        P = np.stack(cols, axis=1) @ fit_map(Wc, deg).T
        resid = e_true - P
        resid -= resid[:, :1]                                  # re-zeroed (tare)
        mx = np.max(np.abs(resid), axis=1)
        return float(np.median(mx)), float(np.percentile(mx, 95)), resid

    w(f"   ADC INL = A x random sine series on [-FSR, +FSR] (endpoint-zero), normalised to peak A; 'smooth' "
      f"sigma_k ~ 1/k^2, 'rough' ~ 1/k, k = 1..{K_SER}.")
    w("   Plus theta0 (0.36 mg) and lever (0.08 mg) tension-stiffening shapes, reluctance (76 mg quadratic), "
      "Kf(x) and PCR quadratics; masses per D-030 (E2 certified, F1 nominal); 5 repeats per point.")
    # decomposition of the typical case
    inl_s3 = draw_inl(2, 3.0)
    mech = draw_mech()
    zero_e = np.zeros_like(mech)
    w("   what drives the typical case (smooth INL, A = 3 ppm FSR, 3-point), p95 of max |error| (mg):")
    for lab, e_t, mo, no in (("INL only", inl_s3, False, False), ("mechanics + magnetics only", mech, False, False),
                             ("calibration-mass errors only", zero_e, True, False),
                             ("repeatability noise only", zero_e, False, True), ("all together", inl_s3 + mech, True, True)):
        _, p95, _ = lin_mc(e_t, "3-point (0, 250, 520 g)", mo, no)
        w(f"     {lab:<34}{p95:>6.2f}")
    lin_results = {}
    nl_drift_fs = {}
    w(f"   {'INL family':<10}{'A ppm FSR':>10}  {'adjustment':<40}{'median':>8}{'p95':>8}  max |error| over 0..520 g (mg)")
    for fam, pw in (("smooth", 2), ("rough", 1)):
        for A in (3.0, 12.0):
            inl_W = draw_inl(pw, A)
            e_true = inl_W + draw_mech()
            for cname in cal_sets:
                med, p95, resid = lin_mc(e_true, cname)
                lin_results[(fam, A, cname)] = (med, p95, resid)
                w(f"   {fam:<10}{A:>10.0f}  {cname:<40}{med:>8.2f}{p95:>8.2f}")
            # nonlinearity drift seen at FS after a 200 g span adjust (operational budget input)
            dT = rng.uniform(-DT_LIN, DT_LIN, N_MC)
            de = (INL_TC * dT)[:, None] * inl_W + (REL_TC * dT)[:, None] * rel[None, :]
            i200, i520 = int(W_CM / 2), int(W_FS / 2)
            nl_drift_fs[(fam, A)] = de[:, i520] - (W_FS / W_CM) * de[:, i200]
    # pure-quadratic check of the 3-point map
    Mq = fit_map([0.0, 250.0, 520.0], 2)
    qv = 1e-4 * Wg**2
    checks.append(("3-point adjustment removes a pure quadratic exactly (residual < 1e-9 mg)",
                   float(np.max(np.abs(qv - Mq @ (1e-4 * np.array([0.0, 250.0, 520.0]) ** 2)))) < 1e-9))
    c3, c5, c9 = list(cal_sets)
    lin_base = lin_results[("smooth", 3.0, c3)]
    w(f"   + ascending/descending hysteresis {HYST_LIN:.1f} mg [ASSUMED] added linearly; smallest set meeting +/-{MET04_LIM:.0f} mg at p95:")
    LIN_PICK = {}
    for fam in ("smooth", "rough"):
        for A in (3.0, 12.0):
            ok = [cn for cn in cal_sets if lin_results[(fam, A, cn)][1] + HYST_LIN <= MET04_LIM]
            pick = ok[0] if ok else "none of these"
            LIN_PICK[(fam, A)] = pick
            vals = ", ".join(f"{lin_results[(fam, A, cn)][1] + HYST_LIN:.2f}" for cn in cal_sets)
            w(f"     {fam:<7} INL, A = {A:>4.0f} ppm FSR: {vals} mg (3/5/9-point) -> {pick}")
    w("   -> Typical INL: 3 points.  Datasheet-max INL: 5 points if smooth, 9 points if rough.  The 9-point set uses")
    w("      only the D-030 masses, so make it the commissioning baseline and keep the M3 PWM-DAC fallback for an INL")
    w("      that even 9 points cannot follow.  OI-34 measures the shape first.")
    w("")

    # ------------------------------------------------------------------ 7. span model learning, MET-06 static
    w("7. Span-temperature model learned in situ (D-062) and MET-06 (static, after compensation, no recalibration)")
    sd200 = {v: SD[("settling sinc^3 x 0.33 s", v, 200.0)] for v in VIB_CASES}
    sig_adj = {v: rss(sd200[v] / math.sqrt(N_ADJ) / (W_CM * 1e3), a28 * sig_tmp) for v in VIB_CASES}
    for v in VIB_CASES:
        w(f"   one auto-adjust, vibration {v:<20}: SD at 200 g {sd200[v]:.3f} mg, /sqrt({N_ADJ}) -> "
          f"{sd200[v] / math.sqrt(N_ADJ) / (W_CM * 1e3) * 1e6:.2f} ppm; with temperature noise {sig_adj[v] * 1e6:.2f} ppm")
    w(f"   (EMFC_MECHANICS §8 used the MET-03 limit, 1 mg, giving 1.7 ppm; the predicted SD is 5-10x smaller.)")
    sa = sig_adj["x1 (SM quiet slab)"]
    sa10 = sig_adj["x10 (SM bad case)"]
    lag_learn = a28 * STABLE_RAMP / 3600 * TAU_MAG["two-sided, bonded faces [EB]"] * LAG_RESID
    for dTl in (2.0, 5.0, 10.0):
        ua = rss(math.sqrt(2) * sa / dTl, 2 * lag_learn / dTl)
        ua10 = rss(math.sqrt(2) * sa10 / dTl, 2 * lag_learn / dTl)
        w(f"   alpha from two adjusts {dTl:4.0f} K apart (learning only when |dT/dt| < {STABLE_RAMP} K/h): "
          f"u = {ua * 1e6:.3f} ppm/K ({ua / a28 * 100:.3f} % of alpha); x10 vibration {ua10 * 1e6:.3f} ppm/K")
    u_alpha = rss(math.sqrt(2) * sa / 5.0, 2 * lag_learn / 5.0)
    checks.append(("in-situ alpha formula reproduces MC §12d: 0.14 % at 5 K with sigma_adj 1.7 ppm",
                   abs(math.sqrt(2) * 1.7e-6 / 5 / a28 * 100 - 0.14) < 0.01))
    # static MET-06 terms (Recoma 28)
    curv_lin = DALPHA_DT["Recoma 28"] * 0.5 * (20 + 30 - 2 * 22.5)      # slope error on a 20-30 C test, linear model
    curv_quad = 0.2 * curv_lin
    ae_typ = ADC_GAIN_TC[0] + REF_TC[0] + SHUNT_TC[0]
    ae_max = ADC_GAIN_TC[1] + REF_TC[1] + SHUNT_TC[1]
    elec_track = {"typ, tracking 80 %": ae_typ * NONTRACK, "max, tracking 80 %": ae_max * NONTRACK,
                  "B4 heat test (separate alpha_e)": AE_CHAR}
    coil_tc = {k_: COIL_AXIAL_TC * KF_CURV * x0 for k_, x0 in COIL_SET.items()}
    null_span_tc = NULL_DRIFT_TC / S_VANE * SPAN_PER_THETA0
    hyst_tc = TMP_HYST * a28 / 10.0
    w(f"   span tempco paths besides the magnet (all learned with alpha while they track the magnet sensor):")
    w(f"     electronics alpha_e typ {ae_typ * 1e6:.2f} / max {ae_max * 1e6:.2f} ppm/K; coil axial position "
      + ", ".join(f"{k_} {v * 1e6:.2f}" for k_, v in coil_tc.items())
      + f" ppm/K; optical null via theta0 {null_span_tc * 1e6:.2f} ppm/K")
    w(f"   residual coefficient = what does NOT track (fraction {NONTRACK:.0%} [ASSUMED]) + learning error + curvature:")
    rows = []
    for label, cv, el, ck in (("as designed now (linear model, alpha_e typ, coil +/-50 um)", curv_lin,
                               elec_track["typ, tracking 80 %"], "+/-50 um (MC)"),
                              ("after >= 10 K of history (quadratic model)", curv_quad,
                               elec_track["typ, tracking 80 %"], "+/-50 um (MC)"),
                              ("+ B4 heat test + coil set +/-20 um", curv_quad,
                               elec_track["B4 heat test (separate alpha_e)"], "+/-20 um (EB)"),
                              ("worst electronics (alpha_e max), quadratic model", curv_quad,
                               elec_track["max, tracking 80 %"], "+/-50 um (MC)")):
        mech = rss(coil_tc[ck] * NONTRACK, null_span_tc * NONTRACK)
        tot = rss(u_alpha, cv, el, mech, hyst_tc)
        rows.append((label, tot))
        w(f"     {label:<58} learn {u_alpha * 1e6:.2f}, curvature {cv * 1e6:.2f}, electronics {el * 1e6:.2f}, "
          f"mech {mech * 1e6:.2f}, sensor hyst {hyst_tc * 1e6:.2f} -> {tot * 1e6:.2f} ppm/K (k=1), "
          f"{2 * tot * 1e6:.2f} (k=2) {'OK' if 2 * tot * 1e6 <= MET06_LIM else 'FAIL'}")
    met06 = dict(rows)
    a_tc = abs(ALPHA["TC16"])
    curv_tc = DALPHA_DT["TC16"] * 0.5 * (20 + 30 - 2 * 22.5)
    tot_tc = rss(curv_tc, elec_track["typ, tracking 80 %"], rss(coil_tc["+/-50 um (MC)"] * NONTRACK,
                                                               null_span_tc * NONTRACK), TMP_HYST * a_tc / 10.0)
    w(f"   TC16 magnet, linear model only: curvature {curv_tc * 1e6:.2f} ppm/K [ASSUMED] -> {tot_tc * 1e6:.2f} ppm/K (k=1)")
    w(f"   -> With Recoma 28, MET-06 (static) needs the quadratic term, i.e. >= 10 K of adjust history; until then the")
    w(f"      linear-model curvature error ({curv_lin * 1e6:.1f} ppm/K on a 20-30 C test) fails it and MET-07 carries")
    w("      the accuracy.  Commissioning can force the history with a deliberate +10 K enclosure soak.")
    w("")

    # ------------------------------------------------------------------ 8. operational span with MET-07
    w("8. Span error at 520 g in use, with auto-adjust (MET-07) - the number that matters for full-range accuracy")
    sd500 = SD[("settling sinc^3 x 0.33 s", "x1 (SM quiet slab)", 500.0)]
    u_cm = rss(U_E2[500] / 500e3, sd500 / math.sqrt(N_REP_CAL) / 500e3, sd200["x1 (SM quiet slab)"]
               / math.sqrt(N_ADJ) / (W_CM * 1e3))
    resid_coef = rss(u_alpha, curv_quad, elec_track["typ, tracking 80 %"],
                     rss(coil_tc["+/-20 um (EB)"] * NONTRACK, null_span_tc * NONTRACK))
    t_age = REF_AGING * (math.sqrt((BURN_IN_H + ADJ_TIME_H) / 1000) - math.sqrt(BURN_IN_H / 1000))
    p_term = a28 * P_ERR * P0["Recoma 28"] * R_POT * P_UNCOMP
    nl_rms = float(np.sqrt(np.mean(nl_drift_fs[("smooth", 3.0)] ** 2))) / (W_FS * 1e3)
    nl_rms_w = float(np.sqrt(np.mean(nl_drift_fs[("rough", 12.0)] ** 2))) / (W_FS * 1e3)

    def op_budget(trigger_K, ramp, tau_key, vib_key, nl):
        terms = {
            "check-mass value (external E2 500 g transfer), systematic": u_cm,
            "one adjust, random (200 g, 10 readings, temperature noise)": sig_adj[vib_key],
            f"drift since the last adjust (<= {trigger_K} K x residual coefficient, uniform)": resid_coef * trigger_K / math.sqrt(3),
            "magnet lag after a ramp change (compensated)": LAG[(tau_key, ramp)] * LAG_RESID,
            "nonlinearity drift extrapolated 200 g -> 520 g": nl,
            f"aging between adjusts ({ADJ_TIME_H:.0f} h trigger, after {BURN_IN_H:.0f} h burn-in)": t_age,
            "magnetic hysteresis after FS / pan-absent excursions": MAG_HYST,
            "coil-power regulation error not seen by the sensor": p_term,
            "shunt PCR not settled at the read time": PCR_FS * PCR_UNSETTLED,
        }
        return terms, rss(*terms.values())

    base_terms, base_tot = op_budget(0.5, "design", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms)
    for name_, val in base_terms.items():
        w(f"   {name_:<78}{val * 1e6:>7.2f} ppm")
    w(f"   {'total (k=1)':<78}{base_tot * 1e6:>7.2f} ppm = {base_tot * W_FS * 1e3:.2f} mg at 520 g; k=2 "
      f"{2 * base_tot * W_FS * 1e3:.2f} mg vs +/-{MET04_LIM:.0f} mg {'OK' if 2 * base_tot * W_FS * 1e3 <= MET04_LIM else 'FAIL'}")
    w("   sensitivity (k=2 at 520 g, mg):")
    variants = [("baseline (0.5 K trigger, 2 K/h, two-sided lag, x1 vib, smooth typ INL)",
                 (0.5, "design", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms)),
                ("one-sided lag (19 s)", (0.5, "design", "one-sided, sensor on base [MC]", "x1 (SM quiet slab)", nl_rms)),
                ("bad ramp 5 K/h", (0.5, "bad", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms)),
                ("x10 floor vibration", (0.5, "design", "two-sided, bonded faces [EB]", "x10 (SM bad case)", nl_rms)),
                ("rough max INL drifting", (0.5, "design", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms_w)),
                ("trigger 0.25 K", (0.25, "design", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms)),
                ("trigger 1.0 K", (1.0, "design", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms)),
                ("trigger 2.0 K", (2.0, "design", "two-sided, bonded faces [EB]", "x1 (SM quiet slab)", nl_rms))]
    OPS = {}
    for lab, args in variants:
        _, tot = op_budget(*args)
        OPS[lab] = tot
        w(f"     {lab:<72}{2 * tot * W_FS * 1e3:>6.2f}")
    w("   auto-adjust load on dosing throughput (45 s each):")
    for rname, r in RAMP.items():
        for trig in (0.5, 2.0):
            n_h = r / trig
            w(f"     ramp {rname:<8}{r:>4.1f} K/h, trigger {trig:.1f} K: {n_h:4.1f} adjusts/h -> "
              f"{n_h * ADJ_DURATION / 3600 * 100:4.1f} % of the time")
    w("   -> Full-range accuracy at 520 g is ~1 mg (k=2) with the 0.5 K trigger.  Dosing loads (<= 10 g) see 1/50 of")
    w("      every span term, so the dosing profile can use a 2 K trigger and lose < 2 % throughput.")
    w("")

    # ------------------------------------------------------------------ 9. eccentric
    w("9. MET-09 eccentric load and placement (mechanics allocation 1.0 mg of the 2 mg limit) [MECH §3]")
    ex = 175e3 * sxf * 25e-3
    ey = 175e3 * syf * 25e-3
    w(f"   fine adjust (dh {DH_FINE * 1e6:.2f} um, dy {DY_FINE * 1e6:.2f} um): {ex:.2f} mg (x) and {ey:.2f} mg (y) at 175 g, "
      f"e = 25 mm; adjuster lock-in stability is the open risk (OI-25)")
    e_dose_x = 10e3 * sxd * 5e-3
    e_dose_y = 10e3 * syd * 5e-3
    e_unadj_y = 10e3 * 2 * DY_UNADJ / W_G**2 * 5e-3
    w(f"   dosing (10 g, centroid within 5 mm): coarse adjust x {e_dose_x:.2f} mg, y {e_dose_y:.2f} mg;  "
      f"y UNADJUSTED ({DY_UNADJ * 1e3:.1f} mm roll-centre offset): {e_unadj_y:.0f} mg")
    w("   -> EMFC_MECHANICS gave the dosing case for x only.  The y adjuster must be set (<= ~10 um) for dosing too,")
    w("      and the fine adjust is needed for MET-03 at 500 g (§4), not only for MET-09.")
    w("")

    # ------------------------------------------------------------------ 10. warm-up
    w("10. MET-10 warm-up (<= 60 min to full spec)")
    tau_pot = R_POT * C_POT
    dT_coil = P0["Recoma 28"] * R_POT
    w(f"   pot + magnet: C {C_POT:.0f} J/K x {R_POT:.0f} K/W -> tau {tau_pot / 60:.1f} min; constant coil power "
      f"{P0['Recoma 28'] * 1e3:.0f} mW warms it {dT_coil * 1e3:.0f} mK "
      f"({dT_coil * a28 * 1e6:.0f} ppm before compensation); left after 60 min: "
      f"{dT_coil * math.exp(-3600 / tau_pot) * 1e3:.1f} mK, tracked by the sensor")
    for P_int, UA, tau_enc in ((2.0, 0.8, 90.0), (4.0, 0.8, 90.0), (8.0, 1.0, 120.0)):
        rise = P_int / UA
        ramp60 = rise / (tau_enc / 60) * math.exp(-60 / tau_enc)
        w(f"   enclosure: {P_int:.0f} W internal, UA {UA:.1f} W/K, tau {tau_enc:.0f} min [ASSUMED, power budget pending] -> "
          f"rise {rise:.1f} K, ramp at 60 min {ramp60:.2f} K/h, "
          f"{'inside' if ramp60 <= RAMP['design'] else 'OUTSIDE'} the 2 K/h design ramp")
    w("   -> Dosing is usable after ~10-20 min (B4 electronics settle; zero is re-tared each throw).  Full-range spec at")
    w("      60 min holds only if the cell compartment sees <= 2 K/h by then: keep the mains, controller and motor heat")
    w("      out of the cell compartment (power budget heat map), and run auto-adjust through warm-up.")
    w("")

    # ------------------------------------------------------------------ 11. dosing
    w("11. Dosing accuracy (DSP-04): one charge, re-tared, read in the settling profile after the motor stops")
    D = {}
    for hk in ("baseline", "light"):
        for Wc_g in (10.0, 0.324):
            zero = MET08[hk][0]
            acc = SD[("settling sinc^3 x 0.33 s", "x1 (SM quiet slab)", 10.0)]
            i10 = int(round(10.0 / 2.0))
            lin = float(np.sqrt(np.mean(lin_base[2][:, i10] ** 2))) if Wc_g >= 10 else 0.02
            ecc = Wc_g * 1e3 * math.hypot(sxd, syd) * SIG_CENTROID
            span = Wc_g * 1e3 * base_tot * 3
            sig = rss(zero, acc, lin, ecc, ESD, span)
            D[(hk, Wc_g)] = sig
            w(f"   {hk:<9}{Wc_g:>6.3f} g: zero {zero:.2f}, noise {acc:.3f}, linearity {lin:.3f}, eccentric {ecc:.3f}, "
              f"ESD {ESD:.2f} [ASSUMED], span {span:.3f} -> 3 sigma {3 * sig:.2f} mg = {3 * sig / GRAIN_MG:.4f} gr "
              f"(DSP-04 assumed {DSP04_3S:.0f} mg)")
    w("   -> Dosing is set by zero stability (optical-null drift) and electrostatics, not by span or magnet drift.")
    w("")

    # ------------------------------------------------------------------ 12. r = 15 vs 20
    w("12. Lever ratio r = 15 vs r = 20 (OI-02 / MAGNET_COIL §10 left this to the error budget)")
    w(f"   {'':<44}{'r = 15':>18}{'r = 20':>18}")
    p15 = P0["Recoma 28"]
    p20 = p15 * (15 / 20) ** 2
    rows12 = [("coil arm b", "150 mm", "200 mm"),
              ("constant coil power P0 (Recoma 28)", f"{p15 * 1e3:.0f} mW", f"{p20 * 1e3:.0f} mW"),
              ("zero per 100 nm, baseline / light hinges", f"{ZERO_PER_100NM['baseline']:.2f} / {ZERO_PER_100NM['light']:.2f} mg",
               f"{ZERO_PER_100NM_R20:.2f} / {ZERO_PER_100NM['light'] * ZERO_PER_100NM_R20 / ZERO_PER_100NM['baseline']:.2f} mg"),
              ("counterweight (option U) [MECH §2]", "187 g", "310 g"),
              ("lever bending modes, same section", "1.00", f"{(150 / 200) ** 2:.2f} x"),
              ("cell length", "base", "+50 mm")]
    for a_, b_, c_ in rows12:
        w(f"   {a_:<44}{b_:>18}{c_:>18}")
    m08_20 = rss(ZERO_PER_100NM_R20, zd60, MET08["baseline"][2] * ZERO_PER_100NM_R20 / ZERO_PER_100NM["baseline"]
                 * STABLE_RAMP / 60, DRAFT_DOSING, SD[("settling sinc^3 x 0.33 s", "x1 (SM quiet slab)", 0.0)])
    w(f"   MET-08 (k=2), baseline hinges: r = 15 {2 * MET08['baseline'][0]:.2f} mg, r = 20 {2 * m08_20:.2f} mg; "
      f"light hinges at r = 15: {2 * MET08['light'][0]:.2f} mg")
    w("   -> Keep r = 15.  The light hinge set buys more zero stability than r = 20 does, without a longer cell, a")
    w("      heavier counterweight or lower lever modes.  Coil power is already only 18 mW.")
    w("")

    # ------------------------------------------------------------------ 13. summary
    w("13. Summary against the requirements (Recoma 28 baseline, assumed environment)")
    rows13 = [
        ("MET-03 SD at 10 g", f"{met03[(10.0, 'fine', 'x1 (SM quiet slab)', 'settling sinc^3 x 0.33 s')]:.2f} mg (k=1)",
         "<= 1 mg", "OK"),
        ("MET-03 SD at 500 g, x1 / x10 floor",
         f"{met03[(500.0, 'fine', 'x1 (SM quiet slab)', 'settling sinc^3 x 0.33 s')]:.2f} / "
         f"{met03[(500.0, 'fine', 'x10 (SM bad case)', 'high-load sinc^3 x 0.90 s')]:.2f} mg (0.9 s window at x10)",
         "<= 1 mg", "OK if fine corners + floor <= ~10x"),
        ("MET-04 linearity (p95)", f"{lin_base[1] + HYST_LIN:.2f} mg typ INL 3-pt; max INL: "
         f"{lin_results[('smooth', 12.0, c5)][1] + HYST_LIN:.2f} smooth 5-pt, "
         f"{lin_results[('rough', 12.0, c9)][1] + HYST_LIN:.2f} rough 9-pt", "+/-2 mg", "OK with 9-pt baseline"),
        ("MET-06 static, now / best practice", f"{2 * met06['as designed now (linear model, alpha_e typ, coil +/-50 um)'] * 1e6:.1f} / "
         f"{2 * met06['+ B4 heat test + coil set +/-20 um'] * 1e6:.1f} ppm/K (k=2)", "+/-2 ppm/K", "after history only"),
        ("Span at 520 g in use (MET-07)", f"{2 * base_tot * W_FS * 1e3:.2f} mg (k=2)", "within +/-2 mg", "OK"),
        ("MET-08 zero over 60 s, baseline / light", f"{2 * MET08['baseline'][0]:.2f} / {2 * MET08['light'][0]:.2f} mg (k=2)",
         "<= 1 mg", "OK (null drift <= 100 nm)"),
        ("MET-09 eccentric", f"{rss(ex, ey):.2f} mg mech + adjust stability", "+/-2 mg", "needs fine adjusters (OI-25)"),
        ("MET-05 settling", "1.14 s at 10 g; ~2.7 s at 500 g with the 0.9 s window", "3 s (goal 1.5)", "OK"),
        ("DSP-04 dosing 3 sigma, 10 g", f"{3 * D[('baseline', 10.0)]:.2f} / {3 * D[('light', 10.0)]:.2f} mg", "~3 mg assumed",
         "better than assumed"),
    ]
    for a_, b_, c_, d_ in rows13:
        w(f"   {a_:<40}{b_:<70}{c_:<16}{d_}")
    w("")

    # ------------------------------------------------------------------ sanity checks
    checks.append(("1 mg on the pan = 19.6 nA (MECH §10: 131 nA at Kf 5 N/A, scaled to 33.3 N/A)",
                   abs(I_PER_MG * 1e9 - 131 * 5 / KF) < 0.2))
    checks.append(("FS shunt voltage 1.56 V matches SM §1", abs(float(v_of(W_FS)) * V_REF - 1.56) < 0.01))
    checks.append(("buoyancy: 1 % air density x net volume moment / a gives the printed mg value",
                   abs(0.012 * vx_net / (A_LOAD_MM * 1e-3) * 1e6 - buoy_per_rho * 0.012) < 1e-9))
    checks.append(("ramp-fit estimator: endpoint SD 2 sigma/sqrt(N) matches a direct least-squares simulation",
                   _check_rampfit(rng)))
    w("Sanity checks")
    for txt, ok in checks:
        w(f"   - {txt}. {'PASS' if ok else 'FAIL'}")
    w(f"   - runtime {time.time() - t_run:.0f} s")


def _check_rampfit(rng) -> bool:
    n = int(TMP_RATE * EST_WIN)
    t = np.arange(n)
    ends = []
    for _ in range(2000):
        y = 0.001 * t + rng.normal(0, 1.0, n)
        p = np.polyfit(t, y, 1)
        ends.append(np.polyval(p, t[-1]) - 0.001 * t[-1])
    return abs(np.std(ends) / (2 / math.sqrt(n)) - 1) < 0.08


if __name__ == "__main__":
    main()
    (ROOT / "analysis" / "error_budget.out.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
