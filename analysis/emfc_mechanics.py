"""Phase 2 EMFC mechanics feasibility study (Config E): force chain, lever ratio, flat-part blade
flexures, sensitivities that drive adjustability, counterweight (OI-15), check mass (OI-07), overload,
and first plant estimates for the servo study.

Run:  .venv/Scripts/python analysis/emfc_mechanics.py   (writes analysis/emfc_mechanics.out.txt)
Doc:  docs/phase2/EMFC_MECHANICS.md

No textbook index is in the repo (reference/textbooks/ absent), so every mechanics method here is an
OUTSIDE METHOD, named where used:
  [BEAM]   Euler-Bernoulli beam: blade pivot K = E*b*t^3/(12*L); bending stress = E*t*theta/(2L);
           guided (S-bending) blade stiffness 12*E*I/L^3.
  [BCOL]   Beam-column under tension P with end moment, free end free to translate:
           K = sqrt(E*I*P) * coth(k*L), k = sqrt(P/(E*I))  (Timoshenko & Gere, elastic stability).
  [EULER]  Fixed-fixed blade buckling P_cr = 4*pi^2*E*I/L^2.
  [VW]     Virtual work on the parallel guide: eccentric-load error dW/W = e*dh/(L_g*h_g) for a
           hinge-spacing mismatch dh; roll path dW/W = 2*e_y*dy/w^2 for a coupling offset dy from the
           guide's roll-stiffness centre (first order, small motion; derived in EMFC_MECHANICS.md section 4).
  [LUMP]   Rigid-lever lumped plant: f_n = sqrt(K/J)/(2*pi).
  [VCM]    Voice-coil motor constant Km = Kf/sqrt(R) = B*sqrt(V_cu/rho_cu) (copper volume in the gap).
Material properties are ASSUMED typical handbook values (replace with the purchased stock's cert).
Geometry is the PROPOSED baseline; Kf/R/Km of the magnet-coil are a band until the magnet/coil study.
"""
from __future__ import annotations

import math
from pathlib import Path

import pint

u = pint.UnitRegistry()
G0 = 9.80665 * u.m / u.s**2
ROOT = Path(__file__).resolve().parents[1]
OUT: list[str] = []
w = OUT.append


def mg_equiv(force):
    """Force at the pan -> equivalent mass in mg."""
    return (force / G0).to(u.mg)


# =============================================================================================
# 0. Requirements and PROPOSED mechanics allocations (the error-budget study replaces these)
# =============================================================================================
W_MAX = 520 * u.g
W_ECC = 175 * u.g                      # MET-09 test load (1/3 Max)
E_X = E_Y = 25 * u.mm                  # ASSUMED eccentric test offset (pan ~100 mm dia; OI-16)
E_DOSE = 5 * u.mm                      # ASSUMED charge-centroid offset in dosing
M_DOSE = 10 * u.g                      # DSP-03 max charge 150 gr = 9.7 g
ALLOC = {
    "zero_per_100nm": 0.3 * u.mg,      # stiffness x optical-null drift, per 100 nm of drift
    "eccentric": 1.0 * u.mg,           # half of MET-09 +/-2 mg, at 175 g
    "span_tc": 0.5e-6 / u.K,           # quarter of MET-06 2 ppm/K
    "linearity": 0.5 * u.mg,           # quarter of MET-04 +/-2 mg
}
DELTA_S = 100 * u.nm                   # ASSUMED optical-null drift over a weighing cycle (measure on the rig)

# =============================================================================================
# 1. Materials (ASSUMED typical; verify against supplier data/certs)
# =============================================================================================
MAT = {
    "Al 6061-T6 (SCS)": dict(E=68.9 * u.GPa, a=23.6e-6 / u.K, k=167 * u.W / u.m / u.K, rho=2700 * u.kg / u.m**3),
    "MIC-6 cast Al (SCS)": dict(E=71.0 * u.GPa, a=23.9e-6 / u.K, k=142 * u.W / u.m / u.K, rho=2800 * u.kg / u.m**3),
    "304 SS (SCS)": dict(E=193 * u.GPa, a=17.3e-6 / u.K, k=16.2 * u.W / u.m / u.K, rho=8000 * u.kg / u.m**3),
    "Ti-6Al-4V (SCS)": dict(E=114 * u.GPa, a=8.6e-6 / u.K, k=6.7 * u.W / u.m / u.K, rho=4430 * u.kg / u.m**3),
}
BLADE_MAT = {
    "1095 spring steel shim (buy)": dict(E=205 * u.GPa, su=1700 * u.MPa, sy=1300 * u.MPa),
    "301 SS full-hard shim (buy)": dict(E=193 * u.GPa, su=1275 * u.MPa, sy=965 * u.MPa),  # ASTM A666 FH minimums
    "BeCu C17200 HT (buy)": dict(E=128 * u.GPa, su=1300 * u.MPa, sy=1100 * u.MPa),
    "1075 blue temper 0.381 mm (SCS)": dict(E=205 * u.GPa, su=1600 * u.MPa, sy=1200 * u.MPa),
}
FATIGUE_FRAC = 0.25                    # ASSUMED allowable alternating stress = 0.25*UTS for >1e7 cycles

w("0. Thermal-distortion figure of merit alpha/k (lower = less distortion per watt of heat flow)")
for n, m in MAT.items():
    w(f"   {n:<22} alpha {m['a'].m * 1e6:5.1f} ppm/K  k {m['k'].m:5.0f} W/mK  alpha/k {(m['a'] / m['k']).to(u.um / u.W).m:5.3f} um/W")
w("   -> Aluminium has the lowest steady-state distortion per watt (high k beats low alpha); Ti/SS are 8-9x worse.")
w("")

# =============================================================================================
# 2. PROPOSED baseline geometry (single-lever Roberval cell, built up from flat plates + shim blades)
# =============================================================================================
A = 10 * u.mm                          # load arm: fulcrum -> coupling
L_G, H_G, W_G = 70 * u.mm, 60 * u.mm, 60 * u.mm   # guide link length, vertical separation, lateral blade spacing
N_GUIDE_BLADES = 8                     # 2 links x 2 hinges x 2 lateral blades
N_FULCRUM = 2
B_BLADE = 10 * u.mm                    # width of each blade
LAM_G, LAM_F, LAM_C = 1.5 * u.mm, 2.0 * u.mm, 2.0 * u.mm   # free lengths: guide, fulcrum, coupling
LEVER_T, LEVER_H = 6.35 * u.mm, 25 * u.mm      # lever plate thickness (y) and depth (z), Al 6061
M_TARE = 90 * u.g                      # ASSUMED pan + receptor + pan support on the load side
M_COIL = 15 * u.g                      # ASSUMED moving coil + former
M_VANE = 2 * u.g
KF_NOM = 5 * u.N / u.A                 # ASSUMED, for current figures only
LEAD_K = 0.10                          # ASSUMED coil-lead stiffness as a fraction of hinge stiffness, routed past the fulcrum
C_CW = 40 * u.mm                       # counterweight arm on the load side
TRIM = 0.02                            # option U leaves +2 % FS coil force at empty pan
RHO_AL = MAT["Al 6061-T6 (SCS)"]["rho"]


def blade_K(Em, t, b, lam, P=0 * u.N):
    """[BEAM]/[BCOL] rotational stiffness of one blade pivot (N*m/rad)."""
    EI = (Em * b * t**3 / 12).to(u.N * u.m**2)
    if P.m <= 0:
        return (EI / lam).to(u.N * u.m)
    x = ((P / EI) ** 0.5 * lam).to("").m
    return ((EI * P) ** 0.5 * (math.cosh(x) / math.sinh(x))).to(u.N * u.m)


def K_total(Em, t, P_f=0 * u.N, P_c=0 * u.N, t_guide=None, b_pivot=None):
    """Lever rotational stiffness about the fulcrum (N*m/rad): fulcrum + coupling + reflected guide + leads.
    t, b_pivot = pivot (fulcrum, coupling) blade thickness and width; t_guide = guide blade thickness.
    Guide blades always use B_BLADE width."""
    tg = t if t_guide is None else t_guide
    bp = B_BLADE if b_pivot is None else b_pivot
    kf = N_FULCRUM * blade_K(Em, t, bp, LAM_F, P_f / N_FULCRUM)
    kc = blade_K(Em, t, bp, LAM_C, P_c)
    kg = N_GUIDE_BLADES * blade_K(Em, tg, B_BLADE, LAM_G) * (A / L_G).to("").m ** 2
    return (kf + kc + kg) * (1 + LEAD_K)


def system(r):
    """Masses, counterweight (option U), inertia and blade tensions for lever ratio r."""
    b = r * A
    bs = b + 10 * u.mm
    L1, L2 = A + 15 * u.mm, bs + 5 * u.mm          # lever plate extends L1 on the load side, L2 on the coil side
    lin = RHO_AL * LEVER_T * LEVER_H
    m_lev = (lin * (L1 + L2)).to(u.g)
    mom_coil = (M_COIL * b + M_VANE * bs + lin * (L2**2 - L1**2) / 2).to(u.g * u.mm)
    m_cw = ((mom_coil - M_TARE * A + TRIM * W_MAX * A) / C_CW).to(u.g)
    J_lev = lin * (L1**3 + L2**3) / 3
    J0 = (J_lev + M_COIL * b**2 + M_VANE * bs**2 + M_TARE * A**2 + abs(m_cw) * C_CW**2).to(u.kg * u.m**2)
    J1 = (J0 + W_MAX * A**2).to(u.kg * u.m**2)
    m_static = (m_lev + M_COIL + M_VANE + abs(m_cw)).to(u.g)

    def tensions(W):
        Fc = ((W + TRIM * W_MAX) * G0 / r).to(u.N)   # coil force magnitude at null
        Pc = ((W + M_TARE) * G0).to(u.N)              # coupling blade tension
        Pf = (Pc + m_static * G0 + Fc).to(u.N)        # total fulcrum tension
        return Pf, Pc, Fc
    return dict(b=b, bs=bs, m_lev=m_lev, m_cw=m_cw, mom_coil=mom_coil, J0=J0, J1=J1, m_static=m_static,
                tensions=tensions)


# =============================================================================================
# 3. Blade options: stiffness vs optical-null drift, stresses, buckling
# =============================================================================================
R_REF = 15
S_REF = system(R_REF)
w(f"1. Blade options (all hinges same t) at r = {R_REF}, a = {A.m:.0f} mm, vane radius {S_REF['bs'].m:.0f} mm, blades {B_BLADE.m:.0f} mm wide")
w(f"   {'blade':<34}{'t mm':>6}{'K_tot mNm/rad':>15}{'zero err / 100nm':>18}{'sig@stop MPa':>14}{'fatigue ok':>11}{'P_cr/P':>8}")
PHI_STOP = 3 * u.mrad                   # lever stop (coil end +/-0.45 mm at b = 150 mm)
P_ECC_LINK = (W_MAX * G0 * E_X / H_G).to(u.N)   # worst guide link axial force from x-eccentric full load
Pf1, Pc1, _ = S_REF["tensions"](W_MAX)
rows = {}
for name, bm in BLADE_MAT.items():
    for tt in ((0.025, 0.05, 0.075, 0.10) if "SCS" not in name else (0.381,)):
        t = tt * u.mm
        K = K_total(bm["E"], t, Pf1, Pc1)
        dF = (K * (DELTA_S / S_REF["bs"]) / A).to(u.N)
        sig = (bm["E"] * t * PHI_STOP / (2 * LAM_F)).to(u.MPa)
        pcr = (4 * math.pi**2 * bm["E"] * B_BLADE * t**3 / 12 / LAM_G**2).to(u.N)
        rows[(name, tt)] = (K, dF)
        w(f"   {name:<34}{tt:>6.3f}{K.to(u.mN * u.m).m:>15.2f}{mg_equiv(dF).m:>15.2f} mg{sig.m:>14.0f}"
          f"{'yes' if sig < FATIGUE_FRAC * bm['su'] else 'NO':>11}{(pcr / (P_ECC_LINK / 2)).to('').m:>8.0f}")
w(f"   allocation: <= {ALLOC['zero_per_100nm'].m} mg per {DELTA_S.m:.0f} nm of optical-null drift (full load, tension-stiffened)")
k381 = rows[("1075 blue temper 0.381 mm (SCS)", 0.381)]
k05 = rows[("1095 spring steel shim (buy)", 0.05)]
w(f"   -> SendCutSend 1075 (0.381 mm, its only spring stock) is {(k381[0] / k05[0]).to('').m:.0f}x stiffer than 0.05 mm shim: "
  f"{mg_equiv(k381[1]).m:.0f} mg per 100 nm. REJECTED for the measuring hinges; thin shim blades must be bought.")
w("")

BL = BLADE_MAT["1095 spring steel shim (buy)"]
T_BL = 0.05 * u.mm                      # fulcrum + coupling blades
T_GUIDE = 0.10 * u.mm                   # guide blades (see 1b)

# 3b. Guide roll stiffness (y-eccentric moment) and the roll-released coupling
w("1b. Guide roll (y-eccentric moment carried by the guide; coupling made roll-compliant)")
K_c_roll = blade_K(BL["E"], T_BL, B_BLADE, LAM_C)   # 90-deg rotated coupling blade bends in roll
for tg in (0.05 * u.mm, 0.10 * u.mm):
    k_S = (12 * BL["E"] * B_BLADE * tg**3 / 12 / LAM_G**3).to(u.N / u.m)   # guided S-bending, one blade [BEAM]
    K_roll = (2 * k_S * (W_G / 2) ** 2).to(u.N * u.m)   # per hinge; near/far in series, two links in parallel -> same
    psi = (W_MAX * G0 * E_Y / K_roll).to(u.mrad)
    frac = (K_c_roll / (K_c_roll + K_roll)).to("").m
    err_lever = (W_ECC * frac * 3e-3 * E_Y / A).to(u.mg)  # roll share through the lever x 3 mrad fulcrum-axis skew
    w(f"   guide t = {tg.m:.2f} mm: K_roll {K_roll.m:6.1f} N*m/rad, roll at 520 g x 25 mm {psi.m:5.2f} mrad; "
      f"lever carries {frac * 100:.4f} % of the roll moment -> {err_lever.m:.3f} mg at 175 g with 3 mrad axis skew")
kg_share = (N_GUIDE_BLADES * blade_K(BL["E"], T_GUIDE, B_BLADE, LAM_G) * (A / L_G).to("").m ** 2
            / K_total(BL["E"], T_BL, t_guide=T_GUIDE)).to("").m
w(f"   -> Guide blades 0.10 mm (roll 0.12 mrad), pivots 0.05 mm; the guide is then {kg_share * 100:.0f} % of K_tot (reflected by (a/L)^2).")
w("      Coupling = two blades in series at 90 deg (one passes lever pitch, one releases roll): roll moment stays in the guide.")
y_star = (W_G / 2 * 0.15).to(u.mm)
w(f"   Shim thickness +/-5 % -> blade stiffness +/-15 % -> roll-stiffness centre offset up to {y_star.m:.1f} mm:")
w("      the y adjuster needs >= +/-5 mm range as well as sub-um resolution.")
w("")

# =============================================================================================
# 4. Lever-ratio trade (moving coil, counterweight option U)
# =============================================================================================
KM_BAND = (0.9 * u.N / u.W**0.5, 1.6 * u.N / u.W**0.5, 2.5 * u.N / u.W**0.5)  # motor constant Km = Kf/sqrt(R)
w("2. Lever-ratio trade (pivots 1095 0.05 mm, guide 0.10 mm; full load 520 g; coil power P = (F/Km)^2)")
w(f"   {'r':>4}{'b mm':>7}{'F_coil N':>10}{'P mW at Km 0.9/1.6/2.5':>25}{'m_cw g':>8}{'zero/100nm mg':>15}{'f_n empty/full Hz':>19}")
trade = {}
for r in (5, 8, 10, 12, 15, 20, 25, 30):
    S = system(r)
    F = (W_MAX * G0 / r).to(u.N)
    P = [((F / km) ** 2).to(u.mW) for km in KM_BAND]
    Pf0, Pc0, _ = S["tensions"](0 * u.g)
    Pf, Pc, _ = S["tensions"](W_MAX)
    K0 = K_total(BL["E"], T_BL, Pf0, Pc0, t_guide=T_GUIDE)
    K1 = K_total(BL["E"], T_BL, Pf, Pc, t_guide=T_GUIDE)
    dF = (K1 * (DELTA_S / S["bs"]) / A).to(u.N)
    f0 = ((K0 / S["J0"]) ** 0.5 / (2 * math.pi)).to(u.Hz)
    f1 = ((K1 / S["J1"]) ** 0.5 / (2 * math.pi)).to(u.Hz)
    trade[r] = dict(S, F=F, P=P, dF=dF, K0=K0, K1=K1, f0=f0, f1=f1)
    w(f"   {r:>4}{S['b'].m:>7.0f}{F.m:>10.3f}{'/'.join(f'{x.m:.0f}' for x in P):>25}{S['m_cw'].m:>8.0f}"
      f"{mg_equiv(dF).m:>15.2f}{f0.m:>9.2f} /{f1.m:>5.2f}")
w("   Selection r = 15 (b = 150 mm), CONDITIONAL on the magnet/coil study reaching Km >= 1.6 N/sqrt(W)")
w("   (P = 45 mW at full load). If Km < 1.2, go to r = 20. r < 10 costs > 100 mW even at Km 1.6; r > 20 makes the")
w("   lever > 200 mm (enclosure, lever modes) for little gain.")
B_GAP, V_CU, RHO_CU = 0.5 * u.T, 0.6 * math.pi * 30 * 4 * 2 * u.mm**3, 1.72e-8 * u.ohm * u.m
Km_est = (B_GAP * (V_CU / RHO_CU) ** 0.5).to(u.N / u.W**0.5)
w(f"   Km plausibility [VCM]: B {B_GAP.m} T, copper {V_CU.to(u.mm**3).m:.0f} mm^3 (dia 30 x 4 x 2 mm coil, 60 % fill) -> "
  f"Km = {Km_est.m:.1f} N/sqrt(W), copper ~{(V_CU * 8.96 * u.g / u.cm**3).to(u.g).m:.0f} g. 1.6 looks reachable (FEMM to confirm).")
w("")
R_SEL = 15
TS = trade[R_SEL]
B_SEL, BS_SEL = TS["b"], TS["bs"]

# 4b. Closing the zero-drift allocation at r = 15 (full load, tension-stiffened)
w(f"2b. Closing the zero-drift allocation at r = {R_SEL} (baseline {mg_equiv(TS['dF']).m:.2f} mg per 100 nm vs {ALLOC['zero_per_100nm'].m} mg)")
w("    Tension stiffening sets a floor K -> sqrt(E*I*P) per blade that blade length cannot remove; t, b and P can.")
Pf_full, Pc_full, _ = TS["tensions"](W_MAX)
lever_mass_cut = 0.5                    # truss-pocketed lever: half the plate mass, so less counterweight too
mom_plate = (TS["mom_coil"] - M_COIL * B_SEL - M_VANE * BS_SEL).to(u.g * u.mm)
dm_light = (TS["m_lev"] * lever_mass_cut + mom_plate * lever_mass_cut / C_CW).to(u.g)
dP_light = (dm_light * G0).to(u.N)
for label, t_p, b_p, t_g, light in (
        ("baseline: pivots 0.05 x 10, guide 0.10", 0.05, 10, 0.10, False),
        ("pivots 0.04 x 10, guide 0.10", 0.04, 10, 0.10, False),
        ("pivots 0.05 x 6, guide 0.075", 0.05, 6, 0.075, False),
        ("pivots 0.04 x 8, guide 0.075, light lever", 0.04, 8, 0.075, True),
):
    Pf_use = Pf_full - (dP_light if light else 0 * u.N)   # lever plate and counterweight both lighter
    K = K_total(BL["E"], t_p * u.mm, Pf_use, Pc_full, t_guide=t_g * u.mm, b_pivot=b_p * u.mm)
    dF = (K * (DELTA_S / BS_SEL) / A).to(u.N)
    k_S = (12 * BL["E"] * B_BLADE * (t_g * u.mm) ** 3 / 12 / LAM_G**3).to(u.N / u.m)   # guide blades stay 10 mm wide
    psi = (W_MAX * G0 * E_Y / (2 * k_S * (W_G / 2) ** 2)).to(u.mrad)
    w(f"    {label:<44} K {K.to(u.mN * u.m).m:5.1f} mN*m/rad -> {mg_equiv(dF).m:4.2f} mg/100 nm, worst roll {psi.m:4.2f} mrad")
w("    -> Thinner/narrower pivots plus a truss-pocketed lever close the allocation; guide 0.075 mm keeps roll ~0.3 mrad.")
w("       Fix the final set in Phase 4 from the measured optical-null drift (100 nm per cycle is a conservative ASSUMED value).")
w("")

# =============================================================================================
# 5. Eccentric (corner) load: sensitivities and the adjustment they force
# =============================================================================================
alloc_ratio = (ALLOC["eccentric"] / W_ECC).to("").m
dh_req = (alloc_ratio * L_G * H_G / E_X).to(u.um)
dy_req = (alloc_ratio * W_G**2 / (2 * E_Y)).to(u.um)
CUT_TOL = 0.127 * u.mm                 # SendCutSend laser +/-0.005 in (sendcutsend.com, read 2026-11-21)
dh_cut = 2 * CUT_TOL                   # two independent features form each hinge spacing
w("3. Eccentric load [VW]")
w(f"   MET-09 allocation {ALLOC['eccentric'].m} mg at {W_ECC.m:.0f} g, e = {E_X.m:.0f} mm -> dW/W <= {alloc_ratio * 1e6:.1f} ppm")
w(f"   x: hinge-spacing mismatch dh <= {dh_req.m:.2f} um   (cut geometry alone: up to {dh_cut.to(u.um).m:.0f} um, "
  f"{(dh_cut / dh_req).to('').m:.0f}x too large)")
w(f"   y: coupling offset from the guide roll-stiffness centre dy <= {dy_req.m:.2f} um")
ecc_unadj = (E_X * dh_cut / (L_G * H_G)).to("").m
w(f"   unadjusted x error at 175 g, e = 25 mm: {(W_ECC * ecc_unadj).to(u.mg).m:.0f} mg")
dose_err_unadj = (M_DOSE * E_DOSE * dh_cut / (L_G * H_G)).to(u.mg)
dh_dose = ((0.3 * u.mg / M_DOSE) * L_G * H_G / E_DOSE).to(u.um)
w(f"   dosing case (10 g charge, centroid within {E_DOSE.m:.0f} mm): unadjusted {dose_err_unadj.m:.1f} mg; "
  f"needs dh <= {dh_dose.m:.0f} um for 0.3 mg (a coarse adjust is enough)")
PITCH, REDUCTION, FEEL = 0.35 * u.mm, 50, 5   # M3x0.35 fine thread (or 1/4-80 adjuster); 50:1 flexure lever; 5 deg feel
res = (PITCH / REDUCTION * FEEL / 360).to(u.um)
w("   -> Two fine corner adjusters (x: fixed-side hinge height; y: coupling lateral position) are mandatory for MET-09.")
w(f"   Adjuster: M3x0.35 fine screw (or purchased 1/4-80 adjuster) through a {REDUCTION}:1 flexure-lever reducer cut from plate"
  f" -> {(PITCH / REDUCTION).to(u.um).m:.0f} um/rev, {res.m:.2f} um per {FEEL} deg: resolution meets x ({dh_req.m:.1f} um)"
  f" and y ({dy_req.m:.1f} um). Lock-in stability is the open risk.")
w("")

# =============================================================================================
# 6. Residual hinge angle at null x tension stiffening -> span, nonlinearity, span tempco
# =============================================================================================
w("4. Residual hinge angle theta0 at null x tension stiffening [BCOL]")


def parasitic_force(theta0, W):
    """Pan-equivalent force from blade moments K(P)*theta0 at fulcrum and coupling for load W on the pan."""
    Pf, Pc, _ = TS["tensions"](W)
    Mf = N_FULCRUM * blade_K(BL["E"], T_BL, B_BLADE, LAM_F, Pf / N_FULCRUM) * theta0
    Mc = blade_K(BL["E"], T_BL, B_BLADE, LAM_C, Pc) * theta0
    return ((Mf + Mc) / A).to(u.N)


for th in (0.2, 0.5, 1.0, 3.0):
    th0 = th * u.mrad
    loads = [x * u.g for x in range(0, 521, 26)]
    F = [parasitic_force(th0, Wl).m for Wl in loads]
    span = (F[-1] - F[0]) / (W_MAX * G0).to(u.N).m
    lin = max(abs(F[i] - (F[0] + (F[-1] - F[0]) * i / (len(F) - 1))) for i in range(len(F)))
    w(f"   theta0 = {th:4.1f} mrad: span term {span * 1e6:8.1f} ppm (calibrated out), "
      f"end-point nonlinearity {mg_equiv(lin * u.N).m:6.2f} mg (alloc {ALLOC['linearity'].m})")
dspan_dth = ((parasitic_force(1 * u.mrad, W_MAX) - parasitic_force(1 * u.mrad, 0 * u.g)) / (W_MAX * G0) / (1 * u.mrad)).to(1 / u.rad)
dth_dT_max = (ALLOC["span_tc"] / dspan_dth).to(u.urad / u.K)
w(f"   span sensitivity {dspan_dth.to(1 / u.mrad).m * 1e6:.0f} ppm per mrad of theta0 -> d(theta0)/dT <= {dth_dT_max.m:.1f} urad/K"
  f" for {ALLOC['span_tc'].m * 1e6:.1f} ppm/K")
dth_grad = (MAT["Al 6061-T6 (SCS)"]["a"] * 0.1 * u.K * (B_SEL / LEVER_H) / 2).to(u.urad)
w(f"   a 0.1 K top-bottom gradient change bends the Al lever ~{dth_grad.m:.0f} urad (mean slope) -> "
  f"{(dspan_dth * dth_grad).to('').m * 1e6:.1f} ppm span shift:")
w("   vertical gradients in the lever must stay stable to a few tens of mK (thermal symmetry, coil heat path, enclosure).")
w("   Set theta0 ~ 0 by clamping the blades with the lever pinned at null in an assembly jig (~0.3 mrad), then trim")
w("   electronically: the servo setpoint (optical null offset) is an adjuster with no mechanical work; theta0 = 0 is where")
w("   the nonlinearity changes sign between two setpoints (Phase 7 characterization).")
w("")

# =============================================================================================
# 7. Gravitational stiffness from vertical offsets of the force lines
# =============================================================================================
Kg_per_mm = ((W_MAX + M_TARE) * G0 * 1 * u.mm).to(u.N * u.m)
dz_tol = (0.2 * TS["K0"] / ((W_MAX + M_TARE) * G0)).to(u.um)
dz_cm_tol = (0.2 * TS["K0"] / (TS["m_static"] * G0)).to(u.um)
w("5. Gravitational stiffness K_g = F * dz (vertical offset of a force line from the fulcrum plane)")
w(f"   full load at the coupling: {Kg_per_mm.to(u.mN * u.m).m:.1f} mN*m/rad per mm of offset vs K_tot {TS['K0'].to(u.mN * u.m).m:.1f} mN*m/rad")
w(f"   keep |K_g| <= 20 % of K_tot -> coupling line within +/-{dz_tol.m:.0f} um of the fulcrum plane;")
w(f"   lever system ({TS['m_static'].m:.0f} g incl. counterweight): centre of mass within +/-{dz_cm_tol.m:.0f} um.")
w("   -> A height adjuster on the counterweight sets the net K_g, i.e. the open-loop stiffness the servo sees;")
w("      measure it on the rig as the coil-current vs null-offset slope.")
w("")

# =============================================================================================
# 8. Lever bending, lever thermal gradient, clamp slip
# =============================================================================================
Em = MAT["Al 6061-T6 (SCS)"]["E"]
I_lev = LEVER_T * LEVER_H**3 / 12
d_lev = (TS["F"] * B_SEL**3 / (3 * Em * I_lev)).to(u.um)
dF_bend = (TS["K1"] * (d_lev / B_SEL) / A).to(u.N)
dK = ((TS["K1"] - TS["K0"]) / TS["K1"]).to("").m
w("6. Lever bending, lever thermal gradient, clamp slip")
w(f"   lever tip deflection at full coil force {d_lev.m:.2f} um -> {mg_equiv(dF_bend).m:.1f} mg, ~linear in load (span, calibrated);")
w(f"   the {dK * 100:.0f} % K(P) change over the range leaves ~{mg_equiv(dF_bend).m * dK / 4:.2f} mg nonlinearity. Vane at the coil, lever deep.")
R_th = (0.1 * u.m / (MAT["Al 6061-T6 (SCS)"]["k"] * LEVER_T * LEVER_H)).to(u.K / u.W)
for label, P_c in (("Km 0.9", TS["P"][0]), ("Km 1.6", TS["P"][1])):
    dT_grad = (P_c * R_th * 0.5).to(u.K)
    dr = (MAT["Al 6061-T6 (SCS)"]["a"] * dT_grad).to("").m
    w(f"   coil heat {label}: {P_c.m:.0f} mW x {R_th.m:.1f} K/W x 0.5 (half flows along the lever, ASSUMED) -> "
      f"{dT_grad.to(u.mK).m:.0f} mK arm-to-arm -> ratio {dr * 1e6:.2f} ppm at full load, ~{(dr * W_MAX / 4).to(u.mg).m:.2f} mg nonlinearity (P ~ W^2)")
w("   -> A constant-power coil drive turns this into a calibrated constant. Ti/SS levers would be 8-9x worse (alpha/k).")
w(f"   clamp slip along x at the fulcrum changes the ratio by slip/a: 1 ppm = {(1e-6 * A).to(u.nm).m:.0f} nm -> clamps must not slip:")
w("   preload >= 100x (shear / mu), bonded + bolted blade clamps, no shear path through dowel clearance.")
w("   Steel blades in Al clamps (documented material mix): 12.6 ppm/K mismatch is taken as blade strain (mN-level), no slip.")
w("")

# =============================================================================================
# 9. Counterweight (OI-15)
# =============================================================================================
w("7. Counterweight (OI-15)")
m_cw_B = (TS["m_cw"] + (0.5 - TRIM) * W_MAX * A / C_CW).to(u.g)
w(f"   coil-side moment {TS['mom_coil'].m:.0f} g*mm (coil, vane, lever plate {TS['m_lev'].m:.0f} g) vs load-side tare "
  f"{(M_TARE * A).to(u.g * u.mm).m:.0f} g*mm")
w(f"   option U (trim tare, +2 % FS margin): {TS['m_cw'].m:.0f} g at {C_CW.m:.0f} mm on the load side")
w(f"   option B (preload W/2, bipolar coil): {m_cw_B.m:.0f} g at {C_CW.m:.0f} mm  (mass penalty small thanks to the lever)")
w("   Tared reading from magnet tempco: m_hat(W) - m_hat(0) = W*(1 - alpha*dT) for ANY preload; preload only adds")
w(f"   zero drift W_pre*alpha*dT: option B at alpha 4e-4/K -> {(0.5 * W_MAX * 4e-4).to(u.mg).m:.0f} mg/K of zero drift vs ~0 for U.")
w(f"   Peak coil power at Km 1.6: U {TS['P'][1].m:.0f} mW, B {TS['P'][1].m / 4:.0f} mW.")
w("   -> Recommend U: dosing sits near zero coil force; preload buys only peak power, which Km and constant-power drive handle.")
w("   Lighter lever (laser-cut truss pockets) cuts both the counterweight and the inertia; refine in Phase 4.")
w("")

# =============================================================================================
# 10. Check mass (OI-07)
# =============================================================================================
w("8. Internal check mass (OI-07): span-adjust uncertainty vs mass value")
SIG_READ, N_AVG = 1.0 * u.mg, 10       # MET-03 repeatability SD; readings averaged per adjust
for m_cm, u_cm in ((100 * u.g, 0.10 * u.mg), (200 * u.g, 0.15 * u.mg), (500 * u.g, 0.30 * u.mg)):
    u_span = (((SIG_READ / N_AVG**0.5) ** 2 + u_cm**2) ** 0.5 / m_cm).to("")
    w(f"   {m_cm.m:4.0f} g (value known to {u_cm.m:.2f} mg by transfer from E2/F1, ASSUMED): span u = {u_span.m * 1e6:4.1f} ppm "
      f"-> {(u_span * W_MAX).to(u.mg).m:.2f} mg at 520 g")
w("   -> 200 g CANDIDATE: span u ~1.7 ppm (0.9 mg at Max), lifter handles 2 N, matches the D-030 E2 200 g reference.")
w("   Non-magnetic austenitic stainless (or OIML-style) slug on a 3-ball kinematic seat on the load receptor (so it also")
w("   checks the lever ratio); a geared stepper (CM_STEP/DIR/HOME) lowers it with a fork that then retracts >= 1 mm clear;")
w("   motor de-energized and gear-held while weighing; interlocked by SAFE (D-043).")
w("")

# =============================================================================================
# 11. Overload / stops (MET-11)
# =============================================================================================
w("9. Overload (MET-11: >= 1 kg static on the pan)")
sig_fs = (BL["E"] * T_BL * PHI_STOP / (2 * LAM_F)).to(u.MPa)
S1kg = TS["tensions"](1000 * u.g)
sig_ax = (S1kg[0] / (N_FULCRUM * B_BLADE * T_BL)).to(u.MPa)
w(f"   lever stops at +/-{PHI_STOP.m:.0f} mrad (coil +/-{(PHI_STOP * B_SEL).to(u.mm).m:.2f} mm, pan +/-{(PHI_STOP * A).to(u.um).m:.0f} um)")
w(f"   fulcrum blade at the stop with 1 kg on the pan: bending {sig_fs.m:.0f} MPa + axial {sig_ax.m:.0f} MPa "
  f"vs yield {BL['sy'].m:.0f} MPa -> factor {(BL['sy'] / (sig_fs + sig_ax)).to('').m:.0f}")
w("   receptor stops (+/-20 um beyond the lever-stop travel) take overload straight to the base; transport lock clamps the receptor.")
w("")

# =============================================================================================
# 12. Plant estimates for the servo study (to be MEASURED on the rig)
# =============================================================================================
w(f"10. Plant estimates at r = {R_SEL} (servo study input; all to be measured)")
w(f"   J empty {TS['J0'].to(u.g * u.cm**2).m:.0f} g*cm^2, full {TS['J1'].to(u.g * u.cm**2).m:.0f} g*cm^2;  "
  f"K_tot empty {TS['K0'].to(u.mN * u.m).m:.1f}, full {TS['K1'].to(u.mN * u.m).m:.1f} mN*m/rad (tension stiffening), plus the adjustable K_g")
w(f"   open-loop f_n empty {TS['f0'].m:.2f} Hz, full {TS['f1'].m:.2f} Hz; vane at {BS_SEL.m:.0f} mm (1 urad = {(BS_SEL * 1 * u.urad).to(u.nm).m:.0f} nm)")
w(f"   coil force per pan mg: {((1 * u.mg * G0) / R_SEL).to(u.uN).m:.3f} uN -> {(((1 * u.mg * G0) / R_SEL) / KF_NOM).to(u.nA).m:.0f} nA at Kf 5 N/A;"
  f" full scale {(TS['F'] / KF_NOM).to(u.mA).m:.0f} mA")
w("   Soft plant (~1 Hz): the servo supplies nearly all stiffness; null acquisition at power-up starts from a lever stop.")
w("")

# =============================================================================================
# Sanity checks
# =============================================================================================
w("Sanity checks")
K_hand = (BL["E"] * B_BLADE * T_BL**3 / (12 * LAM_F)).to(u.N * u.m)
assert abs((blade_K(BL["E"], T_BL, B_BLADE, LAM_F) - K_hand).m) < 1e-12
w(f"   - Blade K by hand E*b*t^3/(12L) = {K_hand.to(u.mN * u.m).m:.3f} mN*m/rad matches the function. PASS")
assert abs((blade_K(BL["E"], T_BL, B_BLADE, LAM_F, 1e-6 * u.N) / K_hand).to("").m - 1) < 1e-3
w("   - Beam-column K(P->0) -> E*I/L (coth limit). PASS")
Pf, _, _ = TS["tensions"](W_MAX)
kl = ((Pf / N_FULCRUM / (BL["E"] * B_BLADE * T_BL**3 / 12)) ** 0.5 * LAM_F).to("").m
w(f"   - k*L in a fulcrum blade at full load = {kl:.2f}: not << 1, so the exact coth form is used. INFO")
assert abs((W_MAX * G0).to(u.N).m - 5.0995) < 1e-3
w(f"   - 520 g weighs {(W_MAX * G0).to(u.N).m:.4f} N. PASS")
Pf0, _, Fc0 = TS["tensions"](0 * u.g)
mom = (TS["m_cw"] * C_CW + M_TARE * A + TRIM * W_MAX * A - TS["mom_coil"]).to(u.g * u.mm).m
assert abs(mom - 2 * TRIM * W_MAX.m * A.m) < 1e-6
w("   - Counterweight closes the empty-pan moment balance to the +2 % FS trim. PASS")
assert 0.1 < TS["f0"].m < 50
w(f"   - Open-loop f_n {TS['f0'].m:.2f} Hz is in the expected ~Hz range for a soft flexure balance. PASS")

txt = "\n".join(OUT)
(ROOT / "analysis/emfc_mechanics.out.txt").write_text(txt + "\n", encoding="utf-8")
print(txt)
