"""Phase 2 (OI-14): trickler concept sizing - kernel quantization, stage flow rates, tube speed,
belt/motor check, hopper volume, cycle-time budget.

Run:  .venv/Scripts/python analysis/trickler_sizing.py   (writes analysis/trickler_sizing.out.txt)

Status of inputs:
  CONFIRMED  charge 5-150 gr, cycle <= 30 s (DSP-03, DSP-06); hopper <= 1 lb (DSP-08); tol +0.02 gr (DSP-04).
  ASSUMED    kernel geometry, solid and bulk density, tube fill fraction and conveying efficiency.
             Sources for the assumed ranges are listed next to each value; all must be replaced by
             measurements (calipers on 20 kernels per powder; bulk density by cup) before Gate C.
Methods:
  - Kernel mass = solid density x cylinder volume (geometry).
  - Final-kernel overthrow: if kernels arrive singly and the controller stops on the first reading
    >= target, the overshoot is ~uniform on [0, m_k), so P(in [T, T+tol]) = min(1, tol/m_k).
    Simplifying model (outside method, Phase 2 estimate); real arrivals of 2+ kernels make it worse.
  - Rotating tube with an internal helix modeled as a screw conveyor:
    Q = eta * phi * (pi/4) D^2 * p * n   (outside method: screw-conveyor capacity form, CEMA-style);
    phi = fill fraction, eta = conveying efficiency, both ASSUMED and to be measured on the rig.
  - Arching/no-flow at a feed opening: opening >= 6 x particle size (outside rule of thumb for
    coarse free-flowing solids; for elongated kernels the kernel LENGTH is used as particle size).
Design factors: tube-speed results are reported with eta*phi spanning 4x, so the motor/belt choice
covers the whole band. Sanity checks are printed at the end.
"""
from __future__ import annotations

import math
from pathlib import Path

import pint

u = pint.UnitRegistry()
u.define("grain_ = 64.79891 * milligram = gr_")
ROOT = Path(__file__).resolve().parents[1]
out: list[str] = []
w = out.append

# --- CONFIRMED inputs -------------------------------------------------------------------------
CHARGE_MIN, CHARGE_MAX = 5 * u.gr_, 150 * u.gr_
CYCLE_MAX = 30 * u.s
TOL = 0.02 * u.gr_
HOPPER_MASS = 1 * u.lb

# --- ASSUMED inputs ---------------------------------------------------------------------------
RHO_SOLID = 1.6 * u.g / u.cm**3  # nitrocellulose-base propellant, ASSUMED (measure: kernel count x mass)
FORMS = {
    # name: (diameter, length or thickness, shape, bulk density low/high, source)
    "spherical (ball)": (0.6 * u.mm, None, "sphere", (0.95, 1.00), "d ASSUMED 0.4-0.8 mm; bulk ~0.95 g/cc (Ramshot Hunter, third-party)"),
    "extruded short-cut": (0.9 * u.mm, 1.0 * u.mm, "cyl", (0.85, 0.95), "ASSUMED"),
    "extruded long, small": (0.76 * u.mm, 2.2 * u.mm, "cyl", (0.85, 0.95), "0.76 x 2.2 mm: generic military rifle grain (Wikipedia, IMR article)"),
    "extruded long, large": (0.97 * u.mm, 2.1 * u.mm, "cyl", (0.85, 0.95), "0.038 x 0.083 in: IMR 4831 forum measurement (AccurateShooter)"),
    "flake": (1.8 * u.mm, 0.15 * u.mm, "disc", (0.55, 0.65), "ASSUMED disc 1.5-2 mm x 0.1-0.2 mm; bulk ~0.6 g/cc ASSUMED"),
}

# Stage plan (PROPOSED): handover points measured from the target, OpenTrickler-like for coarse.
COARSE_STOP = 4 * u.gr_   # OpenTrickler tuning guide: coarse stop "around 3 to 5 gr"
MEDIUM_STOP = 0.4 * u.gr_  # PROPOSED: leaves ~10-20 long kernels for the fine stage
# Cycle-time budget (PROPOSED), all motors-off windows included
T_COARSE, T_MEDIUM, T_FINE = 12 * u.s, 6 * u.s, 6 * u.s
T_SETTLE_EACH = 1.0 * u.s  # A&D FAST about 1 s (research section 2); Config E goal 1.5 s
N_SETTLES = 3              # after coarse, after medium, final (motors off, DSP-07)
SCALE_LATENCY = 0.5 * u.s  # ASSUMED filter + frame latency; measure with the step test (driver spec section 7)

# Tube geometry (PROPOSED starting points; swappable printed tubes, per powder profile)
TUBES = {  # name: (bore D, helix pitch p, feed-opening size)
    "coarse": (16 * u.mm, 10 * u.mm, 16 * u.mm),
    "medium": (10 * u.mm, 6 * u.mm, 14 * u.mm),
    "fine":   (7 * u.mm, 5 * u.mm, 14 * u.mm),
}
ETA_PHI = (0.05, 0.20)  # eta*phi band, ASSUMED (e.g. phi 0.15-0.35 x eta 0.3-0.6)


def kernel_volume(d, l, shape):
    if shape == "sphere":
        return math.pi / 6 * d**3
    return math.pi / 4 * d**2 * l  # cylinder or disc


# 1. Kernel quantization ------------------------------------------------------------------------
w("1. Kernel mass and final-kernel overthrow (tol = +0.02 gr = %.3f mg)" % TOL.to(u.mg).m)
w(f"   {'form':<22}{'m_k mg':>8}{'m_k gr':>8}{'kernels/150gr':>15}{'P(in tol)':>11}{'pack frac':>10}  source")
for name, (d, l, shape, bulk, src) in FORMS.items():
    mk = (RHO_SOLID * kernel_volume(d, l, shape)).to(u.mg)
    p_ok = min(1.0, (TOL / mk).to("").m)
    n150 = (CHARGE_MAX / mk).to("").m
    pack = (sum(bulk) / 2 * u.g / u.cm**3 / RHO_SOLID).to("").m
    w(f"   {name:<22}{mk.m:>8.3f}{mk.to(u.gr_).m:>8.4f}{n150:>15,.0f}{p_ok:>11.0%}{pack:>10.2f}  {src}")
mk_long = (RHO_SOLID * kernel_volume(0.97 * u.mm, 2.1 * u.mm, "cyl")).to(u.mg)
tol_needed = (mk_long / (1 - 0.10)).to(u.gr_)  # tol such that P(overthrow) <= 10 % in this model
w(f"   -> Long kernels: one kernel ({mk_long.m:.2f} mg) exceeds the +0.02 gr window. For overthrow <= 10 %")
w(f"      (DSP-05 extruded goal) this model needs tol >= {tol_needed.m:.3f} gr. CONTRADICTION with DSP-04 default.")
w("")

# 2. Stage flow requirements ---------------------------------------------------------------------
w("2. Stage mass flow needed (worst case = 150 gr charge)")
m_coarse = CHARGE_MAX - COARSE_STOP
m_medium = COARSE_STOP - MEDIUM_STOP
m_fine = MEDIUM_STOP
flows = {"coarse": m_coarse / T_COARSE, "medium": m_medium / T_MEDIUM, "fine": m_fine / T_FINE}
for st, q in flows.items():
    w(f"   {st:<7} deliver {({'coarse': m_coarse, 'medium': m_medium, 'fine': m_fine}[st]).to(u.gr_).m:7.2f} gr "
      f"in {({'coarse': T_COARSE, 'medium': T_MEDIUM, 'fine': T_FINE}[st]).m:4.0f} s -> {q.to(u.gr_ / u.s).m:6.2f} gr/s "
      f"({q.to(u.mg / u.s).m:7.1f} mg/s)")
turndown = (flows["coarse"] / flows["fine"]).to("").m
w(f"   Coarse/fine turndown {turndown:.0f}:1 -> three stages of ~{turndown ** (1 / 2):.0f}:1 each between neighbours"
  " (why three: one tube can't cover this range)")
# Final-approach limit: flow x latency < half a kernel to keep the last-kernel model valid
q_final_max = (0.5 * mk_long / SCALE_LATENCY).to(u.mg / u.s)
w(f"   Fine final approach (long kernel): keep flow x latency < m_k/2 -> flow < {q_final_max.m:.1f} mg/s"
  f" ({(q_final_max / mk_long).to(1 / u.s).m:.1f} kernel/s); use pulse-and-wait near target")
w("")

# 3. Tube speed (screw-conveyor model) -------------------------------------------------------------
w("3. Tube speed from Q = eta*phi*(pi/4)D^2*p*n  (eta*phi band %.2f-%.2f)" % ETA_PHI)
rpm_results = {}
for st, (D, p, feed) in TUBES.items():
    v_rev = [(e * math.pi / 4 * D**2 * p).to(u.mm**3) for e in ETA_PHI]
    q = flows[st]
    rpm = []
    for bulk in (0.55, 0.95):  # flake (worst volume) .. ball
        rho_b = bulk * u.g / u.cm**3
        qv = (q / rho_b).to(u.mm**3 / u.s)
        rpm.append([(qv / v).to(u.rpm).m for v in v_rev])
    lo = min(min(r) for r in rpm)
    hi = max(max(r) for r in rpm)
    rpm_results[st] = (lo, hi)
    mass_rev = [(v * 0.9 * u.g / u.cm**3).to(u.mg).m for v in v_rev]
    w(f"   {st:<7} D={D.m:>4.0f} mm p={p.m:>3.0f} mm: {v_rev[0].m:6.0f}-{v_rev[1].m:6.0f} mm^3/rev "
      f"(~{mass_rev[0]:.0f}-{mass_rev[1]:.0f} mg/rev at 0.9 g/cc) -> {lo:6.1f}-{hi:6.1f} rpm")
w("   Fine-stage resolution: 1/16 rev pulse = %.1f-%.1f mg at 0.9 g/cc"
  % tuple((e * math.pi / 4 * TUBES['fine'][0]**2 * TUBES['fine'][1] / 16 * 0.9 * u.g / u.cm**3).to(u.mg).m
          for e in ETA_PHI))
w("")

# 4. Feed-opening / bore vs kernel size (no-arching rule of thumb) ------------------------------------
w("4. Feed opening >= 6 x particle size (kernel LENGTH for elongated kernels)")
worst_len = 2.2 * u.mm
need = 6 * worst_len
for st, (D, p, feed) in TUBES.items():
    ok = "OK" if feed >= need else "TOO SMALL"
    pitch_ok = "OK" if p >= 2 * worst_len else "short"
    w(f"   {st:<7} feed {feed.m:4.0f} mm vs {need.m:.1f} mm needed: {ok}; helix pitch {p.m:.0f} mm vs 2 x L = "
      f"{(2 * worst_len).m:.1f} mm: {pitch_ok}")
w("   -> Every tube gets a >= 14 mm feed throat from the hopper, even where the conveying bore is smaller.")
w("")

# 5. Drive: belt ratio, step rate, torque ----------------------------------------------------------
w("5. Drive check (GT2 2 mm pitch; NEMA 17 class stepper; TMC2209 at 1/16 microstep)")
PITCH = 2 * u.mm
def pd(teeth):
    return (teeth * PITCH / math.pi).to(u.mm)
STEPS_REV = 200 * 16
for st, (z_m, z_t) in {"coarse": (20, 40), "medium": (20, 60), "fine": (20, 60)}.items():
    ratio = z_t / z_m
    lo, hi = rpm_results[st]
    motor_hi = hi * ratio
    f_step = (motor_hi * u.rpm * STEPS_REV).to(u.Hz)
    w(f"   {st:<7} {z_m}T:{z_t}T (PD {pd(z_m).m:.1f}/{pd(z_t).m:.1f} mm), ratio {ratio:.0f}:1 -> motor up to "
      f"{motor_hi:6.0f} rpm, STEP up to {f_step.to(u.kHz).m:5.1f} kHz")
# Torque estimate: powder drag + bearing friction, coarse tube worst
D, p, _ = TUBES["coarse"]
L_TUBE = 150 * u.mm  # PROPOSED tube length
powder_in_tube = (0.35 * math.pi / 4 * D**2 * L_TUBE * 1.0 * u.g / u.cm**3).to(u.g)
MU = 0.6  # ASSUMED powder-wall friction
t_powder = (MU * powder_in_tube * u.gravity * D / 2).to(u.mN * u.m)
T_BEARINGS = 2 * 1.0 * u.mN * u.m  # ASSUMED two small ball bearings incl. seals, generous
t_tube = t_powder + T_BEARINGS
t_motor = t_tube / 2  # 2:1 belt
T_NEMA17_HOLD_MIN = 200 * u.mN * u.m  # typical small NEMA 17 (~20 N.cm); confirm with the chosen motor's datasheet
w(f"   Coarse tube torque: powder drag {t_powder.m:.2f} + bearings {T_BEARINGS.m:.1f} = {t_tube.m:.2f} mN.m "
  f"-> {t_motor.m:.2f} mN.m at the motor;")
w(f"   vs ~{T_NEMA17_HOLD_MIN.m:.0f} mN.m holding (typ. small NEMA 17, to confirm): margin "
  f"{(T_NEMA17_HOLD_MIN / t_motor).to('').m:.0f}x. Torque is not the driver; smoothness and "
  "resolution are (run the motor at reduced current).")
w("")

# 6. Hopper volume -------------------------------------------------------------------------------
w("6. Hopper volume for 1 lb")
for label, bulk in (("ball/extruded 0.95", 0.95), ("extruded 0.85", 0.85), ("flake 0.55", 0.55)):
    v = (HOPPER_MASS / (bulk * u.g / u.cm**3)).to(u.cm**3)
    w(f"   {label:<20} {v.m:6.0f} cm^3")
w("   -> Size the hopper for >= 850 cm^3 of usable volume (flake worst case) + 15 % headspace.")
w("")

# 7. Cycle-time budget ---------------------------------------------------------------------------
w("7. Cycle-time budget, 150 gr worst case")
t_total = T_COARSE + T_MEDIUM + T_FINE + N_SETTLES * T_SETTLE_EACH
w(f"   coarse {T_COARSE.m:.0f} + medium {T_MEDIUM.m:.0f} + fine {T_FINE.m:.0f} + {N_SETTLES} settles x "
  f"{T_SETTLE_EACH.m:.1f} s = {t_total.m:.0f} s  vs  {CYCLE_MAX.m:.0f} s limit -> margin {(CYCLE_MAX - t_total).m:.0f} s")
n_pulses = math.ceil((T_FINE / (SCALE_LATENCY + 0.2 * u.s)).to("").m)
w(f"   Fine pulse-and-wait cycles available in {T_FINE.m:.0f} s at {SCALE_LATENCY.m:.1f} s latency + 0.2 s pulse: {n_pulses}")
w("")

# Sanity checks --------------------------------------------------------------------------------
w("Sanity checks")
for name, (d, l, shape, bulk, src) in FORMS.items():
    pack = (sum(bulk) / 2 * u.g / u.cm**3 / RHO_SOLID).to("").m
    assert 0.3 < pack < 0.75, name
w("   - Packing fraction (bulk/solid) 0.34-0.61 for all forms: physically plausible for random packs. PASS")
assert abs(TOL.to(u.mg).m - 0.02 * 64.79891) < 1e-9
w(f"   - 0.02 gr = {TOL.to(u.mg).m:.4f} mg (exact grain definition). PASS")
assert t_total <= CYCLE_MAX
w("   - Cycle budget closes under DSP-06. PASS")

txt = "\n".join(out)
(ROOT / "analysis/trickler_sizing.out.txt").write_text(txt + "\n", encoding="utf-8")
print(txt)
