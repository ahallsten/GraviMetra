"""Phase 1: derived numbers behind docs/01_REQUIREMENTS.md.

Run:  python3 analysis/req_numbers.py
Every input is labeled with its source; the outputs are the numbers quoted in the requirements.
"""
import pint

u = pint.UnitRegistry()
u.define("grain_ = 64.79891 * milligram = gr_")  # exact definition of the grain

# --- Inputs -----------------------------------------------------------------
CAP = 520 * u.g            # AD520 capacity (CONFIRMED)
D = 1 * u.mg               # readability (CONFIRMED)
REP = 1 * u.mg             # repeatability SD (CONFIRMED)
LIN = 2 * u.mg             # linearity +/- (CONFIRMED)
CHARGE_MIN = 5 * u.gr_     # CONFIRMED (Q4)
CHARGE_MAX = 150 * u.gr_   # CONFIRMED (Q4)
TOL_STEP = 0.02 * u.gr_    # display step in grains / default acceptance tolerance
DRIFT_BENCH = 2e-6 / u.K   # A&D FX-i sensitivity drift +/-2 ppm/degC (10-30 degC), benchmark
SMCO_TC = [0.03e-2 / u.K, 0.05e-2 / u.K]  # Sm-Co Br reversible tempco range (verify per grade)
HOPPER = 1 * u.lb          # CONFIRMED (Q5)

# OIML R111-1 MPE (+/- mg) for selected nominal values
MPE = {  # nominal_g: {class: mg}
    10: {"E2": 0.06, "F1": 0.20},
    20: {"E2": 0.08, "F1": 0.25},
    50: {"E2": 0.10, "F1": 0.30},
    100: {"E2": 0.15, "F1": 0.50},
    200: {"E2": 0.30, "F1": 1.0},
    500: {"E2": 0.75, "F1": 2.5},
}


def q(x, unit, nd=3):
    return f"{x.to(unit).magnitude:.{nd}g} {unit}"


print("=== Metrology scale of the problem ===")
divisions = (CAP / D).to("dimensionless").magnitude
print(f"Divisions Max/d            : {divisions:,.0f}")
print(f"Linearity as ppm of FS     : {q(LIN / CAP, 'ppm')}")
print(f"Repeatability as ppm of FS : {q(REP / CAP, 'ppm')}")

print("\n=== Grain conversions ===")
print(f"Charge range               : {q(CHARGE_MIN, 'g')} .. {q(CHARGE_MAX, 'g')}")
print(f"0.02 gr display step       : {q(TOL_STEP, 'mg')}")
print(f"1 mg in grains             : {q(D, 'gr_', 4)}")
print(f"3-sigma repeatability      : {q(3 * REP, 'mg')} = {q(3 * REP, 'gr_', 3)}")

print("\n=== Span drift (per kelvin) ===")
print(f"Benchmark 2 ppm/K at Max   : {q(DRIFT_BENCH * CAP * u.K, 'mg')}/K")
print(f"Benchmark 2 ppm/K at 150 gr: {q(DRIFT_BENCH * CHARGE_MAX * u.K, 'mg')}/K")
for tc in SMCO_TC:
    raw_full = (tc * CAP * u.K).to("mg")
    raw_charge = (tc * 10 * u.g * u.K).to("mg")
    t_err_full = (D / (tc * CAP)).to("mK")  # magnet-temp error that gives 1 mg at Max
    t_err_10g = (D / (tc * 10 * u.g)).to("mK")
    print(
        f"SmCo {tc.to('percent/K').magnitude:.2f} %/K: raw {raw_full:.0f~P}/K at Max, "
        f"{raw_charge:.1f~P}/K at 10 g coil load; temp error for 1 mg: "
        f"{t_err_full:.1f~P} (Max) / {t_err_10g:.0f~P} (10 g)"
    )

print("\n=== Verification masses vs +/-2 mg linearity (1/3 rule -> MPE <= 0.67 mg) ===")
limit = LIN.to("mg").magnitude / 3
for nom, d in MPE.items():
    flags = {c: ("OK" if v <= limit else "TOO COARSE") for c, v in d.items()}
    print(f"{nom:>4} g  E2 {d['E2']:.2f} mg {flags['E2']:<10}  F1 {d['F1']:.2f} mg {flags['F1']}")
print("OIML guidance: balances with >100,000 d call for E2-class test weights.")

print("\n=== Hopper ===")
print(f"1 lb resident powder       : {q(HOPPER, 'g', 4)} = {q(HOPPER, 'gr_', 5)}")
