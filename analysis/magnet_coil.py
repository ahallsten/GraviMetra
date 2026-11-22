"""Phase 2 (OI-24): magnet/coil first-order study for the EMFC cell (Config E).

Run:  .venv/Scripts/python analysis/magnet_coil.py   (writes analysis/magnet_coil.out.txt; a few minutes)

Method (outside method, not from the user's mechanical textbook):
  2-D axisymmetric magnetostatics, finite volume on a tensor grid, flux function psi = r*A_phi:
  B_r = -(1/r) dpsi/dz, B_z = (1/r) dpsi/dr, flux through a coaxial circle = 2*pi*psi.
  Magnet = linear recoil line B = mu0*mu_rec*H + Br (equivalent surface current on its side face).
  Steel linear (mu_r 1000) in the sweep; the selected design is re-solved with non-linear steel
  (secant permeability on an ASSUMED conservative mild-steel B-H curve).
  Coil force constant from the Lorentz integral Kf = (2*pi*N/A_c) * int [psi(r,z2) - psi(r,z1)] dr.
  Validated in section 1 against closed forms and by energy consistency (Kf = d(lambda)/dz).

Tags: [DS] vendor datasheet (docs/datasheets.md), [MECH] docs/phase2/EMFC_MECHANICS.md,
[ASSUMED] engineering assumption to be measured or confirmed, [STD] published standard value.
"""
from __future__ import annotations

import math
import sys
import time
from pathlib import Path

import numpy as np
import pint
import scipy.sparse as sp
import scipy.sparse.linalg as spla
from scipy.interpolate import RegularGridInterpolator

u = pint.UnitRegistry()
ROOT = Path(__file__).resolve().parents[1]
MU0 = 4e-7 * math.pi
G0 = 9.80665
mm = 1e-3

lines: list[str] = []


def w(s: str = "") -> None:
    lines.append(s)
    print(s, flush=True)


# ---------------------------------------------------------------- inputs
W_MAX = 520 * u.g                      # [REQ] capacity
R_LEVER = 15                           # [MECH D-050] lever ratio, coil arm / load arm
A_LOAD = 10 * u.mm                     # [MECH]
B_COIL = R_LEVER * A_LOAD              # coil arm 150 mm
TRIM = 0.02                            # [MECH D-051] option U: +2 % FS coil force at empty pan
HEADROOM = 1.2                         # [ASSUMED] coil force headroom over FS for dynamics/pan spread
KM_REQ = 1.6 * u.N / u.W**0.5          # [MECH OI-24] Km needed for r = 15 (~45 mW at FS)
J_EMPTY = 13407 * u.g * u.cm**2        # [MECH §10]
K_EMPTY = 59.0 * u.mN * u.m / u.rad    # [MECH §10]
M_COIL_MECH = 15 * u.g                 # [MECH] moving-coil mass assumed in the plant
STOP = 3 * u.mrad                      # [MECH] lever stops -> coil +/-0.45 mm
DELTA_S = 100 * u.nm                   # [MECH] optical-null drift per weighing cycle (ASSUMED there)
C_CW = 40 * u.mm                       # [MECH] counterweight arm (load side)

F_FS = ((W_MAX * (1 + TRIM)) * G0 * u.m / u.s**2 / R_LEVER).to(u.N)
F_MAX = HEADROOM * F_FS

# Magnet grades [DS]; Br in T, HcB in A/m, alpha = reversible temp. coefficient of Br (1/K)
GRADES = {
    "Recoma 28 (Sm2Co17)": dict(Br=1.10, Br_min=1.04, HcB=800e3, alpha=-350e-6,
                                src="Arnold Recoma 28 sheet (Recoma-Combined-160301.pdf): Br 1.10/1.04 T, "
                                    "HcB 800 kA/m, a(Br) -0.035 %/K (20-150 C)"),
    "Recoma 2:17 Stab '0' (TC)": dict(Br=0.83, Br_min=None, HcB=630e3, alpha=0.0,
                                      src="Arnold Recoma Stab sheet 23.01.2020: Sm2Co17, 0.83 T, HcB 630 kA/m, "
                                          "RTC ~0 ppm/K (-60..+100 C); custom grade"),
    "EEC 2:17-TC16 (TC)": dict(Br=0.825, Br_min=0.785, HcB=621e3, alpha=-10e-6,
                               src="EEC TC material sheet: 8.25/7.85 kG, Hc 7.8/7.4 kOe, RTC -0.001 %/C typ "
                                   "(-50..150 C); table extraction misaligned, mapped by (BH)max check - CONFIRM"),
    "Recoma 22 (SmCo5)": dict(Br=0.94, Br_min=0.90, HcB=730e3, alpha=-450e-6,
                              src="Arnold Recoma 22 sheet: 0.94/0.90 T, a(Br) -0.045 %/K"),
}
for gname, gd in GRADES.items():
    gd["mu_rec"] = gd["Br"] / (MU0 * gd["HcB"])
BASE_GRADE = "Recoma 28 (Sm2Co17)"
TC_GRADE = "EEC 2:17-TC16 (TC)"

RHO_CU = 1.724e-8      # ohm*m at 20 C [STD IEC 60028 annealed copper]
ALPHA_CU = 3.93e-3     # 1/K [STD]
FILL = 0.55            # [ASSUMED] copper fill of the winding window, random hand winding (orthocyclic ~0.7)
D_CU, D_AL, D_FE, D_SMCO = 8960, 2700, 7850, 8300   # kg/m^3 ([DS] SmCo 8.3 g/cm^3)
SIG_AL = 2.3e7         # S/m [ASSUMED] 6061-T6 ~40 % IACS bobbin
K_AIR = 0.026          # W/(m K) [STD]
H_CONV = 5.0           # W/(m^2 K) [ASSUMED] still air, small parts
G_MOUNT = 0.02         # W/K [ASSUMED] coil bracket conduction into the lever
R_POT = 8.0            # K/W [ASSUMED] pot (magnet) to ambient through the base
MUR_STEEL = 1000.0     # linear sweep
B_DESIGN = 1.1         # T [ASSUMED] mean flux density for sizing base and wall (1.3 T saturated the
                       # base corner under the magnet edge on the conservative B-H curve)
B_RIM_MAX = 1.5        # T sweep feasibility limit at the pole-piece rim (linear model)
POT_OD_MAX = 60 * mm   # [ASSUMED] space under the lever end
KM_MARGIN = 1.25       # [ASSUMED] margin on Km for fill factor, clearance and B-H uncertainty
CLEAR = 0.35 * mm      # [ASSUMED] radial running clearance per side (cut tol +/-0.13 mm + assembly)
T_BOB = 0.30 * mm      # [ASSUMED] Al bobbin wall
CAVITY = 4.0 * mm      # [ASSUMED] wall inner radius beyond the gap outer radius
PLATES = [3.18, 4.76, 6.35, 9.53, 12.7]   # mm [ASSUMED] steel plate thicknesses, confirm vs SendCutSend

# Conservative mild-steel B-H curve [ASSUMED]: between hot-rolled A36 and 1010; replace with the
# supplier's cert or a measured curve. H in A/m, B in T.
BH_H = np.array([0, 200, 400, 600, 1000, 2000, 4000, 8000, 16000, 40000, 100000, 300000, 1e6], float)
BH_B = np.array([0, 0.40, 0.80, 1.00, 1.20, 1.40, 1.55, 1.65, 1.75, 1.90, 2.05, 2.30, 2.9], float)


# ---------------------------------------------------------------- solver
def build_axis(x0, x1, breaks, hfun):
    pts = sorted({x0, x1, *[b for b in breaks if x0 < b < x1]})
    xs = [x0]
    for a, b in zip(pts[:-1], pts[1:]):
        x = a
        while b - x > 1e-12:
            n = math.ceil((b - x) / hfun(x) - 1e-9)
            x = b if n <= 1 else x + (b - x) / n
            xs.append(x)
    return np.array(xs)


def spacing(fine_lo, fine_hi, core_lo, core_hi, h_f, h_m=0.3 * mm, grow=0.15, h_max=8 * mm):
    def h(x):
        if fine_lo <= x < fine_hi:
            return h_f
        if core_lo <= x < core_hi:
            return h_m
        d = core_lo - x if x < core_lo else x - core_hi
        return min(h_max, h_m + grow * d)
    return h


def materials(r, z, regions):
    """regions: list of (r0, r1, z0, z1, mu_r, Br). Later entries override earlier ones."""
    rc, zc = (r[:-1] + r[1:]) / 2, (z[:-1] + z[1:]) / 2
    nu = np.full((len(rc), len(zc)), 1 / MU0)
    br = np.zeros_like(nu)
    kind = np.zeros(nu.shape, dtype=np.int8)       # 0 air, 1 steel, 2 magnet
    for (r0, r1, z0, z1, mur, b, k) in regions:
        m = ((rc >= r0) & (rc <= r1))[:, None] & ((zc >= z0) & (zc <= z1))[None, :]
        nu[m] = 1 / (MU0 * mur)
        br[m] = b
        kind[m] = k
    return nu, br, kind


def solve(r, z, nu, br, jint=None):
    Nr, Nz = len(r) - 1, len(z) - 1
    dr, dz = np.diff(r), np.diff(z)
    I, J = np.meshgrid(np.arange(1, Nr), np.arange(1, Nz), indexing="ij")
    hS, hN = dz[J - 1] / 2, dz[J] / 2
    re, rw = (r[I] + r[I + 1]) / 2, (r[I - 1] + r[I]) / 2
    aE = (nu[I, J - 1] * hS + nu[I, J] * hN) / (re * dr[I])
    aW = (nu[I - 1, J - 1] * hS + nu[I - 1, J] * hN) / (rw * dr[I - 1])
    lw = np.log(r[I] / (r[I] - dr[I - 1] / 2))
    le = np.log((r[I] + dr[I] / 2) / r[I])
    aN = (nu[I - 1, J] * lw + nu[I, J] * le) / dz[J]
    aS = (nu[I - 1, J - 1] * lw + nu[I, J - 1] * le) / dz[J - 1]
    sE = nu[I, J - 1] * br[I, J - 1] * hS + nu[I, J] * br[I, J] * hN
    sW = nu[I - 1, J - 1] * br[I - 1, J - 1] * hS + nu[I - 1, J] * br[I - 1, J] * hN
    rhs = -(sE - sW)
    if jint is not None:
        rhs = rhs + jint
    ni, nj = Nr - 1, Nz - 1
    idx = (I - 1) * nj + (J - 1)
    rows, cols, vals = [idx.ravel()], [idx.ravel()], [(aE + aW + aN + aS).ravel()]
    for m, ci, cj, a in ((I + 1 <= Nr - 1, I + 1, J, aE), (I - 1 >= 1, I - 1, J, aW),
                         (J + 1 <= Nz - 1, I, J + 1, aN), (J - 1 >= 1, I, J - 1, aS)):
        rows.append(idx[m])
        cols.append((ci[m] - 1) * nj + (cj[m] - 1))
        vals.append(-a[m])
    A = sp.csr_matrix((np.concatenate(vals), (np.concatenate(rows), np.concatenate(cols))), shape=(ni * nj,) * 2)
    psi = np.zeros((Nr + 1, Nz + 1))
    psi[1:Nr, 1:Nz] = spla.spsolve(A.tocsc(), rhs.ravel()).reshape(ni, nj)
    return psi


def cv_overlap(r, z, rect):
    """Integral over each interior node's control volume of the indicator of rect (dr dz measure)."""
    r1, r2, z1, z2 = rect
    dr, dz = np.diff(r), np.diff(z)
    rl = np.r_[r[1:-1] - dr[:-1] / 2]
    rh = np.r_[r[1:-1] + dr[1:] / 2]
    zl = np.r_[z[1:-1] - dz[:-1] / 2]
    zh = np.r_[z[1:-1] + dz[1:] / 2]
    orr = np.clip(np.minimum(rh, r2) - np.maximum(rl, r1), 0, None)
    ozz = np.clip(np.minimum(zh, z2) - np.maximum(zl, z1), 0, None)
    return orr[:, None] * ozz[None, :]


def cell_bmag(psi, r, z):
    dr, dz = np.diff(r), np.diff(z)
    rc = (r[:-1] + r[1:]) / 2
    dpz = ((psi[:-1, 1:] - psi[:-1, :-1]) + (psi[1:, 1:] - psi[1:, :-1])) / (2 * dz[None, :])
    dpr = ((psi[1:, :-1] - psi[:-1, :-1]) + (psi[1:, 1:] - psi[:-1, 1:])) / (2 * dr[:, None])
    return np.hypot(dpz, dpr) / rc[:, None]


class Field:
    def __init__(self, r, z, psi):
        self.r, self.z, self.psi = r, z, psi
        self.f = RegularGridInterpolator((r, z), psi)

    def at(self, rr, zz):
        rr, zz = np.broadcast_arrays(np.atleast_1d(rr), np.atleast_1d(zz))
        return self.f(np.c_[rr.ravel(), zz.ravel()]).reshape(rr.shape)

    def kf_turn(self, r1, r2, z1, z2, n=160):
        """Force per ampere-turn (N/A) of a uniformly wound rectangular section (Lorentz integral)."""
        rs = np.linspace(r1, r2, n)
        d = self.at(rs, z2) - self.at(rs, z1)
        return 2 * math.pi * np.trapezoid(d, rs) / ((r2 - r1) * (z2 - z1))

    def link_turn(self, r1, r2, z1, z2, n=48):
        """Mean flux per turn (Wb) over the section."""
        rs, zs = np.linspace(r1, r2, n), np.linspace(z1, z2, n)
        R, Z = np.meshgrid(rs, zs, indexing="ij")
        return 2 * math.pi * np.trapezoid(np.trapezoid(self.at(R, Z), zs, axis=1), rs) / ((r2 - r1) * (z2 - z1))

    def Br_line(self, rr, zs):
        p = self.at(np.full_like(zs, rr), zs)
        return -np.gradient(p, zs) / rr


# ---------------------------------------------------------------- pot geometry
def next_plate(t_need):
    for t in PLATES:
        if t * mm >= t_need:
            return t * mm
    return PLATES[-1] * mm


class Pot:
    def __init__(self, Dm, hm, tp, tc, Br=1.10, mu_rec=1.09, tb=None, Ro=None):
        self.Dm, self.hm, self.tp, self.tc = Dm, hm, tp, tc
        self.rm = self.rp = Dm / 2
        self.r_ci = self.rp + CLEAR + T_BOB
        self.r_co = self.r_ci + tc
        self.r_go = self.r_co + CLEAR
        self.g = self.r_go - self.rp
        self.r_wi = self.r_go + CAVITY
        phi_est = 0.8 * Br * math.pi * self.rm**2
        self.Ro = Ro or math.ceil(math.sqrt(self.r_wi**2 + phi_est / (math.pi * B_DESIGN)) / mm) * mm
        self.tb = tb or next_plate(phi_est / (B_DESIGN * 2 * math.pi * self.rm))
        self.zp0 = self.tb + hm
        self.zp1 = self.zp0 + tp
        self.zmid = (self.zp0 + self.zp1) / 2
        self.Br, self.mu_rec = Br, mu_rec

    def regions(self, mur_steel=MUR_STEEL, Br=None, mu_rec=None):
        Br = self.Br if Br is None else Br
        mu_rec = self.mu_rec if mu_rec is None else mu_rec
        s = mur_steel
        return [(0, self.Ro, 0, self.tb, s, 0, 1),
                (0, self.rm, self.tb, self.zp0, mu_rec, Br, 2),
                (0, self.rp, self.zp0, self.zp1, s, 0, 1),
                (self.r_wi, self.Ro, self.tb, self.zp0, s, 0, 1),
                (self.r_go, self.Ro, self.zp0, self.zp1, s, 0, 1)]

    def grid(self, dom=200 * mm, h_f=0.08 * mm):
        rb = [self.rm, self.r_ci - T_BOB, self.r_ci, self.r_co, self.r_go, self.r_wi, self.Ro]
        h_r = spacing(self.rp - 1.5 * mm, self.r_go + 1.5 * mm, 0, self.Ro + 1 * mm, h_f)
        r = build_axis(0, dom, rb + [self.rp - 1.5 * mm, self.r_go + 1.5 * mm, self.Ro + 1 * mm], h_r)
        zf0, zf1 = self.zp0 - 1.6 * self.tp - 1 * mm, self.zp1 + 1.6 * self.tp + 1 * mm
        zb = [0, self.tb, self.zp0, self.zp1, zf0, zf1]
        h_z = spacing(zf0, zf1, -1 * mm, self.zp1 + 1 * mm, 0.1 * mm)
        z = build_axis(-dom, dom, zb, h_z)
        return r, z

    def mass_steel(self):
        v = (math.pi * self.Ro**2 * self.tb + math.pi * self.rp**2 * self.tp
             + math.pi * (self.Ro**2 - self.r_wi**2) * self.hm + math.pi * (self.Ro**2 - self.r_go**2) * self.tp)
        return v * D_FE

    def mass_magnet(self):
        return math.pi * self.rm**2 * self.hm * D_SMCO


def coil_rect(pot, hc, zc):
    return (pot.r_ci, pot.r_co, zc - hc / 2, zc + hc / 2)


def best_position(F, pot, hc):
    zs = np.linspace(pot.zmid - pot.tp, pot.zmid + pot.tp, 81)
    zs = zs[zs - hc / 2 >= pot.tb + 0.5 * mm]
    k = np.array([abs(F.kf_turn(pot.r_ci, pot.r_co, zc - hc / 2, zc + hc / 2)) for zc in zs])
    i = int(np.argmax(k))
    if 0 < i < len(zs) - 1:   # parabolic refinement
        a, b, c = k[i - 1], k[i], k[i + 1]
        den = a - 2 * b + c
        dzz = 0.5 * (a - c) / den * (zs[1] - zs[0]) if den != 0 else 0.0
        zc = zs[i] + dzz
    else:
        zc = zs[i]
    return zc, abs(F.kf_turn(pot.r_ci, pot.r_co, zc - hc / 2, zc + hc / 2))


def km_of(kf1, pot, hc, fill=FILL):
    a_c = pot.tc * hc
    rbar = (pot.r_ci + pot.r_co) / 2
    return kf1 * math.sqrt(fill * a_c / (RHO_CU * 2 * math.pi * rbar))


def coil_mass(pot, hc, fill=FILL):
    rbar = (pot.r_ci + pot.r_co) / 2
    m_cu = D_CU * fill * pot.tc * hc * 2 * math.pi * rbar * 1.10    # +10 % enamel/bond
    m_bob = D_AL * T_BOB * (hc + 1 * mm) * 2 * math.pi * (pot.r_ci - T_BOB / 2)
    return m_cu + m_bob


def flux_report(F, pot):
    zw = pot.tb + pot.hm / 2
    phi_m = 2 * math.pi * abs(F.at(pot.rm * 0.999, zw)[0])
    rg = (pot.rp + pot.r_go) / 2
    phi_g = 2 * math.pi * abs(F.at(rg, pot.zp0)[0] - F.at(rg, pot.zp1)[0])
    b_rim = 2 * math.pi * abs(F.at(pot.rp, pot.zp0)[0] - F.at(pot.rp, pot.zp1)[0]) / (2 * math.pi * pot.rp * pot.tp)
    b_base = abs(F.at(pot.rm, 0)[0] - F.at(pot.rm, pot.tb)[0]) / (pot.rm * pot.tb)
    b_wall = 2 * abs(F.at(pot.Ro, zw)[0] - F.at(pot.r_wi, zw)[0]) / (pot.Ro**2 - pot.r_wi**2)
    return dict(phi_m=phi_m, phi_g=phi_g, sigma=phi_m / phi_g, b_gap=phi_g / (2 * math.pi * rg * pot.tp),
                b_rim=b_rim, b_base=b_base, b_wall=b_wall)


def solve_pot(pot, nonlinear=False, Br=None, mu_rec=None, dom=200 * mm, iters=60):
    r, z = pot.grid(dom=dom)
    nu, br, kind = materials(r, z, pot.regions(Br=Br, mu_rec=mu_rec))
    psi = solve(r, z, nu, br)
    hist = []
    if nonlinear:   # damped secant iteration on |B| (plain secant oscillates in saturated corners)
        steel = kind == 1
        b_eff = np.maximum(cell_bmag(psi, r, z)[steel], 1e-3)
        for it in range(iters):
            b = np.maximum(cell_bmag(psi, r, z)[steel], 1e-3)
            b_eff = b_eff + 0.3 * (b - b_eff)
            nu[steel] = np.maximum(np.interp(b_eff, BH_B, BH_H) / b_eff, 1 / (MU0 * 5000))
            psi_new = solve(r, z, nu, br)
            ch = np.max(np.abs(psi_new - psi)) / np.max(np.abs(psi_new))
            psi = psi_new
            hist.append(ch)
            if ch < 2e-5:
                break
    return Field(r, z, psi), (r, z, nu, br, kind), hist


# ================================================================ main
def main():
    t_start = time.time()
    w("EMFC magnet/coil first-order study (OI-24).  Inputs tagged [DS]/[MECH]/[ASSUMED]/[STD] in the script.")
    w("")
    w("0. Requirement")
    w(f"   FS coil force F_FS = (520 g x 1.02 trim) x g / r{R_LEVER} = {F_FS:.4f~P};  with {HEADROOM:.1f}x headroom "
      f"F_max = {F_MAX:.4f~P}")
    w(f"   Km target (D-050, OI-24): >= {KM_REQ:~P}  ->  P_FS = F_FS^2/Km^2 = {((F_FS / KM_REQ)**2).to(u.mW):.1f~P}")
    w("")

    # ------------------------------------------------------------ 1. validation
    w("1. Solver validation")
    # 1a bare magnet, mu_rec = 1, on-axis field vs closed form
    Rm, Lm, Brv = 12.7 * mm, 12.7 * mm, 1.0
    hb = spacing(0, 20 * mm, 0, 20 * mm, 0.1 * mm)
    r = build_axis(0, 300 * mm, [Rm], hb)
    z = build_axis(-300 * mm, 300 * mm, [0, Lm, 30 * mm, -20 * mm], spacing(-20 * mm, 40 * mm, -20 * mm, 40 * mm, 0.1 * mm))
    nu, br, _ = materials(r, z, [(0, Rm, 0, Lm, 1.0, Brv, 2)])
    F = Field(r, z, solve(r, z, nu, br))
    errs = []
    for dz_ in (1 * mm, 5 * mm, 15 * mm):
        zz = Lm + dz_
        bz_fd = 2 * F.at(r[1], zz)[0] / r[1]**2
        bz_an = Brv / 2 * ((zz) / math.hypot(zz, Rm) - (zz - Lm) / math.hypot(zz - Lm, Rm))
        errs.append(abs(bz_fd / bz_an - 1))
        w(f"   1a bare magnet Ø25.4x12.7, Br 1 T, on axis {dz_ / mm:.0f} mm above: FV {bz_fd:.4f} T vs "
          f"closed form {bz_an:.4f} T ({(bz_fd / bz_an - 1) * 100:+.2f} %)")
    assert max(errs) < 0.01, "1a failed"
    w("      PASS (< 1 %)")
    # 1b thick solenoid centre field
    r1c, r2c, hc_ = 13.5 * mm, 15.5 * mm, 6.0 * mm
    r = build_axis(0, 300 * mm, [r1c, r2c], spacing(0, 25 * mm, 0, 25 * mm, 0.1 * mm))
    z = build_axis(-300 * mm, 300 * mm, [-hc_ / 2, hc_ / 2], spacing(-15 * mm, 15 * mm, -15 * mm, 15 * mm, 0.1 * mm))
    nu, br, _ = materials(r, z, [])
    jd = 1.0 / ((r2c - r1c) * hc_)                       # 1 ampere-turn
    psi = solve(r, z, nu, br, jint=jd * cv_overlap(r, z, (r1c, r2c, -hc_ / 2, hc_ / 2)))
    F = Field(r, z, psi)
    bz_fd = 2 * F.at(r[1], 0.0)[0] / r[1]**2
    h2 = hc_ / 2
    bz_an = MU0 * jd * h2 * math.log((r2c + math.hypot(r2c, h2)) / (r1c + math.hypot(r1c, h2)))
    w(f"   1b air coil 1 A-turn, centre field: FV {abs(bz_fd) * 1e6:.3f} uT vs closed form {bz_an * 1e6:.3f} uT "
      f"({(abs(bz_fd) / bz_an - 1) * 100:+.2f} %)")
    assert abs(abs(bz_fd) / bz_an - 1) < 0.01, "1b failed"
    l1 = abs(F.link_turn(r1c, r2c, -h2, h2))
    a_in, b_in, c_in = (r1c + r2c) / 2 / 0.0254, hc_ / 0.0254, (r2c - r1c) / 0.0254
    l_wh = 0.8 * a_in**2 / (6 * a_in + 9 * b_in + 10 * c_in) * 1e-6
    w(f"      self-inductance per turn^2: FV {l1 * 1e9:.2f} nH vs Wheeler multilayer {l_wh * 1e9:.2f} nH "
      f"({(l1 / l_wh - 1) * 100:+.1f} %; Wheeler is a ~1-3 % approximation)")
    assert abs(l1 / l_wh - 1) < 0.05
    w("      PASS")
    # 1c energy consistency in a pot
    pot = Pot(25.4 * mm, 12.7 * mm, 6.35 * mm, 2.0 * mm)
    F, _, _ = solve_pot(pot)
    hc, zc = 8 * mm, pot.zmid + 0.7 * mm
    kf_l = abs(F.kf_turn(pot.r_ci, pot.r_co, zc - hc / 2, zc + hc / 2))
    d = 0.05 * mm
    kf_e = abs(F.link_turn(pot.r_ci, pot.r_co, zc - hc / 2 + d, zc + hc / 2 + d)
               - F.link_turn(pot.r_ci, pot.r_co, zc - hc / 2 - d, zc + hc / 2 - d)) / (2 * d)
    w(f"   1c pot, Lorentz integral {kf_l:.5f} N/A-turn vs d(lambda)/dz {kf_e:.5f} N/A-turn "
      f"({(kf_l / kf_e - 1) * 100:+.2f} %)")
    assert abs(kf_l / kf_e - 1) < 0.01
    w("      PASS")
    w("")

    # ------------------------------------------------------------ 2. grades
    w("2. Magnet grades [DS]")
    w(f"   {'grade':<28}{'Br T':>6}{'Br min':>8}{'mu_rec':>8}{'a(Br) ppm/K':>13}  source")
    for gname, gd in GRADES.items():
        bm = f"{gd['Br_min']:.3f}" if gd["Br_min"] else "  -  "
        w(f"   {gname:<28}{gd['Br']:>6.3f}{bm:>8}{gd['mu_rec']:>8.3f}{gd['alpha'] * 1e6:>13.0f}  {gd['src']}")
    w("   NdFeB is ~3x worse in a(Br) (about -0.11 %/K) and is excluded (SmCo CONFIRMED in CLAUDE.md).")
    w("   Arnold's Stab sheet: 'SmCo [is] the material of choice in measuring systems such as electronic")
    w("   balances'; TC grades are made to order ('according to specific customer requirements').")
    w("")

    # ------------------------------------------------------------ 3. sweep
    g28 = GRADES[BASE_GRADE]
    gtc = GRADES[TC_GRADE]
    w("3. Pot sweep (linear steel mu_r 1000, Recoma 28 nominal Br; coil height and axial position optimised for Km)")
    w(f"   fixed: radial clearance {CLEAR / mm:.2f} mm/side, Al bobbin {T_BOB / mm:.2f} mm, fill {FILL}, cavity "
      f"{CAVITY / mm:.0f} mm, base/wall sized for {B_DESIGN} T mean; plates {PLATES} mm [ASSUMED]")
    w(f"   Km_TC = Km x Br_min(TC16)/Br_nom(R28) = x{gtc['Br_min'] / g28['Br']:.3f} (linear scaling, conservative)")
    hdr = (f"   {'magnet':<12}{'tp':>5}{'tc':>5}{'gap':>6}{'hc':>6}{'B coil':>8}{'Km':>7}{'Km TC':>7}{'coil g':>8}"
           f"{'pot OD':>8}{'pot g':>7}{'B rim':>7}  ok")
    w(hdr)
    magnets = [(19.05, 9.53), (25.4, 9.53), (25.4, 12.7), (30.0, 10.0), (31.75, 12.7)]
    results = []
    for (dm, hm) in magnets:
        best = None
        for tp in (3.18, 4.76, 6.35):
            for tc in (1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0):
                pot = Pot(dm * mm, hm * mm, tp * mm, tc * mm, Br=g28["Br"], mu_rec=g28["mu_rec"])
                F, _, _ = solve_pot(pot)
                fr = flux_report(F, pot)
                for kh in (0.75, 1.0, 1.25, 1.5, 2.0, 2.5):
                    hc = kh * tp * mm
                    zc, kf1 = best_position(F, pot, hc)
                    km = km_of(kf1, pot, hc)
                    mc = coil_mass(pot, hc)
                    ok = (fr["b_rim"] <= B_RIM_MAX and mc <= M_COIL_MECH.to(u.kg).m
                          and 2 * pot.Ro <= POT_OD_MAX)
                    rec = dict(dm=dm, hm=hm, tp=tp, tc=tc, hc=hc, zc=zc, kf1=kf1, km=km, mc=mc, pot=pot, fr=fr, ok=ok,
                               bcoil=kf1 / (2 * math.pi * (pot.r_ci + pot.r_co) / 2))
                    results.append(rec)
                    if ok and (best is None or km > best["km"]):
                        best = rec
        for rec in sorted([x for x in results if x["dm"] == dm and x["hm"] == hm and x["ok"]],
                          key=lambda x: -x["km"])[:4]:
            p = rec["pot"]
            w(f"   Ø{dm:.2f}x{hm:.2f}{rec['tp']:>5.2f}{rec['tc']:>5.1f}{p.g / mm:>6.2f}{rec['hc'] / mm:>6.2f}"
              f"{rec['bcoil']:>8.3f}{rec['km']:>7.2f}{rec['km'] * gtc['Br_min'] / g28['Br']:>7.2f}"
              f"{rec['mc'] * 1e3:>8.1f}{2 * p.Ro / mm:>8.0f}{(p.mass_steel() + p.mass_magnet()) * 1e3:>7.0f}"
              f"{rec['fr']['b_rim']:>7.2f}  {'yes' if rec['ok'] else 'no'}")
    n_bad = sum(1 for x in results if not x["ok"])
    w(f"   ({len(results)} coil/pot combinations; {n_bad} rejected for rim B > {B_RIM_MAX} T, coil > "
      f"{M_COIL_MECH:~P} or pot OD > {POT_OD_MAX / mm:.0f} mm; top 4 per magnet shown)")
    w("")

    # selection: the pot is fixed (its mass does not enter the plant); coil power sets every thermal error,
    # so take the highest Km within the coil-mass and OD limits, and require the TC drop-in to keep
    # Km >= 1.6 with a 1.25 margin for fill factor and clearance.
    req = KM_REQ.to(u.N / u.W**0.5).m * KM_MARGIN
    cands = [x for x in results if x["ok"] and x["km"] * gtc["Br_min"] / g28["Br"] >= req]
    rule = (f"highest Km with coil <= {M_COIL_MECH:~P}, pot OD <= {POT_OD_MAX / mm:.0f} mm and "
            f"Km(TC16, min Br) >= 1.6 x {KM_MARGIN}")
    if not cands:
        cands = [x for x in results if x["ok"]]
        rule = "highest Km (no design meets the TC margin)"
    sel = max(cands, key=lambda x: x["km"])
    alt = max([x for x in cands if x["dm"] == 25.4 and x["hm"] == 12.7] or [sel], key=lambda x: x["km"])
    pot = sel["pot"]
    w(f"   Selection rule: {rule}.")
    w(f"   SELECTED: magnet Ø{sel['dm']:.2f} x {sel['hm']:.2f} mm, pole/top plate {sel['tp']:.2f} mm, coil radial "
      f"build {sel['tc']:.1f} mm x height {sel['hc'] / mm:.2f} mm, gap {pot.g / mm:.2f} mm, Km {sel['km']:.2f}")
    w(f"   ALTERNATIVE (1 x 1/2 in, the most common SmCo disc): Ø{alt['dm']:.2f} x {alt['hm']:.2f}, tp {alt['tp']:.2f}, "
      f"tc {alt['tc']:.1f}, hc {alt['hc'] / mm:.2f} mm, Km {alt['km']:.2f}, coil {alt['mc'] * 1e3:.1f} g, "
      f"OD {2 * alt['pot'].Ro / mm:.0f} mm")
    w("")

    # ------------------------------------------------------------ 4. selected design detail
    w("4. Selected design, non-linear steel [ASSUMED B-H], 300 mm domain")
    hc = sel["hc"]
    out = {}
    for gname in (BASE_GRADE, TC_GRADE):
        gd = GRADES[gname]
        for tag, brv in (("nom", gd["Br"]), ("min", gd["Br_min"])):
            Fq, mats, hist = solve_pot(pot, nonlinear=True, Br=brv, mu_rec=gd["mu_rec"], dom=300 * mm)
            zc, kf1 = best_position(Fq, pot, hc)
            out[(gname, tag)] = dict(F=Fq, mats=mats, zc=zc, kf1=kf1, km=km_of(kf1, pot, hc), fr=flux_report(Fq, pot),
                                     iters=len(hist))
            w(f"   {gname:<28} Br {tag} {brv:.3f} T: Kf/turn {kf1:.4f} N/A, Km {out[(gname, tag)]['km']:.2f} N/sqrt(W)"
              f"  (non-linear iterations {len(hist)}; linear-steel Km was "
              f"{sel['km'] * brv / g28['Br']:.2f})")
    base = out[(BASE_GRADE, "nom")]
    Fb, fr, zc0, kf1 = base["F"], base["fr"], base["zc"], base["kf1"]
    w(f"   flux: magnet {fr['phi_m'] * 1e6:.0f} uWb, gap {fr['phi_g'] * 1e6:.0f} uWb -> leakage factor sigma "
      f"{fr['sigma']:.2f}; mean gap B {fr['b_gap']:.3f} T; mean B: pole rim {fr['b_rim']:.2f} T, base "
      f"{fr['b_base']:.2f} T, wall {fr['b_wall']:.2f} T")
    rr, zz, nu, br, kind = base["mats"]
    bst = cell_bmag(Fb.psi, rr, zz)[kind == 1]
    w(f"   steel cells: 95th percentile |B| {np.percentile(bst, 95):.2f} T, 99th {np.percentile(bst, 99):.2f} T "
      f"(local corner peaks above this are expected)")
    w(f"   pot: OD {2 * pot.Ro / mm:.0f} mm, height {pot.zp1 / mm:.1f} mm (base {pot.tb / mm:.2f} + magnet "
      f"{pot.hm / mm:.2f} + top {pot.tp / mm:.2f}), steel {pot.mass_steel() * 1e3:.0f} g + magnet "
      f"{pot.mass_magnet() * 1e3:.0f} g")
    w(f"   coil: r {pot.r_ci / mm:.2f}..{pot.r_co / mm:.2f} mm, height {hc / mm:.2f} mm, centre "
      f"{(zc0 - pot.zmid) / mm:+.2f} mm from the top-plate mid-plane (Kf maximum), mass {coil_mass(pot, hc) * 1e3:.1f} g "
      f"(plant assumed {M_COIL_MECH:~P})")
    # Kf(x)
    offs = np.array([-0.45, -0.2, -0.05, 0.0, 0.05, 0.2, 0.45]) * mm
    kfx = np.array([abs(Fb.kf_turn(pot.r_ci, pot.r_co, zc0 + o - hc / 2, zc0 + o + hc / 2)) for o in offs])
    w("   Kf(x)/Kf(0) at coil offsets (mm): " + ", ".join(f"{o / mm:+.2f}: {k / kfx[3]:.5f}" for o, k in zip(offs, kfx)))
    d = 0.05 * mm
    kp = abs(Fb.kf_turn(pot.r_ci, pot.r_co, zc0 + d - hc / 2, zc0 + d + hc / 2))
    km_ = abs(Fb.kf_turn(pot.r_ci, pot.r_co, zc0 - d - hc / 2, zc0 - d + hc / 2))
    k2 = (kp - 2 * kf1 + km_) / d**2              # d2Kf/dz2 per turn
    set_tol = 0.05 * mm
    ds = DELTA_S.to(u.m).m * (B_COIL / (160 * u.mm)).to("").m   # vane at 160 mm [MECH] -> coil axial drift
    span_ppm = abs(k2) * set_tol * ds / kf1 * 1e6
    w(f"   curvature d2Kf/dz2 / Kf = {k2 / kf1 * 1e-6:.4f} /mm^2.  With the coil set within +/-{set_tol / mm:.2f} mm of the "
      f"maximum, a {DELTA_S:~P} null drift changes Kf by {span_ppm:.4f} ppm (negligible).")
    w(f"   stop-to-stop (+/-0.45 mm) Kf change {(kfx.min() / kfx[3] - 1) * 100:+.2f} % -> servo authority unaffected.")
    out_sel = dict(kf1=kf1, k2=k2)
    w("")

    # ------------------------------------------------------------ 5. wire table
    w("5. Winding options (selected geometry, Recoma 28 nominal, non-linear steel)")
    a_c = pot.tc * hc
    rbar = (pot.r_ci + pot.r_co) / 2
    # inductance per turn^2 with frozen secant permeability, magnet source off
    rr, zz, nu, br, kind = base["mats"]
    rect = coil_rect(pot, hc, zc0)

    def l_turn2(zc_):
        rect_ = coil_rect(pot, hc, zc_)
        jd = 1.0 / a_c
        p = solve(rr, zz, nu, np.zeros_like(br), jint=jd * cv_overlap(rr, zz, rect_))
        return abs(Field(rr, zz, p).link_turn(*rect_))
    L1 = l_turn2(zc0)
    dL1 = (l_turn2(zc0 + 0.1 * mm) - l_turn2(zc0 - 0.1 * mm)) / (0.2 * mm)
    w(f"   inductance per turn^2 {L1 * 1e9:.2f} nH (frozen secant mu, an estimate; small-signal L is lower), "
      f"dL/dz per turn^2 {dL1 * 1e6:+.3f} uH/m")
    w(f"   {'AWG':>4}{'d mm':>7}{'N':>6}{'R ohm':>8}{'Kf N/A':>8}{'I_FS mA':>9}{'V_FS V':>8}{'I_max mA':>9}"
      f"{'L mH':>7}{'L/R ms':>8}")
    awg_rows = {}
    for awg in (30, 32, 34, 36, 38):
        dw = 0.127e-3 * 92 ** ((36 - awg) / 39)
        aw = math.pi * dw**2 / 4
        N = FILL * a_c / aw
        R = RHO_CU * N * 2 * math.pi * rbar / aw
        Kf = N * kf1
        I_fs = F_FS.m / Kf
        L = N**2 * L1
        awg_rows[awg] = dict(N=N, R=R, Kf=Kf, I_fs=I_fs, L=L)
        w(f"   {awg:>4}{dw * 1e3:>7.3f}{N:>6.0f}{R:>8.1f}{Kf:>8.2f}{I_fs * 1e3:>9.1f}{I_fs * R:>8.2f}"
          f"{HEADROOM * I_fs * 1e3:>9.1f}{L * 1e3:>7.2f}{L / R * 1e3:>8.3f}")
    km_b = km_of(kf1, pot, hc)
    p_fs = (F_FS.m / km_b) ** 2
    w(f"   P_FS = F_FS^2/Km^2 = {p_fs * 1e3:.1f} mW for every gauge; P at F_max {(F_MAX.m / km_b)**2 * 1e3:.1f} mW. "
      f"Orthocyclic fill 0.70 would raise Km x{math.sqrt(0.70 / FILL):.2f}.")
    # reluctance force nonlinearity with linear (non-constant-current) drive
    pick = awg_rows[34]
    f_rel = 0.5 * pick["I_fs"]**2 * pick["N"]**2 * dL1
    w(f"   reluctance force 1/2 I^2 dL/dz at FS (AWG 34): {f_rel * 1e6:+.2f} uN = {abs(f_rel) / F_FS.m * 1e6:.1f} ppm "
      f"of FS force, quadratic in load with linear drive; constant with +/-I0 PWM (I^2 constant).")
    w("")

    # ------------------------------------------------------------ 6. power, thermal, constant power
    w("6. Power and thermal")
    h_ov = min(hc, pot.tp)
    g_gap = K_AIR * 2 * math.pi * h_ov * ((pot.r_ci - T_BOB) / CLEAR + pot.r_co / CLEAR)
    a_rest = 2 * math.pi * (pot.r_ci + pot.r_co) * max(hc - h_ov, 0) + 2 * math.pi * rbar * pot.tc * 2
    g_conv = H_CONV * a_rest
    g_tot = g_gap + g_conv + G_MOUNT
    for label, p in (("FS (linear drive, full load)", p_fs), ("constant power at F_max", (F_MAX.m / km_b)**2)):
        w(f"   {label}: P {p * 1e3:.1f} mW -> coil rise {p / g_tot * 1e3:.0f} mK over the pot "
          f"(gap {g_gap * 1e3:.1f} + convection {g_conv * 1e3:.1f} + mount {G_MOUNT * 1e3:.0f} mW/K); "
          f"pot/magnet rise ~{p * R_POT * 1e3:.0f} mK at R_pot {R_POT} K/W")
    frac_pot = g_gap / g_tot
    dT_m = p_fs * frac_pot * R_POT
    a28 = abs(GRADES[BASE_GRADE]["alpha"])
    err_lin = (W_MAX * a28 * dT_m).to(u.mg)
    w(f"   Linear drive: magnet temperature follows load. 0 -> 520 g changes coil power by {p_fs * 1e3:.0f} mW; "
      f"~{frac_pot * 100:.0f} % reaches the pot -> magnet +{dT_m * 1e3:.0f} mK -> span error at 520 g "
      f"{err_lin:.1f~P} (Recoma 28) / {(W_MAX * 10e-6 * dT_m).to(u.mg):.2f~P} (TC16), with a thermal time "
      f"constant of minutes (creep-like).")
    w("   -> constant coil power is required at full range (it is optional for dosing loads of a few grams).")
    w("   Constant-power options (implementation chosen in the servo/measurand study):")
    w(f"     a) +/-I0 bipolar PWM in one winding: P = I0^2 R constant = {(F_MAX.m / km_b)**2 * 1e3:.0f} mW; force ~ (2D-1); "
      f"Km unchanged; measurand is duty x I0.")
    for phi in (0.8, 0.7):
        w(f"     b) split winding, {phi * 100:.0f} % force + {100 - phi * 100:.0f} % non-inductive bifilar heater: "
          f"Km x{math.sqrt(phi):.3f} = {km_b * math.sqrt(phi):.2f}; heater tops total power up to "
          f"{(F_MAX.m / (km_b * math.sqrt(phi)))**2 * 1e3:.0f} mW.")
    w("     c) separate heater resistor on the bobbin: Km unchanged, but heat enters at another spot (residual")
    w("        load-dependent gradient and a transient after each load step).")
    w("")

    # ------------------------------------------------------------ 7. span tempco
    w("7. Span temperature coefficient of Kf")
    geo = 14e-6
    w(f"   Kf ~ Br x geometry. Geometric part ~+{geo * 1e6:.0f} ppm/K [ASSUMED: Cu coil length +17 ppm/K, steel gap"
      f" +12 ppm/K partly cancelling]; lever ratio is invariant to uniform temperature (all-Al lever).")
    tau_m = (4 * pot.hm**2 / (math.pi**2 * 10 / (D_SMCO * 350)))
    dTdt = 2.0 / 3600
    for gname in (BASE_GRADE, TC_GRADE, "Recoma 22 (SmCo5)"):
        a = GRADES[gname]["alpha"] + geo
        lag_ppm = abs(a) * tau_m * dTdt * 1e6
        w(f"   {gname:<28} net {a * 1e6:+6.0f} ppm/K | 15 K garage swing {abs(a) * 15 * 520e3:7.0f} mg at 520 g "
          f"uncompensated | comp. to 2 ppm/K needs alpha to {2e-6 / abs(a) * 100 if a else float('inf'):.1f} % | "
          f"sensor lag {lag_ppm:.2f} ppm")
    w(f"   lag model: sensor on the base under the magnet, magnet conduction time constant 4h^2/(pi^2 a) = "
      f"{tau_m:.0f} s [DS k 10 W/mK, c 350 J/kgK], garage ramp 2 K/h [ASSUMED].")
    w("   With Recoma 28 the closure of OI-02 rests on three things together: magnet-temperature compensation")
    w("   (alpha learned from check-mass adjustments at different temperatures), auto internal adjustment at")
    w("   dT >= 0.5 K (D-032), and constant coil power. A TC grade turns the same scheme from a 1 % problem into")
    w("   a 20-50 % problem.")
    w("")

    # ------------------------------------------------------------ 8. damping
    w("8. Eddy-current damping from the Al bobbin")
    zs = np.linspace(zc0 - hc / 2 - 0.5 * mm, zc0 + hc / 2 + 0.5 * mm, 400)
    rb = pot.r_ci - T_BOB / 2
    brl = Fb.Br_line(rb, zs)
    c_lin = 2 * math.pi * rb * SIG_AL * T_BOB * np.trapezoid(brl**2, zs)
    b_m = B_COIL.to(u.m).m
    c_rot = c_lin * b_m**2
    c_crit = 2 * math.sqrt(K_EMPTY.to(u.N * u.m / u.rad).m * J_EMPTY.to(u.kg * u.m**2).m)
    w(f"   solid bobbin: c = 2 pi r sigma t int(B_r^2 dz) = {c_lin:.2f} N s/m at the coil -> {c_rot * 1e3:.1f} mN m s/rad "
      f"at the fulcrum; critical (open loop, empty) {c_crit * 1e3:.1f} -> zeta {c_rot / c_crit:.1f}")
    w("   a single axial slit in the bobbin removes it (zeta ~0). Choice goes to the servo study; both use the same part.")
    w("   With a current-source drive the winding itself adds no damping (open-circuit for back-EMF).")
    w("")

    # ------------------------------------------------------------ 9. moving coil vs moving magnet
    w("9. Moving coil vs moving magnet")
    m_pot = pot.mass_steel() + pot.mass_magnet()
    m_coil = coil_mass(pot, hc)
    for label, m in (("moving coil", m_coil), ("moving magnet (whole pot on the lever)", m_pot)):
        dcw = m * b_m / C_CW.to(u.m).m
        dJ = (m * b_m**2 + dcw * C_CW.to(u.m).m**2) * 1e7
        w(f"   {label:<40} moving {m * 1e3:6.0f} g, extra counterweight {dcw * 1e3:6.0f} g, J {dJ:8.0f} g cm^2 "
          f"({dJ / J_EMPTY.m * 100:4.0f} % of the plant J), fulcrum load +{(m + dcw) * 1e3:.0f} g")
    w("   Moving magnet removes the two coil leads but multiplies J and fulcrum tension (blade stiffness grows with")
    w("   tension, D-050) and puts the stray field on the moving part. Rejected. Moving coil with two flexible")
    w("   leads crossing at the fulcrum axis:")
    for awg_l in (40, 44):
        dl = 0.127e-3 * 92 ** ((36 - awg_l) / 39)
        k_lead = 117e9 * math.pi * dl**4 / 64 / 0.010
        w(f"     AWG {awg_l} lead, 10 mm free loop: ~{k_lead * 1e6:.2f} uN m/rad each "
          f"({2 * k_lead / K_EMPTY.to(u.N * u.m / u.rad).m * 100:.3f} % of K_tot)")
    w("")

    # ------------------------------------------------------------ 10. stray and external fields
    w("10. Stray field and external field")
    for rx in (50, 100, 140):
        bz = Fb.at(rx * mm, pot.zmid)
        bmag = abs(Fb.Br_line(rx * mm, np.array([pot.zmid - 1 * mm, pot.zmid, pot.zmid + 1 * mm]))[1])
        bzv = abs((Fb.at(rx * mm + 0.5 * mm, pot.zmid)[0] - Fb.at(rx * mm - 0.5 * mm, pot.zmid)[0]) / (1 * mm) / (rx * mm))
        w(f"   |B| at {rx} mm from the pot axis, mid-plane: ~{math.hypot(bmag, bzv) * 1e6:.0f} uT "
          f"(earth field ~50 uT)")
    w("   140 mm is roughly where the coupling blade and the check mass sit; forces on 1095 shim blades there are")
    w("   negligible (V chi B dB/dx / mu0 ~ 1e-11 N << 1 mg = 9.8e-6 N). Check mass must still be non-magnetic.")
    n_i_fs = F_FS.m / kf1
    m_dip = n_i_fs * math.pi * rbar**2
    t_ext = m_dip * 20e-6
    w(f"   coil dipole at FS: N*I = {n_i_fs:.1f} A-turn -> m = {m_dip * 1e3:.2f} mA m^2; in a 20 uT horizontal field "
      f"(unshielded bound) torque {t_ext * 1e9:.0f} nN m = {t_ext / (F_FS.m * b_m) * 1e6:.2f} ppm of FS torque; "
      f"turning the balance 180 deg changes span by up to {2 * t_ext / (F_FS.m * b_m) * 1e6:.1f} ppm before pot "
      f"shielding -> treat as a span term; re-adjust after moving the balance.")
    w("")

    # ------------------------------------------------------------ 11. sanity
    w("11. Sanity checks")
    bavg = kf1 / (2 * math.pi * rbar)
    w(f"   - Kf per turn / (2 pi r_mean) = average B in the winding {bavg:.3f} T, below mean gap B "
      f"{fr['b_gap']:.3f} T (overhung coil sees fringe) and below Br. " +
      ("PASS" if bavg < fr["b_gap"] * 1.05 < GRADES[BASE_GRADE]["Br"] else "CHECK"))
    km_up = fr["b_gap"] * math.sqrt(FILL * a_c * 2 * math.pi * rbar / RHO_CU)
    km_lo = km_up * min(1, pot.tp / hc)
    ok_h = km_lo <= km_b <= km_up
    w(f"   - hand Km = B_gap sqrt(f V/rho): all copper at mean gap B {km_up:.2f} (upper bound), only the in-gap "
      f"fraction {km_lo:.2f} (lower bound); solver {km_b:.2f}. " + ("PASS" if ok_h else "CHECK"))
    km_r = km_b**2 / KM_REQ.m**2
    w(f"   - P_FS at the selected Km is {p_fs * 1e3:.0f} mW vs {((F_FS / KM_REQ)**2).to(u.mW).m:.0f} mW at the 1.6 "
      f"target (x{1 / km_r:.2f}). " + ("PASS" if km_b >= KM_REQ.m else "FAIL"))
    assert out[(TC_GRADE, "min")]["km"] > 0
    w(f"   - TC16 min-Br Km {out[(TC_GRADE, 'min')]['km']:.2f} vs Recoma 28 nominal {km_b:.2f}: ratio "
      f"{out[(TC_GRADE, 'min')]['km'] / km_b:.3f} vs Br ratio {gtc['Br_min'] / g28['Br']:.3f} (less saturation helps). INFO")
    w("")

    # ------------------------------------------------------------ 12. alternatives (user questions 2026-11-22)
    w("12. Alternatives to a plain permanent magnet (user questions 2026-11-22)")
    smco_mmf = GRADES[BASE_GRADE]["HcB"] * pot.hm
    w(f"   The Ø{pot.Dm / mm:.2f} x {pot.hm / mm:.2f} mm SmCo disc supplies H_cB x l_m = {smco_mmf:.0f} A-turns of MMF "
      f"at zero power.")
    kf_target = kf1

    def field_window(p, top_gap=2.2 * mm):
        return (p.rm + 0.5 * mm, p.r_wi - 0.5 * mm, p.tb + 0.5 * mm, p.zp0 - top_gap)

    def kf_per_at(p, core_mu, window):
        r_, z_ = p.grid()
        nu_, br_, _ = materials(r_, z_, p.regions(Br=0.0, mu_rec=core_mu))
        a_w = (window[1] - window[0]) * (window[3] - window[2])
        ps = solve(r_, z_, nu_, br_, jint=(1.0 / a_w) * cv_overlap(r_, z_, window))
        Fx = Field(r_, z_, ps)
        _, k = best_position(Fx, p, hc)
        return k, a_w

    w("   12a. Electromagnet instead of the magnet (soft-iron core in its place, field winding in the cavity,")
    w("        linear steel = lower bound on power, same moving coil and Kf):")
    envs = [("same pot envelope", Pot(pot.Dm, pot.hm, pot.tp, pot.tc, tb=pot.tb, Ro=pot.Ro)),
            ("enlarged pot, OD 100 x 40 mm core", Pot(pot.Dm, 40 * mm, pot.tp, pot.tc, tb=pot.tb, Ro=50 * mm))]
    envs[1][1].r_wi = 40 * mm
    em_rows = []
    for label, p in envs:
        win = field_window(p)
        k_at, a_w = kf_per_at(p, MUR_STEEL, win)
        ni = kf_target / k_at
        mlt = math.pi * (win[0] + win[1])
        p_em = ni**2 * RHO_CU * mlt / (FILL * a_w)
        j_dens = ni / (FILL * a_w) / 1e6
        hgt = p.zp1
        a_surf = 2 * math.pi * p.Ro * hgt + 2 * math.pi * p.Ro**2
        dT = p_em / (10.0 * a_surf)
        em_rows.append((label, ni, p_em))
        w(f"        {label:<34} window {a_w * 1e6:5.0f} mm^2: needs {ni:6.0f} A-turns -> {j_dens:5.1f} A/mm^2, "
          f"{p_em:7.1f} W (vs {p_fs * 1e3:.0f} mW coil), surface rise ~{dT:.0f} K at 10 W/m^2K")
    w("        -> 10^3-10^4 x the heat the whole cell budget is built around, inside the cell. Plus: field depends on")
    w("        a second current (1 ppm regulation = the same difficulty as the measurand itself), iron hysteresis")
    w("        (power-up remanence state) and iron permeability vs temperature. Rejected: a bigger accuracy problem.")
    # 12b permanent magnet + trim winding
    p_tr = Pot(pot.Dm, pot.hm, pot.tp, pot.tc, tb=pot.tb, Ro=pot.Ro)
    win = field_window(p_tr)
    k_tr, a_w = kf_per_at(p_tr, GRADES[BASE_GRADE]["mu_rec"], win)
    shift = abs(GRADES[BASE_GRADE]["alpha"] + geo) * 15
    ni_tr = shift * kf_target / k_tr
    mlt = math.pi * (win[0] + win[1])
    p_tr_w = ni_tr**2 * RHO_CU * mlt / (FILL * a_w)
    w(f"   12b. Magnet + trim winding to hold Kf over the 15 K swing ({shift * 100:.2f} %): {ni_tr:.0f} A-turns, up to "
      f"{p_tr_w * 1e3:.0f} mW varying with temperature. It still needs the magnet temperature (or a ppm field sensor,")
    w("        which does not exist at this price), so it only moves the firmware correction into hardware. No gain.")
    # 12c two coils
    w("   12c. Two coils on the lever:")
    w(f"        - opposed pair for constant power: each has half the copper (Km/sqrt2), so constant power is "
          f"{2 * (F_MAX.m / km_b)**2 * 1e3:.0f} mW vs {(F_MAX.m / km_b)**2 * 1e3:.0f} mW for +/-I0 PWM in one winding "
          f"(the 80/20 split heater is in between).")
    w("        - drift: both coils sit in the same field, so any force or ratio still carries B(T). A second coil")
    w("          cannot cancel the magnet's temperature coefficient.")
    # 12d in-situ alpha
    u_cm = 1.7e-6
    for dT_ in (2.0, 5.0):
        u_a = math.sqrt(2) * u_cm / dT_
        w(f"   12d. In-situ alpha from two check-mass adjustments {dT_:.0f} K apart: u(alpha) = sqrt2 x {u_cm * 1e6:.1f} ppm / "
          f"{dT_:.0f} K = {u_a * 1e6:.2f} ppm/K = {u_a / abs(GRADES[BASE_GRADE]['alpha'] + geo) * 100:.2f} % of alpha "
          f"(needed: 0.6 %).")
    w("        -> the coefficient of the finished cell (magnet + coil + steel + electronics) is measurable to better")
    w("        than any vendor value; the hard part is the magnet's temperature at each instant (lag, gradients).")
    # 12e passive shunt
    w("   12e. Passive shunt of temperature-compensator alloy (Carpenter Temperature Compensator 30, 30 % Ni-Fe,")
    w("        permeability falls with temperature, -51..71 C; stock strip/bar). It diverts flux fraction s from the gap")
    w("        and diverts less as it warms. First order: s x k = |alpha_net|, k = relative change of shunt flux per K:")
    for k_ in (0.005, 0.01, 0.02):
        s_ = abs(GRADES[BASE_GRADE]["alpha"] + geo) / k_
        w(f"        k {k_ * 100:.1f} %/K -> shunt carries {s_ * 100:.1f} % of gap flux -> Km loss ~{s_ * 100:.1f} %")
    w("        k is not published (Carpenter's pages give no curves): measure on the rig. Trimmed by shunt cross-section.")
    w("        Residual after trimming is set by the curvature of mu(T) over 15-30 C [ASSUMED +/-10-30 ppm/K until measured].")
    w(f"   - Runtime {time.time() - t_start:.0f} s.")
    (ROOT / "analysis/magnet_coil.out.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    sys.setrecursionlimit(10000)
    main()
