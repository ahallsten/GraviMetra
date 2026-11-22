"""Phase 2: EMFC servo architecture and measurand study (Config E).

Run:  .venv/Scripts/python analysis/servo_measurand.py   (writes analysis/servo_measurand.out.txt; ~1 min)

Scope: loop architecture (analog / digital / hybrid), measurand (ADS126x + shunt vs PWM options),
constant-power implementation (D-058), coil driver (OI-04), coil temperature by resistance (D-060),
null acquisition, firmware-crash behaviour, readout filters for the two measurement profiles.

Methods (outside methods, standard control engineering, not from the user's mechanical textbook):
  - loop shaping and margins on exact complex frequency responses (PID + lead + low-pass, delays);
  - non-linear time-domain simulation: 2-DOF plant (lever + coil on a bracket), 10 kHz discrete
    controller (Tustin), DAC quantisation with sigma-delta error feedback, actuator limits, stops,
    sensor saturation;
  - noise: one-sided PSDs propagated through closed-loop transfer functions and the readout filter.

Tags: [DS] vendor datasheet (docs/datasheets.md), [MECH] docs/phase2/EMFC_MECHANICS.md,
[MC] docs/phase2/MAGNET_COIL.md + analysis/magnet_coil.out.txt, [REQ] docs/01_REQUIREMENTS.md,
[TRK] analysis/trickler_sizing.out.txt, [ASSUMED] engineering assumption to be measured on the rig.
"""
from __future__ import annotations

import math
import random
import sys
import time
from pathlib import Path

import numpy as np
import pint

u = pint.UnitRegistry()
ROOT = Path(__file__).resolve().parents[1]
G0 = 9.80665
lines: list[str] = []


def w(s: str = "") -> None:
    lines.append(s)
    print(s, flush=True)


# ================================================================== inputs
W_MAX = 520e-3                 # kg [REQ MET-01]
R_LEV = 15                     # [MECH D-050]
A_LOAD = 0.010                 # m  [MECH]
B_COIL = 0.150                 # m
S_VANE = 0.160                 # m  vane radius [MECH §10]
TRIM = 0.02                    # [MECH D-051]
M_TARE = 0.090                 # kg pan + receptor [MECH]
F_FS = W_MAX * (1 + TRIM) * G0 / R_LEV          # 0.3468 N at the coil
F_MAX = 1.2 * F_FS             # [MC] headroom
F_MIN = -0.2 * F_FS            # [this study] pan removal needs -15.3 % FS
J_EMPTY, J_FULL = 13407e-7, 13927e-7            # kg m^2 [MECH §10]
K_EMPTY, K_FULL = 59.0e-3, 66.1e-3              # N m/rad [MECH §10]
M_COIL_PLANT = 0.015           # kg coil mass included in J [MECH]
M_COIL = 0.0138                # kg actual coil + bobbin [MC §4]
C_SOLID = 2.03                 # N s/m at the coil, solid Al bobbin [MC §8]
ZETA_AIR = 0.005               # [ASSUMED] open-loop air damping, slit bobbin
F_BRACKET = 800.0              # Hz [ASSUMED] coil-on-bracket mode (requirement >= 500 Hz)
ZETA_BRACKET = 0.02            # [ASSUMED] Q = 25

KF_TURN = 0.0570               # N/A per turn, Recoma 28 nominal [MC §4]
L_TURN2 = 78.8e-9              # H per turn^2 (estimate) [MC §5]
R_CI, R_CO, H_C = 16.53e-3, 19.52e-3, 7.14e-3   # m coil section [MC §4]
FILL = 0.55                    # [MC ASSUMED]
RHO_CU, ALPHA_CU = 1.724e-8, 3.93e-3

FS_CTRL = 10_000.0             # Hz control rate [design choice]
T_CTRL = 1 / FS_CTRL
F_TIA = 5000.0                 # Hz sensor TIA + anti-alias [ASSUMED]
F_DAC = 1500.0                 # Hz DAC reconstruction, 2nd order zeta 0.7 [ASSUMED]
F_DRV = 3000.0                 # Hz coil current loop [ASSUMED]
DAC_BITS = 12                  # STM32G491 DAC1 [DS DS13122]

X_LIN = 50e-6                  # m fine sensor linear range at the vane [ASSUMED requirement]
X_SAT = 80e-6                  # m fine sensor saturates as X_SAT*tanh(x/X_SAT)
SENS_N = 1.0e-9                # m/sqrt(Hz) fine sensor white noise at the vane [ASSUMED]
SENS_CORNER = 0.1              # Hz 1/f corner [ASSUMED]
COARSE_RANGE = 0.6e-3          # m coarse (acquisition) range at the vane [ASSUMED requirement]
COARSE_GAIN_ERR = 0.05         # [ASSUMED]
X_STOP = 3e-3 * B_COIL         # m stop at the coil radius (+/-3 mrad) [MECH]

ADS_EN = 0.10e-6               # V/sqrt(Hz), ADS126x ADC1 gain 1 (Table 8-1: 0.995 uV at 400 SPS sinc4) [DS]
ADS_INL_TYP, ADS_INL_MAX = 3e-6, 12e-6          # of FSR, PGA bypassed [DS]
ADS_FSR = 5.0                  # V = 2 x VREF(2.5 V) [DS]
ADS_GAIN_DRIFT = (0.5e-6, 4e-6)                 # /K typ, max [DS]
ADS2_NOISE_10SPS = {1: 7.34e-6, 32: 0.28e-6}    # V rms, ADC2 sinc1 10 SPS [DS Table 8-3]
REF_TC_MAX = 2e-6              # /K LTC6655B [DS]
REF_PP = 0.25e-6               # 0.1-10 Hz peak-peak, relative [DS]
REF_HYST = 30e-6               # LS8 thermal hysteresis (-40..85 C) [DS]
SHUNT_TCR = 0.05e-6            # /K typ 0-60 C, VPG foil family [DS family; part TBD]
SHUNT_PCR = 5e-6               # at rated power [DS family]
SHUNT_PRATED = 0.25            # W [ASSUMED rating of the chosen part]
OPA_ISC = 60e-3                # A OPA2388 short-circuit current [DS]

M_KERNEL = 2.5e-6              # kg largest kernel (long extruded) [TRK]
H_DROP = 0.040                 # m [ASSUMED] trickler outlet height above the pan
FLOW_COARSE = 0.788e-3         # kg/s [TRK]

# Floor vertical acceleration amplitude density, m/s^2/sqrt(Hz) [ASSUMED quiet slab-on-grade garage,
# no machinery running; x10 case = compressor / footsteps].  Measure on the rig (the EMFC at full
# load is itself the best seismometer).
VIB_PTS = [(0.01, 3e-6), (1.0, 3e-6), (5.0, 1e-4), (50.0, 1e-4), (300.0, 3e-5), (5000.0, 3e-6)]


def awg_d(n):
    return 0.127e-3 * 92 ** ((36 - n) / 39)


# ================================================================== helpers (frequency domain)
def lp1(s, f):
    return 1 / (1 + s / (2 * math.pi * f))


def lp2(s, f, z=0.7):
    wn = 2 * math.pi * f
    return wn**2 / (s**2 + 2 * z * wn * s + wn**2)


class Plant:
    def __init__(self, J, K, solid_bobbin, m_c=M_COIL, f_b=F_BRACKET, z_b=ZETA_BRACKET):
        self.M_tot = J / B_COIL**2
        self.M_L = self.M_tot - M_COIL_PLANT
        self.m_c = m_c
        self.K_L = K / B_COIL**2
        self.c_L = 2 * ZETA_AIR * math.sqrt(self.K_L * self.M_tot)
        self.c_coil = C_SOLID if solid_bobbin else 0.0
        self.k_b = m_c * (2 * math.pi * f_b) ** 2
        self.c_b = 2 * z_b * math.sqrt(self.k_b * m_c)
        self.gv = S_VANE / B_COIL

    def tf(self, s):
        """(vane displacement per coil force, vane displacement per lever disturbance force)."""
        a11 = self.m_c * s**2 + (self.c_coil + self.c_b) * s + self.k_b
        a12 = -(self.c_b * s + self.k_b)
        a22 = self.M_L * s**2 + (self.c_L + self.c_b) * s + self.K_L + self.k_b
        det = a11 * a22 - a12**2
        return self.gv * (-a12) / det, self.gv * (-a11) / det

    def modes(self):
        M = np.array([[self.m_c, 0], [0, self.M_L]])
        Kk = np.array([[self.k_b, -self.k_b], [-self.k_b, self.k_b + self.K_L]])
        ev = np.sort(np.linalg.eigvals(np.linalg.solve(M, Kk)).real)
        return np.sqrt(ev) / (2 * math.pi)


class Controller:
    """PID with lead: C = Kp (1 + wi/s) (1 + s/wz)/(1 + s/wp) * LP2(wf)."""

    def __init__(self, fc, plant, alpha=16.0, ri=10.0, rf=12.0, zf=0.7):
        self.fc, self.alpha, self.ri, self.rf, self.zf = fc, alpha, ri, rf, zf
        wc = 2 * math.pi * fc
        self.wz, self.wp = wc / math.sqrt(alpha), wc * math.sqrt(alpha)
        self.wi, self.wf = wc / ri, wc * rf
        self.Kp = 1.0
        s = 1j * wc
        self.Kp = 1 / abs(self.loop(s, plant))

    def C(self, s):
        return (self.Kp * (1 + self.wi / s) * (1 + s / self.wz) / (1 + s / self.wp)
                * lp2(s, self.wf / (2 * math.pi), self.zf))

    def electronics(self, s, digital=True):
        h = lp1(s, F_TIA) * lp2(s, F_DAC) * lp1(s, F_DRV)
        if digital:
            h = h * np.exp(-s * 1.5 * T_CTRL)     # ZOH (T/2) + one-sample computation delay
        return h

    def G(self, s, digital=True):
        return self.C(s) * self.electronics(s, digital)

    def loop(self, s, plant, digital=True):
        return self.G(s, digital) * plant.tf(s)[0]


def margins(f, L):
    mag, ph = np.abs(L), np.unwrap(np.angle(L))
    i = np.where((mag[:-1] >= 1) & (mag[1:] < 1))[0]
    fc = f[i[0]] if len(i) else float("nan")
    pm = 180 + math.degrees(ph[i[0]]) if len(i) else float("nan")
    # gain margin: first -180 deg crossing above crossover
    gm = float("inf")
    for k in range(len(f) - 1):
        if f[k] > fc and (ph[k] + math.pi) * (ph[k + 1] + math.pi) <= 0:
            gm = -20 * math.log10(mag[k])
            break
    smax = np.max(np.abs(1 / (1 + L)))
    return fc, pm, gm, smax


def vib_density(f):
    pf = np.array([p[0] for p in VIB_PTS])
    pa = np.array([p[1] for p in VIB_PTS])
    return np.exp(np.interp(np.log(f), np.log(pf), np.log(pa)))


def sinc_k(f, T, k):
    x = np.pi * f * T
    return np.abs(np.where(x == 0, 1.0, np.sin(x) / np.where(x == 0, 1, x))) ** k


# ================================================================== time-domain simulation
class Sim:
    """2-DOF plant at 50 kHz, controller at 10 kHz.  Coordinates: linear at the coil radius,
    +X = direction the coil force pushes; load pushes the lever in -X.  Stops at +/-X_STOP."""

    SUB = 5

    def __init__(self, plant, ctrl, kf, quantize=True, seed=1):
        self.p, self.c = plant, ctrl
        self.kf = kf
        self.quant = quantize
        self.lsb = (F_MAX - F_MIN) / 2**DAC_BITS
        self.rng = random.Random(seed)
        T = T_CTRL
        K = 2 / T
        wz, wp = ctrl.wz, ctrl.wp
        n0, n1 = 1 + K / wz, 1 - K / wz
        d0, d1 = 1 + K / wp, 1 - K / wp
        self.lead = (n0 / d0, n1 / d0, d1 / d0)
        wf, zf = ctrl.wf, ctrl.zf
        Kw = wf / math.tan(wf * T / 2)
        D = Kw**2 + 2 * zf * wf * Kw + wf**2
        self.bq = (wf**2 / D, 2 * wf**2 / D, wf**2 / D, (2 * wf**2 - 2 * Kw**2) / D, (Kw**2 - 2 * zf * wf * Kw + wf**2) / D)
        self.k_stop = plant.M_tot * (2 * math.pi * 300) ** 2
        self.c_stop = 2 * 0.3 * math.sqrt(self.k_stop * plant.M_tot)

    def run(self, t_end, Fd_fn, x0=0.0, F0=None, mode="track", sp_fn=None, impulses=(), crash=None,
            log_every=1, soft_off_tau=None):
        p, c = self.p, self.c
        dt = T_CTRL / self.SUB
        Fd0 = Fd_fn(0.0)
        if F0 is None:
            F0 = Fd0
        # state
        x1 = x2 = x0
        if x0 <= -X_STOP:
            x2 = -X_STOP - max(Fd0 - F0, 0) / self.k_stop
            x1 = x2
        v1 = v2 = 0.0
        # electronics states
        y_tia = x2 * p.gv
        dac_q = F0
        r1 = r2 = F0          # DAC reconstruction (2nd order) states: value, derivative
        r2 = 0.0
        F = F0
        # controller states
        integ = F0 if mode == "track" else 0.0
        e_prev = 0.0
        lead_x1 = lead_y1 = F0
        bq_x1 = bq_x2 = bq_y1 = bq_y2 = F0
        sd_e1 = sd_e2 = 0.0
        u_cmd = F0
        u_out = F0
        coil_on = True
        frozen = False
        imp = sorted(impulses)
        ip = 0
        wn_d, z_d = 2 * math.pi * F_DAC, 0.7
        a_drv = 2 * math.pi * F_DRV
        a_tia = 2 * math.pi * F_TIA
        n_steps = int(round(t_end / T_CTRL))
        log = {"t": [], "F": [], "xv": [], "stop": []}
        stop_hit = False
        peak_stop_force = 0.0
        sensor_src = "coarse" if mode == "acquire" else "fine"
        for k in range(n_steps):
            t = k * T_CTRL
            Fd = Fd_fn(t)
            # ---- controller sample
            xv = y_tia
            if sensor_src == "fine":      # fine channel, coarse channel takes over beyond its range
                y = X_SAT * math.tanh(xv / X_SAT)
                if abs(xv) > 1.5 * X_LIN:
                    sensor_src = "coarse"
            else:
                y = max(-COARSE_RANGE, min(COARSE_RANGE, xv * (1 + COARSE_GAIN_ERR)))
                if abs(xv) < 0.8 * X_LIN:
                    sensor_src = "fine"
                    y = X_SAT * math.tanh(xv / X_SAT)
            sp = sp_fn(t, y) if sp_fn else 0.0
            e = sp - y
            if crash and t >= crash[0]:
                frozen = True
            if crash and t >= crash[1]:
                coil_on = False
            if not frozen:
                # PI (Tustin) with conditional integration
                integ_new = integ + c.Kp * c.wi * T_CTRL * 0.5 * (e + e_prev)
                v = c.Kp * e + integ_new
                b0, b1, a1 = self.lead
                lo = b0 * v + b1 * lead_x1 - a1 * lead_y1
                lead_x1, lead_y1 = v, lo
                q0, q1, q2, p1, p2 = self.bq
                bo = q0 * lo + q1 * bq_x1 + q2 * bq_x2 - p1 * bq_y1 - p2 * bq_y2
                bq_x2, bq_x1, bq_y2, bq_y1 = bq_x1, lo, bq_y1, bo
                u_sat = min(F_MAX, max(F_MIN, bo))
                if not ((bo > F_MAX and e > 0) or (bo < F_MIN and e < 0)):
                    integ = integ_new
                e_prev = e
                u_cmd = u_sat
            # ---- plant sub-steps
            F_acc = 0.0
            for _ in range(self.SUB):
                if not frozen:
                    if self.quant:   # 2nd-order error feedback at 50 kHz (DAC by DMA): NTF = (1 - z^-1)^2
                        v_ = u_cmd - 2 * sd_e1 + sd_e2
                        q = round((v_ - F_MIN) / self.lsb) * self.lsb + F_MIN
                        q = min(F_MAX + 2 * self.lsb, max(F_MIN - 2 * self.lsb, q))
                        sd_e2, sd_e1 = sd_e1, q - v_
                        u_out = q
                    else:
                        u_out = u_cmd
                # DAC reconstruction 2nd order, then driver 1st order
                acc = wn_d**2 * (u_out - r1) - 2 * z_d * wn_d * r2
                r2 += acc * dt
                r1 += r2 * dt
                if coil_on:
                    F += a_drv * (r1 - F) * dt
                else:
                    F += -F * (dt / (soft_off_tau or 50e-6))
                fb = p.k_b * (x1 - x2) + p.c_b * (v1 - v2)
                fs = 0.0
                if x2 < -X_STOP:
                    fs = max(0.0, -self.k_stop * (x2 + X_STOP) - self.c_stop * v2)
                elif x2 > X_STOP:
                    fs = min(0.0, -self.k_stop * (x2 - X_STOP) - self.c_stop * v2)
                if fs != 0.0:
                    stop_hit = True
                    peak_stop_force = max(peak_stop_force, abs(fs))
                a1_ = (F - fb - p.c_coil * v1) / p.m_c
                a2_ = (fb - p.c_L * v2 - p.K_L * x2 - Fd + fs) / p.M_L
                v1 += a1_ * dt
                v2 += a2_ * dt
                x1 += v1 * dt
                x2 += v2 * dt
                y_tia += a_tia * (x2 * p.gv - y_tia) * dt
                F_acc += F
            while ip < len(imp) and imp[ip][0] <= t:
                v2 += -imp[ip][1] / p.M_L          # impulse in N s at the coil radius, -X
                ip += 1
            if k % log_every == 0:
                log["t"].append(t)
                log["F"].append(F_acc / self.SUB)
                log["xv"].append(x2 * p.gv)
                log["stop"].append(fs != 0.0)
        for key in log:
            log[key] = np.array(log[key])
        log["stop_hit"] = stop_hit
        log["peak_stop_force"] = peak_stop_force
        return log


def to_mg(F):
    """Coil force (N) to pan-mass equivalent (mg)."""
    return F * R_LEV / G0 * 1e6


def settle_time(t, y, final, tol, t0):
    """First time after t0 from which |y - final| stays <= tol."""
    bad = np.where((t >= t0) & (np.abs(y - final) > tol))[0]
    if len(bad) == 0:
        return 0.0
    if bad[-1] == len(t) - 1:
        return float("inf")
    return t[bad[-1] + 1] - t0


def readout(t, F, fs_out=1200.0, T=None, k=2):
    """ADS126x model: sinc4 (four cascaded moving averages of one output period) decimated to ~1200 SPS,
    then the profile filter: sinc^k FIR of window T (cascaded moving averages).  Causal."""
    step = int(round(1 / (fs_out * (t[1] - t[0]))))
    y = np.asarray(F, float)
    for _ in range(4):
        y = np.convolve(y, np.ones(step) / step, mode="full")[: len(F)]
    ys, ts = y[::step], t[::step]
    if T:
        m = max(1, int(round(T / (ts[1] - ts[0]))))
        for _ in range(k):
            ys = np.convolve(ys, np.ones(m) / m, mode="full")[: len(ys)]
    return ts, ys


# ================================================================== main
def main():
    t0_run = time.time()
    w("EMFC servo architecture and measurand study.  Inputs tagged [DS]/[MECH]/[MC]/[REQ]/[TRK]/[ASSUMED].")
    w("")

    # ------------------------------------------------------------ 1. winding configuration
    w("1. Coil winding for constant power (D-058) and the drive levels")
    d37 = awg_d(37)
    a37 = math.pi * d37**2 / 4
    A_c = (R_CO - R_CI) * H_C
    mlt = math.pi * (R_CI + R_CO)
    n_wire = FILL * A_c / (4 * a37)
    r_wire = RHO_CU * n_wire * mlt / a37
    w(f"   4 wires in hand, AWG 37 (d {d37 * 1e3:.4f} mm): {n_wire:.0f} turns each, {r_wire:.1f} ohm each "
      f"(window {A_c * 1e6:.1f} mm^2, fill {FILL}, MLT {mlt * 1e3:.1f} mm)")
    configs = {
        "all 4 in series, no heater": dict(N=4 * n_wire, R=4 * r_wire, Rh=None),
        "A+B force, C+D hairpin heater": dict(N=2 * n_wire, R=2 * r_wire, Rh=2 * r_wire),
    }
    w(f"   {'configuration':<32}{'N':>6}{'R ohm':>8}{'Kf N/A':>8}{'L mH':>7}{'Km':>6}{'I_FS mA':>9}{'I_max':>7}"
      f"{'I_min':>7}{'P_FS mW':>9}{'P_max':>7}")
    for name, cf in configs.items():
        cf["Kf"] = cf["N"] * KF_TURN
        cf["L"] = cf["N"] ** 2 * L_TURN2
        cf["Km"] = cf["Kf"] / math.sqrt(cf["R"])
        cf["I_fs"], cf["I_max"], cf["I_min"] = F_FS / cf["Kf"], F_MAX / cf["Kf"], F_MIN / cf["Kf"]
        cf["P_fs"], cf["P_max"] = cf["I_fs"] ** 2 * cf["R"], cf["I_max"] ** 2 * cf["R"]
        w(f"   {name:<32}{cf['N']:>6.0f}{cf['R']:>8.1f}{cf['Kf']:>8.2f}{cf['L'] * 1e3:>7.1f}{cf['Km']:>6.2f}"
          f"{cf['I_fs'] * 1e3:>9.2f}{cf['I_max'] * 1e3:>7.2f}{cf['I_min'] * 1e3:>7.2f}{cf['P_fs'] * 1e3:>9.1f}"
          f"{cf['P_max'] * 1e3:>7.1f}")
    cfg = configs["A+B force, C+D hairpin heater"]
    P0 = math.ceil(cfg["P_max"] * 1e3) * 1e-3
    tc_scale = 0.719 ** 2          # Km ratio TC16 min / Recoma 28 nom [MC §4] -> power x 1/0.517
    P0_tc = math.ceil(cfg["P_max"] / tc_scale * 1e3) * 1e-3
    vh = math.sqrt(P0 * cfg["Rh"])
    vh_tc = math.sqrt(P0_tc * cfg["Rh"])
    w(f"   Selected: A+B force winding, C+D non-inductive hairpin heater (same copper volume, same heat path).")
    w(f"   Constant total power P0 = {P0 * 1e3:.0f} mW (Recoma 28) / {P0_tc * 1e3:.0f} mW (TC16 min Br): heater "
      f"{(P0 - cfg['P_max']) * 1e3:.1f}..{P0 * 1e3:.0f} mW, up to {vh:.2f} / {vh_tc:.2f} V rms across "
      f"{cfg['Rh']:.0f} ohm.")
    w(f"   The all-series option (Km {configs['all 4 in series, no heater']['Km']:.2f}) is kept for rig A/B "
      f"tests of linear vs constant-power drive on the same coil.")
    R_S = 150.0
    v_s_max, v_s_fs, v_s_min = cfg["I_max"] * R_S, cfg["I_fs"] * R_S, cfg["I_min"] * R_S
    v_c_max = cfg["I_max"] * cfg["R"]
    w(f"   Shunt {R_S:.0f} ohm (foil): {v_s_min:+.2f} / {v_s_fs:+.2f} / {v_s_max:+.2f} V at -20 % / FS / +120 %; "
      f"coil {v_c_max:.2f} V at I_max; total compliance {v_s_max + v_c_max:.2f} V (+ L di/dt).")
    w(f"   Shunt power at I_max {cfg['I_max']**2 * R_S * 1e3:.1f} mW -> PCR shift "
      f"{SHUNT_PCR * cfg['I_max']**2 * R_S / SHUNT_PRATED * 1e6:.2f} ppm (quadratic in load).")
    w("")

    # ------------------------------------------------------------ 2. plant
    w("2. Plant (2-DOF: lever at the coil radius + coil on its bracket)")
    plants = {(lbl, sb): Plant(J, K, sb) for lbl, J, K in (("empty", J_EMPTY, K_EMPTY), ("full", J_FULL, K_FULL))
              for sb in (False, True)}
    pe = plants[("empty", False)]
    fm = pe.modes()
    w(f"   effective mass at the coil {pe.M_tot * 1e3:.1f} g (empty), lever stiffness {pe.K_L:.2f} N/m, "
      f"rigid-body f_n {fm[0]:.2f} Hz, coil-bracket mode {fm[1]:.0f} Hz (bracket alone {F_BRACKET:.0f} Hz) [ASSUMED]")
    for sb in (False, True):
        pl = plants[("empty", sb)]
        zeta = (pl.c_L + pl.c_coil) / (2 * math.sqrt(pl.K_L * pl.M_tot))
        w(f"   bobbin {'solid' if sb else 'slit '}: open-loop zeta {zeta:.3f}")
    w(f"   coil torque constant at the fulcrum Kf*b = {cfg['Kf'] * B_COIL:.2f} N m/A; vane 1 urad = {S_VANE * 1e9 / 1e3:.0f} nm")
    w("")

    # ------------------------------------------------------------ 3. loop design
    w("3. Loop design: digital PID + lead + 2nd-order low-pass at 10 kHz; electronics TIA 5 kHz, DAC recon 1.5 kHz,")
    w("   current loop 3 kHz [ASSUMED]; digital delay 1.5 samples.  Margins on exact frequency responses.")
    f = np.logspace(-3, 4, 6000)
    s = 2j * np.pi * f
    w(f"   {'f_c target':>10}{'plant':>14}{'f_c':>7}{'PM deg':>8}{'GM dB':>7}{'|S|max':>8}{'bracket |L| dB':>15}"
      f"{'Kp N/m':>9}")
    designs = {}
    for fc in (10, 20, 25, 30, 40):
        ctrl = Controller(fc, plants[("empty", False)])
        designs[fc] = ctrl
        for key in (("empty", False), ("full", False), ("empty", True)):
            pl = plants[key]
            L = ctrl.loop(s, pl)
            fcm, pm, gm, smax = margins(f, L)
            i_b = np.argmin(np.abs(f - fm[1]))
            lb = 20 * np.log10(np.max(np.abs(L[max(0, i_b - 60): i_b + 60])))
            tag = f"{key[0]}/{'solid' if key[1] else 'slit'}"
            w(f"   {fc:>10}{tag:>14}{fcm:>7.1f}{pm:>8.1f}{gm:>7.1f}{smax:>8.2f}{lb:>15.1f}{ctrl.Kp:>9.0f}")
    FC = 25
    ctrl = designs[FC]
    pl0 = plants[("empty", False)]
    L_d = ctrl.loop(s, pl0, digital=True)
    L_a = ctrl.loop(s, pl0, digital=False)
    _, pm_d, _, _ = margins(f, L_d)
    _, pm_a, _, _ = margins(f, L_a)
    w(f"   Selected f_c = {FC} Hz: Kp {ctrl.Kp:.0f} N/m (force at the coil per m at the vane), lead zero "
      f"{ctrl.wz / 2 / math.pi:.1f} Hz / pole {ctrl.wp / 2 / math.pi:.0f} Hz, integral corner {ctrl.wi / 2 / math.pi:.1f} Hz, "
      f"low-pass {ctrl.wf / 2 / math.pi:.0f} Hz (zeta {ctrl.zf}).")
    w(f"   These are derived from measured plant data (J, K, Kf, sensor slope) at commissioning, never hard-coded.")
    w(f"   Same law built analog (no sampling delay): PM {pm_a:.1f} deg vs digital {pm_d:.1f} "
      f"deg -> the digital penalty is {pm_a - pm_d:.1f} deg at 10 kHz.")
    for fs_alt in (2000.0, 5000.0):
        ph = math.degrees(2 * math.pi * FC * 1.5 / fs_alt)
        w(f"      (at {fs_alt / 1e3:.0f} kHz the delay would cost {ph:.1f} deg)")
    w(f"   Requirement on the mechanics: non-collocated modes (coil bracket, vane mount) >= 500 Hz with Q <= 50 keep")
    w(f"   loop gain at the mode below -20 dB including the resonant peak; a firmware notch covers surprises.")
    w("")

    # ------------------------------------------------------------ 4. time-domain simulations
    w("4. Time-domain simulations (non-linear: 12-bit DAC with 2nd-order sigma-delta at 50 kHz by DMA, limits")
    w("   -20..+120 % FS, stops, sensor tanh with coarse-channel takeover beyond 75 um)")
    kf = cfg["Kf"]
    trim_F = TRIM * W_MAX * G0 / R_LEV
    gF = G0 / R_LEV                 # N at the coil per kg on the pan

    def Fd_const(m):
        return lambda t: trim_F + m * gF

    def Fd_step(m0, m1, t1):
        return lambda t: trim_F + (m0 if t < t1 else m1) * gF

    sim_e = Sim(plants[("empty", False)], ctrl, kf)
    sim_f = Sim(plants[("full", False)], ctrl, kf)
    results = {}

    def report(name, lg, final_F, t_step, tol_mg=0.5):
        xv = lg["xv"]
        ts_, ys_ = readout(lg["t"], lg["F"], T=0.02, k=1)   # ADS126x sinc4 at 1200 SPS + 20 ms average
        st = settle_time(ts_, to_mg(ys_), to_mg(final_F), tol_mg, t_step)
        w(f"   {name:<46} peak vane {np.max(np.abs(xv)) * 1e6:8.2f} um, settles (ADS + 20 ms) to +/-{tol_mg} mg in "
          f"{st * 1e3:7.1f} ms, stop contact {'YES' if lg['stop_hit'] else 'no'}")
        results[name] = (lg, st)
        return st

    lg = sim_e.run(3.0, Fd_step(0.0, 0.010, 0.2), F0=trim_F)
    report("10 g placed (dosing scale)", lg, trim_F + 0.010 * gF, 0.2)
    lg_pan = sim_e.run(2.5, Fd_step(-M_TARE, 0.0, 0.2), F0=trim_F - M_TARE * gF)
    report("pan placed (+90 g from pan-absent)", lg_pan, trim_F, 0.2)
    lg = sim_e.run(2.5, Fd_step(0.0, -M_TARE, 0.2), F0=trim_F)
    report("pan removed (-90 g; holds null at -15 % FS)", lg, trim_F - M_TARE * gF, 0.2)
    lg = sim_f.run(2.5, Fd_step(0.0, W_MAX, 0.2), F0=trim_F)
    report("520 g dropped on (instant step)", lg, trim_F + W_MAX * gF, 0.2)

    def Fd_ramp(m1, t1, tr):
        return lambda t: trim_F + m1 * gF * min(1.0, max(0.0, (t - t1) / tr))
    lg = sim_f.run(2.5, Fd_ramp(W_MAX, 0.2, 0.2), F0=trim_F)
    report("520 g set down by hand (0.2 s ramp)", lg, trim_F + W_MAX * gF, 0.4)
    # kernel impact
    v_imp = math.sqrt(2 * G0 * H_DROP)
    p_k = M_KERNEL * v_imp
    p_coil = p_k * A_LOAD / B_COIL
    lg_k = sim_e.run(2.0, Fd_step(0.010, 0.010 + M_KERNEL, 0.2), F0=trim_F + 0.010 * gF, impulses=[(0.2, p_coil)])
    st_k = report(f"one {M_KERNEL * 1e6:.1f} mg kernel from {H_DROP * 1e3:.0f} mm", lg_k,
                  trim_F + (0.010 + M_KERNEL) * gF, 0.2, tol_mg=0.1)
    lg10q = results["10 g placed (dosing scale)"][0]
    ts_, y_ = readout(lg10q["t"], lg10q["F"])
    seg = (ts_ > 1.0)
    w(f"      DAC sigma-delta residue in the 1200 SPS data (steady, 10 g): {np.std(to_mg(y_[seg])):.3f} mg rms "
      f"before the readout filter")
    dt_l = lg_k["t"][1] - lg_k["t"][0]
    imp_meas = np.sum(lg_k["F"][lg_k["t"] >= 0.2] - (trim_F + (0.010 + M_KERNEL) * gF)) * dt_l
    w(f"      impact impulse {p_k * 1e6:.2f} uN s at the pan = {p_coil * 1e6:.3f} uN s at the coil; integral of the "
      f"measured force excess {imp_meas * 1e6:.3f} uN s (momentum check)")
    w(f"      a steady falling stream reads high by its impact force = weight of the powder in flight "
      f"(fall time {math.sqrt(2 * H_DROP / G0) * 1e3:.0f} ms): coarse stage {FLOW_COARSE * math.sqrt(2 * H_DROP / G0) * 1e6:.0f} mg"
      f" -> the reading during steady flow equals the mass already released.")
    # acquisition
    def sp_ramp(rate):
        st = {"sp": None}

        def fn(t, y):
            if st["sp"] is None:
                st["sp"] = y
            st["sp"] = st["sp"] + math.copysign(min(rate * T_CTRL, abs(st["sp"])), -st["sp"]) if st["sp"] else 0.0
            return st["sp"]
        return fn

    for label, sim_x, m in (("empty pan", sim_e, 0.0), ("520 g left on the pan", sim_f, W_MAX)):
        lga = sim_x.run(2.5, Fd_const(m), x0=-X_STOP - 1e-6, F0=0.0, mode="acquire", sp_fn=sp_ramp(2e-3))
        xv = lga["xv"]
        inside = np.where(np.abs(xv) < 1e-6)[0]
        t_acq = lga["t"][inside[0]] if len(inside) else float("inf")
        after = lga["t"] > 0.05
        overs = np.max(xv[after]) if np.any(after) else 0.0
        far_stop = np.any(xv[after] > X_STOP * S_VANE / B_COIL * 0.99)
        w(f"   acquisition from the stop, {label:<22} null (|x| < 1 um) at {t_acq * 1e3:6.0f} ms, overshoot "
          f"{overs * 1e6:6.1f} um, opposite stop {'HIT' if far_stop else 'not reached'}")
    # crash
    crash_v = {}
    for lab, tau in (("hard cut (50 us)", None), ("hardware soft-off, RC 0.5 s", 0.5)):
        lgc = sim_f.run(2.5, Fd_step(W_MAX, W_MAX + 0.010, 0.25), F0=trim_F + W_MAX * gF, crash=(0.2, 0.3),
                        soft_off_tau=tau)
        v_imp_pan = 0.0
        xv = lgc["xv"]
        tt = lgc["t"]
        k_hit = np.where(lgc["stop"])[0]
        if len(k_hit):
            k0 = k_hit[0]
            v_imp_pan = abs((xv[k0] - xv[k0 - 5]) / (tt[k0] - tt[k0 - 5])) * A_LOAD / S_VANE
        crash_v[lab] = v_imp_pan * B_COIL / A_LOAD
        w(f"   crash at full load, {lab:<28}: freeze 0.20 s, +10 g 0.25 s, coil off 0.30 s -> stop at "
          f"{v_imp_pan * 1e3:5.2f} mm/s (pan), peak stop force {to_mg(lgc['peak_stop_force']) / 1e6:5.2f} kg-equivalent "
          f"(model stop 300 Hz, zeta 0.3)")
    F_allow = 1.0 * G0 / R_LEV
    w(f"   -> requirement to mechanics: keep the stop peak <= 1 kg-equivalent ({F_allow:.2f} N at the coil, MET-11):")
    for lab, vc in crash_v.items():
        k_req = (F_allow / vc) ** 2 / pe.M_tot
        w(f"      {lab:<28}: arrival {vc * 1e3:5.1f} mm/s at the coil -> k_stop <= {k_req:8.0f} N/m "
          f"(compression {F_allow / k_req * 1e3:.3f} mm at that force)")
    w("      The coil is not a hazard to people (12-35 mW), so SAFE may ramp it down in hardware instead of cutting it;")
    w("      the motors still cut hard.  Elastomer-faced stops plus the soft-off ramp.")
    w("")

    # ------------------------------------------------------------ 5. noise
    w("5. Noise at the measurand (one-sided densities, pan-mass equivalent)")
    fn = np.logspace(-3, np.log10(4000), 8000)
    sn = 2j * np.pi * fn
    pl = plants[("empty", False)]
    G = ctrl.G(sn)
    PF, Pd = pl.tf(sn)
    Lp = G * PF
    T_n = G / (1 + Lp)                                  # sensor noise (m) -> coil force
    T_d = -G * Pd / (1 + Lp)                            # lever disturbance force -> coil force
    sens = SENS_N * np.sqrt(1 + SENS_CORNER / fn)
    S_sens = np.abs(T_n * sens) * R_LEV / G0 * 1e6      # mg/sqrt(Hz)
    S_ads = np.full_like(fn, ADS_EN * kf / R_S * R_LEV / G0 * 1e6)
    ref_rms = REF_PP / 6.6
    ref_dens = ref_rms / math.sqrt(9.9)

    def S_vib(W):
        return np.abs(T_d) * vib_density(fn) / G0 * (W + TRIM * W_MAX) * 1e6

    def S_ref(W):
        return np.full_like(fn, ref_dens * (W + TRIM * W_MAX) * 1e6)

    w(f"   {'source':<38}{'0.1 Hz':>10}{'1 Hz':>10}{'10 Hz':>10}   mg/sqrt(Hz)")
    for name, arr in (("fine sensor 1 nm/rtHz (via the loop)", S_sens), ("ADS126x 0.10 uV/rtHz on 150 ohm", S_ads),
                      ("reference noise, 10 g on the pan", S_ref(0.010)), ("reference noise, 500 g", S_ref(0.5)),
                      ("floor vibration, 10 g on the pan", S_vib(0.010)), ("floor vibration, 500 g", S_vib(0.5))):
        vals = [np.interp(x, fn, arr) for x in (0.1, 1.0, 10.0)]
        w(f"   {name:<38}" + "".join(f"{v:>10.4f}" for v in vals))
    w("   driver and DAC quantisation noise enter as actuator noise and are suppressed by 1/(1+L) in band (negligible).")
    w("")

    # ------------------------------------------------------------ 6. readout filters / profiles
    w("6. Readout filters (ADS126x at 1200 SPS, then cascaded moving averages = sinc^k of window T)")
    w("   SD from the integrated PSDs; settling from the simulated 10 g step (loop + filter) to +/-0.5 mg.")
    lg10 = results["10 g placed (dosing scale)"][0]
    final10 = trim_F + 0.010 * gF
    w(f"   {'filter':<16}{'latency s':>10}{'SD 10 g':>9}{'SD 500 g':>9}{'SD 500 g x10 vib':>17}{'settle 10 g s':>14}")
    profile_rows = []
    for k_, T_ in ((2, 0.05), (2, 0.1), (3, 0.2), (3, 0.33), (3, 0.5), (4, 0.33), (4, 0.4)):
        H2 = sinc_k(fn, T_, k_) ** 2
        def sd(arrs):
            return math.sqrt(sum(np.trapezoid(a**2 * H2, fn) for a in arrs))
        sd10 = sd([S_sens, S_ads, S_ref(0.010), S_vib(0.010)])
        sd500 = sd([S_sens, S_ads, S_ref(0.5), S_vib(0.5)])
        sd500x = sd([S_sens, S_ads, S_ref(0.5), 10 * S_vib(0.5)])
        ts, yF = readout(lg10["t"], lg10["F"], T=T_, k=k_)
        st = settle_time(ts, to_mg(yF), to_mg(final10), 0.5, 0.2)
        lat = k_ * T_ / 2
        profile_rows.append((k_, T_, lat, sd10, sd500, sd500x, st))
        w(f"   sinc^{k_} {T_ * 1e3:5.0f} ms{lat:>10.3f}{sd10:>9.3f}{sd500:>9.3f}{sd500x:>17.3f}{st:>14.2f}")
    pr = {(r[0], r[1]): r for r in profile_rows}
    d_ = pr[(2, 0.1)]
    s_ = pr[(3, 0.33)]
    w(f"   Dispensing profile: sinc^2 x 100 ms - latency {d_[2]:.2f} s, SD {d_[3]:.2f} mg at 10 g, output 20 Hz. The fine")
    w("   stage needs latency < 0.5 s (TRK: flow x latency < m_k/2 at 2.5 mg/s).")
    w(f"   Settling profile: sinc^3 x 0.33 s - settles {s_[6]:.2f} s after the load lands (MET-05 goal 1.5 s), SD "
      f"{s_[3]:.3f} mg at 10 g and {s_[4]:.2f} mg at 500 g (MET-03 <= 1 mg).")
    w(f"   With 10x the assumed floor vibration the 500 g SD becomes {s_[5]:.1f} mg: full-load repeatability is set by the")
    w("   floor, not the electronics -> measure the garage floor on the rig; longer window or isolation feet at high load.")
    w("   Stable flag: |slope| < 0.5 d/s and SD of the last 1 s < 0.5 d in the settling profile (d = 1 mg).")
    w("")

    # ------------------------------------------------------------ 7. measurand options
    w("7. Measurand options")
    span_v = v_s_fs
    inl_t = ADS_INL_TYP * ADS_FSR / span_v
    inl_m = ADS_INL_MAX * ADS_FSR / span_v
    w(f"   M1 ADS126x across a {R_S:.0f} ohm foil shunt (span 0..{span_v:.2f} V of a {ADS_FSR:.0f} V FSR):")
    w(f"      noise {ADS_EN * 1e6:.2f} uV/rtHz -> {S_ads[0]:.3f} mg/rtHz (negligible after the settling filter)")
    w(f"      INL {ADS_INL_TYP * 1e6:.0f} ppm FSR typ / {ADS_INL_MAX * 1e6:.0f} max = {inl_t * 1e6:.1f} / {inl_m * 1e6:.0f} ppm"
      f" of FS before adjustment vs MET-04 3.85 ppm -> needs a >= 3-point linearity adjustment (CAL-03 masses)")
    w(f"      gain drift {ADS_GAIN_DRIFT[0] * 1e6:.1f} typ / {ADS_GAIN_DRIFT[1] * 1e6:.0f} max ppm/K, LTC6655B {REF_TC_MAX * 1e6:.0f} "
      f"ppm/K max, shunt {SHUNT_TCR * 1e6:.2f} ppm/K typ -> electronics span tempco up to "
      f"{(ADS_GAIN_DRIFT[0] + REF_TC_MAX + SHUNT_TCR) * 1e6:.1f} ppm/K (typ ADC) - compensated against a B4 board TMP117,")
    w(f"      separate from the magnet coefficient; LTC6655 thermal hysteresis (up to {REF_HYST * 1e6:.0f} ppm after a full "
      f"-40..85 C cycle) is caught by auto-adjust (MET-07).")
    # M2: +/-I0 PWM in the coil
    m_eff = pe.M_tot
    for f_pwm in (1000.0, 5000.0, 20000.0):
        F_ripple = 4 / math.pi * F_MAX
        x_r = F_ripple / (m_eff * (2 * math.pi * f_pwm) ** 2)
        Lc = cfg["L"]
        t_tr = Lc * 2 * cfg["I_max"] / 10.0
        w(f"   M2 +/-I0 PWM in the coil at {f_pwm / 1e3:4.0f} kHz: +/-{F_MAX:.2f} N square-wave force -> {x_r * 1e9:7.2f} nm "
          f"rigid-body motion at the coil (audible below ~18 kHz); current reversal through {Lc * 1e3:.0f} mH at 10 V "
          f"takes {t_tr * 1e6:.0f} us = {t_tr * f_pwm * 100:.0f} % of a period")
    t_tr_ok, v_bus = 0.02 / 20e3, 24.0          # reversal <= 2 % of a 20 kHz period from a 24 V bus
    n_ok = t_tr_ok * KF_TURN * v_bus / (2 * L_TURN2 * F_MAX)
    i0_ok = F_MAX / (n_ok * KF_TURN)
    w(f"      -> rejected: an inaudible 20 kHz version (reversal <= 2 % of a period from 24 V) needs ~{n_ok:.0f} turns and")
    w(f"         I0 ~{i0_ok * 1e3:.0f} mA (a power stage and a hot shunt), and the full-scale square-wave force excites the")
    w("         coil bracket. In M1 the coil force is smooth DC.")
    w("   M3 duty of a precision PWM DAC (reference switched into an RC filter, then the V-I driver) as the measurand:")
    lsb_t = 1 / 170e6
    for fp in (20000.0,):
        w(f"      G491 timer at 170 MHz, {fp / 1e3:.0f} kHz PWM: {lsb_t * fp * 1e6:.0f} ppm per period, sigma-delta dithered "
          f"to << 1 ppm in the loop band (no HRTIM needed);")
    for Rf in (10e3, 100e3, 1e6):
        w(f"      switch R_on mismatch 1 ohm into a {Rf / 1e3:5.0f} k filter -> bow nonlinearity {1 / Rf / 4 * 1e6:6.2f} ppm "
          f"(D(1-D) shape)")
    w("      -> linear by construction but sensitive to switch charge injection and R_on drift: kept as the rig")
    w("         fallback if M1 + linearity adjustment misses MET-04.")
    w("   The ADS126x is not used inside the loop (sinc latency, noise at high rates); position uses the MCU ADCs.")
    w("")

    # ------------------------------------------------------------ 8. drive, heater, coil temperature, CPU
    w("8. Coil driver, heater, coil temperature, firmware load")
    di_dt = 0.5 / cfg["L"]
    w(f"   Driver: zero-drift op-amp (OPA2388 class, ISC +/-{OPA_ISC * 1e3:.0f} mA) drives the coil directly; I range "
      f"{cfg['I_min'] * 1e3:+.1f}..{cfg['I_max'] * 1e3:+.1f} mA; coil + shunt return to a ~1.0 V node on a 5 V rail "
      f"(swing {1.0 + v_s_min + cfg['I_min'] * cfg['R']:.2f}..{1.0 + v_s_max + v_c_max:.2f} V).")
    w(f"   With 0.5 V of headroom the current slews {di_dt:.0f} A/s through {cfg['L'] * 1e3:.0f} mH: a full-scale change "
      f"takes {cfg['I_fs'] / di_dt * 1e3:.2f} ms (the loop needs ~10 ms). Hardware current limit + COIL_EN gated by SAFE.")
    w("   -> OI-04 closes: no power op-amp (OPA593) is needed once Km is 3+.")
    w(f"   Heater: C+D hairpin driven bipolar (3-level) at >= 25 kHz from a small H-bridge on the 3.3-5 V rail, power set")
    w(f"   by duty (P = D V^2/R_h); P_heater = P0 - I^2 R_coil every 10 ms.  Residual hairpin imbalance (~1 % of a turn")
    w(f"   per pair, ASSUMED) gives only an AC force at the drive frequency -> no DC error, inaudible.")
    p_err = 0.02 * P0
    w(f"   Power accuracy: 2 % of P0 = {p_err * 1e3:.2f} mW -> ~{p_err * 1.1e3:.2f} mg at 520 g (MC §6: 1.1 mg/mW, Recoma 28).")
    for g_, lab in ((1, "FS (gain 1)"), (32, "empty pan (gain 32)")):
        v_c = (cfg["I_fs"] if g_ == 1 else TRIM * F_FS / kf) * cfg["R"]
        rel = ADS2_NOISE_10SPS[g_] / v_c
        w(f"   Coil temperature by resistance (D-060): R_c = R_s x V_coil/V_shunt (ratiometric, ADC2 at 10 SPS), {lab}: "
          f"V_coil {v_c * 1e3:.1f} mV -> {rel * 1e6:.1f} ppm -> {rel / ALPHA_CU * 1e3:.1f} mK per reading")
    cyc = 400
    w(f"   Firmware: control ISR at 10 kHz (dual ADC -> ratio -> PID/lead/LPF/notch -> sigma-delta -> DAC by DMA,")
    w(f"   hardware-triggered so jitter does not reach the plant) ~{cyc} cycles = {cyc / 170e6 * FS_CTRL * 100:.1f} % of the "
      f"170 MHz core; ADS126x DRDY at 1200 SPS; readout filters, heater and thermal model at 10-100 Hz.")
    w("")

    # ------------------------------------------------------------ 9. architecture comparison
    w("9. Loop architecture comparison (same control law; numbers above)")
    rows = [
        ("Phase margin at 25 Hz", f"{pm_a:.0f} deg", f"{pm_d:.0f} deg", f"{pm_a:.0f} deg (analog PD)"),
        ("Quantisation", "none", "12-bit DAC, sigma-delta 50 kHz: 0.009 mg", "small (integrator only)"),
        ("Retune / notch for a found mode", "change parts", "parameter", "parts + parameter"),
        ("Acquisition, anti-windup, gain scheduling", "analog switches + MCU", "code", "mixed"),
        ("Firmware hang", "loop keeps running", "DAC holds -> watchdog -> coil off", "loop keeps running"),
        ("Host-testable (pio test -e native)", "no (SPICE)", "yes, with the plant model", "partly"),
        ("Parts on B4", "~8 op-amp stages + switches", "TIAs + driver only", "~5 stages + switches"),
    ]
    w(f"   {'':<44}{'analog':<30}{'digital (STM32)':<47}{'hybrid':<24}")
    for r in rows:
        w(f"   {r[0]:<44}{r[1]:<30}{r[2]:<47}{r[3]:<24}")
    w("   -> digital loop in the STM32 (the user's stated preference holds up on the numbers).")
    w("")

    # ------------------------------------------------------------ 10. sanity
    w("10. Sanity checks")
    s0 = 2j * np.pi * np.array([1e-4])
    T0 = (-ctrl.G(s0) * pl.tf(s0)[1] / (1 + ctrl.G(s0) * pl.tf(s0)[0]))[0]
    ok1 = abs(T0 - 1) < 1e-3
    w(f"   - closed-loop DC gain disturbance -> coil force = {abs(T0):.6f} (integral action -> 1). " + ("PASS" if ok1 else "FAIL"))
    lgx = results["10 g placed (dosing scale)"][0]
    Fend = np.mean(lgx["F"][lgx["t"] > lgx["t"][-1] - 0.1])
    ok2 = abs(to_mg(Fend - final10)) < 0.05
    w(f"   - simulated 10 g step ends at {to_mg(Fend - trim_F):.3f} mg above trim (expect 10000). " + ("PASS" if ok2 else "FAIL"))
    ok3 = abs(imp_meas / p_coil - 1) < 0.05
    w(f"   - kernel momentum: measured/applied impulse {imp_meas / p_coil:.3f}. " + ("PASS" if ok3 else "CHECK"))
    hf = 2 * np.pi * 30
    Pm = abs(plants[('empty', False)].tf(np.array([1j * hf]))[0][0])
    Pr = S_VANE / B_COIL / ((pe.M_L + pe.m_c) * hf**2)
    ok4 = abs(Pm / Pr - 1) < 0.01
    w(f"   - plant at 30 Hz {Pm:.4e} m/N vs mass line {Pr:.4e}. " + ("PASS" if ok4 else "CHECK"))
    fw = np.linspace(1e-4, 600, 400001)
    var = np.trapezoid(np.sinc(fw * 0.1) ** 2, fw)
    ok5 = abs(var / (1 / (2 * 0.1)) - 1) < 0.01
    w(f"   - white noise through a 0.1 s moving average: ENBW {var:.3f} Hz vs 1/(2T) = 5.000 Hz. " + ("PASS" if ok5 else "FAIL"))
    w(f"   - runtime {time.time() - t0_run:.0f} s")
    (ROOT / "analysis/servo_measurand.out.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
