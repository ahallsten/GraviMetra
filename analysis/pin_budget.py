"""Phase 2 (OI-18): controller MCU I/O list, pin budget per package, and a candidate pin map.

Run:  .venv/Scripts/python analysis/pin_budget.py   (writes analysis/pin_budget.out.txt)
Data: analysis/data/stm32g491_pins.json, extracted from DS13122 Rev 4 by analysis/tools/extract_g4_pins.py.

Assumptions (each one is a design choice made here, see docs/phase2/CONTROLLER_IO.md):
  A1  Display = MCUFRIEND 8080 8-bit shield (D-034): D0..D7 + RD/WR/RS/CS/RST. 4-wire touch shares
      WR and RS (analog) with D6/D7 or D0/D1 (digital); CS is also put on an ADC pin to cover shield variants.
  A2  D0..D7 on 8 consecutive bits of one port so one BSRR write sets the byte (LVGL flush speed, OI-12).
  A3  Steppers: one STEP timer output per channel, one shared DIR, EN through a 2-to-4 decoder
      (SEL0/SEL1 + EN_REQ) so only one driver can be enabled in hardware (SAF-04). One TMC2209
      single-wire UART bus with MS1/MS2 addressing. DIAG of the three drivers OR-ed into one input.
  A4  RS-232 port uses TX/RX/RTS/CTS of one U(S)ART (RTS/CTS optional, default unused for A&D).
  A5  EMFC module (Config E) interface reserved now: SPI + CS/DRDY/START/RESET for the ADS1262 and
      8 control/monitor lines. The servo architecture study (Phase 2) may change these 8.
  A6  Rev-B reserved nets (CAN, RS-485, battery sense) get MCU pins only if the package allows;
      IF-03 says the footprints exist but are not populated.
  A7  Excluded from general use: PA11/PA12 (USB), PA13/PA14 (SWD), PB3 (SWO), PF0/PF1 (HSE crystal),
      PB8 (BOOT0, kept as a strap), PG10 (NRST).
  A8  Actuator-control outputs avoid pins with a reset-time pull (RM0440 sec. 9.3.1: PA15/PA13/PB4 pull-up,
      PA14 pull-down) and the UCPD dead-battery CC pins PB4/PB6 (RM0440 PWR_CR3.UCPD1_DBDIS). PC13..PC15
      are low-drive (DS13122 Table 12 note 2) and are used only for slow inputs/LED.
  A9  Interrupt inputs need distinct EXTI line numbers (EXTI line n is shared by all PxN pins).
  A10 Each pulse output (STEP1..3, CM_STEP, BUZZER) gets its own timer instance, because channels of
      one timer share its period (ARR), which would couple their frequencies.  The heater H-bridge is the
      exception: HEAT_IN1/IN2 are two channels of ONE timer (same period, phase-shifted 3-level drive),
      and that timer is kept away from every pulse output.
  A12 POS_A on an ADC1 channel and POS_B on an ADC2 channel for dual simultaneous sampling (ratiometric
      (A-B)/(A+B) of the optical sensor, SERVO_MEASURAND.md).
  A11 LCD D0..D7 only on 5 V-tolerant (FT*) pins. Why: if the shield's 74HC245s run at 5 V (OI-21, unmeasured),
      a bus read drives 5 V into the MCU; TT pins are 3.6 V max (DS13122 Table 12 I/O structure column).
Sanity check: every assignment is re-validated against the AF/ADC tables after solving.
"""
from __future__ import annotations

import itertools
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PINS = json.loads((ROOT / "analysis/data/stm32g491_pins.json").read_text())

RESERVED = {"PA11": "USB_DM", "PA12": "USB_DP", "PA13": "SWDIO", "PA14": "SWCLK", "PB3": "SWO",
            "PF0": "HSE_IN", "PF1": "HSE_OUT", "PB8": "BOOT0 strap", "PG10": "NRST"}
RESET_PULLED = {"PA15", "PA13", "PA14", "PB4"}
UCPD_DB = {"PB4", "PB6"}
LOW_DRIVE = {"PC13", "PC14", "PC15"}


def has_adc(p):
    return "ADC" in PINS[p]["additional"]


def has_dac(p):
    return "DAC" in PINS[p]["additional"]


def af_of(p, signal):
    """Return AF index if `signal` (e.g. 'USART2_TX') is an alternate function of pin p."""
    for i, a in enumerate(PINS[p]["af"]):
        for part in a.split("/"):
            if part == signal or part.startswith(signal + "_"):  # USART2_RTS_DE matches USART2_RTS
                return i
    return None


def timer_channels(p):
    """Non-complementary timer channel outputs on pin p, e.g. ('TIM3', 2)."""
    out = []
    for a in PINS[p]["af"]:
        for m in re.finditer(r"(TIM\d+)_CH(\d)(?!N)", a):
            out.append((m.group(1), int(m.group(2))))
    return out


# ---------------------------------------------------------------------------------------------
# Function list.  kind: gpio | out (actuator-safe output) | exti (interrupt input) | slow (slow in/LED)
#                       adc | dac | tim | af:<GROUP>:<SIGNAL>
# tier: 1 = needed on Rev A, 2 = reserved for Config E (EMFC module), 3 = Rev-B reserved (DNP)
# ---------------------------------------------------------------------------------------------
FUNCS = [
    # HMI (A1, A2): bus handled separately
    ("LCD_RD", "out", 1), ("LCD_WR", "adc", 1), ("LCD_RS", "adc", 1), ("LCD_CS", "adc", 1), ("LCD_RST", "out", 1),
    # Steppers (A3)
    ("STEP1", "tim", 1), ("STEP2", "tim", 1), ("STEP3", "tim", 1), ("MOT_DIR", "out", 1),
    ("MOT_SEL0", "out", 1), ("MOT_SEL1", "out", 1), ("MOT_EN_REQ", "out", 1),
    ("TMC_UART", "af:TMC:TX", 1), ("TMC_DIAG", "exti", 1),
    # Scale RS-232 (A4)
    ("SCALE_TX", "af:SCALE:TX", 1), ("SCALE_RX", "af:SCALE:RX", 1),
    ("SCALE_RTS", "af:SCALE:RTS", 1), ("SCALE_CTS", "af:SCALE:CTS", 1),
    # USB device extra
    ("USB_VBUS_SENSE", "gpio", 1),
    # Temperature sensors (TMP117 ambient on controller, magnet/coil on EMFC module)
    ("I2C_SCL", "af:I2C:SCL", 1), ("I2C_SDA", "af:I2C:SDA", 1),
    # Safety (SAF-01/03)
    ("ESTOP_MON", "exti", 1), ("LID_MON", "exti", 1), ("WDG_KICK", "out", 1),
    ("SAFE_CHAIN_OK", "exti", 1), ("VM_SENSE", "adc", 1),
    # Housekeeping
    ("V12_SENSE", "adc", 1), ("V5_SENSE", "adc", 1), ("BOARD_NTC", "adc", 1),
    ("BUZZER", "tim", 1), ("STATUS_LED", "slow", 1),
    # Config E: ADS1262 on the EMFC module (A5)
    ("ADC_SCK", "af:ADCSPI:SCK", 2), ("ADC_MISO", "af:ADCSPI:MISO", 2), ("ADC_MOSI", "af:ADCSPI:MOSI", 2),
    ("ADC_CS", "out", 2), ("ADC_DRDY", "exti", 2), ("ADC_START", "out", 2), ("ADC_RESET", "out", 2),
    # Config E: EMFC module control (A5, fixed by the servo study, docs/phase2/SERVO_MEASURAND.md)
    ("COIL_EN", "out", 2), ("COIL_FAULT", "exti", 2), ("EMFC_DAC", "dac", 2),
    ("POS_A", "adc1", 2), ("POS_B", "adc2", 2), ("POS_COARSE", "adc", 2), ("LED_EN", "out", 2),
    ("HEAT_IN1", "af:HEAT:CH1", 2), ("HEAT_IN2", "af:HEAT:CH2", 2), ("HEAT_ISENSE", "adc", 2),
    ("CM_STEP", "tim", 2), ("CM_DIR", "out", 2), ("CM_HOME", "exti", 2),
    # Optional sensor
    ("PAN_DET", "slow", 3),
    # Rev-B reserved (A6)
    ("CAN_TX", "af:CAN:TX", 3), ("CAN_RX", "af:CAN:RX", 3),
    ("RS485_TX", "af:RS485:TX", 3), ("RS485_RX", "af:RS485:RX", 3), ("RS485_DE", "out", 3),
    ("BATT_SENSE", "adc", 3),
]
GROUP_INSTANCES = {
    "SCALE": ["USART1", "USART2", "USART3", "UART4", "UART5", "LPUART1"],
    "TMC": ["USART1", "USART2", "USART3", "UART4", "UART5", "LPUART1"],
    "RS485": ["USART1", "USART2", "USART3", "UART4", "UART5", "LPUART1"],
    "I2C": ["I2C1", "I2C2", "I2C3"],
    "ADCSPI": ["SPI1", "SPI2", "SPI3"],
    "CAN": ["FDCAN1", "FDCAN2"],
    "HEAT": ["TIM15", "TIM4", "TIM5", "TIM2", "TIM3"],   # two channels of one timer (heater H-bridge)
}
TIMER_GROUPS = ("HEAT",)
UART_GROUPS = ("SCALE", "TMC", "RS485")


def package_pins(pkg):
    return [p for p in PINS if pkg in PINS[p]["pkg"] and p not in RESERVED]


def candidates(func_kind, p, inst):
    if func_kind == "gpio":
        return p not in LOW_DRIVE and p not in UCPD_DB
    if func_kind == "out":
        return p not in LOW_DRIVE and p not in RESET_PULLED and p not in UCPD_DB
    if func_kind == "exti":
        return p not in UCPD_DB  # slow safety inputs may use PC13..PC15
    if func_kind == "slow":
        return True
    if func_kind == "adc":
        return has_adc(p)
    if func_kind == "adc1":   # ADC1 channel (dual simultaneous mode with adc2)
        return bool(re.search(r"ADC1\d*_IN|ADC12_IN", PINS[p]["additional"]))
    if func_kind == "adc2":
        return bool(re.search(r"ADC2_IN|ADC12_IN", PINS[p]["additional"]))
    if func_kind == "dac":
        return has_dac(p)
    if func_kind == "tim":
        return bool(timer_channels(p)) and p not in RESET_PULLED and p not in UCPD_DB
    if func_kind.startswith("af:"):
        _, grp, sig = func_kind.split(":")
        return af_of(p, f"{inst[grp]}_{sig}") is not None
    raise ValueError(func_kind)


def cost(func_kind, p):
    """Prefer spending plain pins on plain functions (keep ADC/DAC/timer-rich pins free)."""
    c = 0
    if func_kind not in ("adc", "adc1", "adc2") and has_adc(p):
        c += 3
    if func_kind != "dac" and has_dac(p):
        c += 5
    if func_kind != "tim" and timer_channels(p):
        c += 1
    if func_kind != "slow" and p in LOW_DRIVE:
        c += 4
    if func_kind == "slow" and p in LOW_DRIVE:
        c -= 6
    return c


def solve(pkg, funcs, bus):
    """Backtracking assignment. bus = (port, base) for LCD D0..D7 or None to search."""
    avail = package_pins(pkg)
    buses = []
    for port in "EDCBA":
        for base in (0, 8):
            pins = [f"P{port}{base + i}" for i in range(8)]
            if all(p in avail for p in pins) and all(PINS[p].get("io", "").startswith("FT") for p in pins):
                buses.append(pins)
    if bus is not None:
        buses = [b for b in buses if b[0] == f"P{bus[0]}{bus[1]}"]
    groups_needed = sorted({k.split(":")[1] for _, k, _ in funcs if k.startswith("af:")})
    inst_lists = [GROUP_INSTANCES[g] for g in groups_needed]
    for bus_pins in buses:
        free0 = [p for p in avail if p not in bus_pins]
        for combo in itertools.product(*inst_lists):
            inst = dict(zip(groups_needed, combo))
            uarts = [inst[g] for g in UART_GROUPS if g in inst]
            if len(set(uarts)) != len(uarts):
                continue
            res = _bt(funcs, inst, set(free0), {}, set(), {inst[g] for g in TIMER_GROUPS if g in inst})
            if res is not None:
                lcd = {f"LCD_D{i}": p for i, p in enumerate(bus_pins)}
                return {**lcd, **res}, inst
    return None, None


def _bt(funcs, inst, free, assign, exti_used, tim_used):
    if sum(1 for k in assign if not k.endswith("#tim")) == len(funcs):
        return dict(assign)
    # most-constrained-first
    best = None
    for name, kind, _ in funcs:
        if name in assign:
            continue
        cands = [p for p in free if candidates(kind, p, inst)]
        if kind == "exti":
            cands = [p for p in cands if int(p[2:]) not in exti_used]
        if kind == "tim":
            cands = [p for p in cands if any(tc[0] not in tim_used for tc in timer_channels(p))]
        if best is None or len(cands) < len(best[2]):
            best = (name, kind, cands)
        if not cands:
            return None
    name, kind, cands = best
    for p in sorted(cands, key=lambda q: (cost(kind, q), q)):
        assign[name] = p
        free.discard(p)
        added_exti = None
        added_tim = None
        if kind == "exti":
            added_exti = int(p[2:])
            exti_used.add(added_exti)
        if kind == "tim":
            added_tim = next(tc for tc in timer_channels(p) if tc[0] not in tim_used)
            tim_used.add(added_tim[0])
            assign[name] = p
            assign[name + "#tim"] = f"{added_tim[0]}_CH{added_tim[1]}"
        res = _bt(funcs, inst, free, assign, exti_used, tim_used)
        if res is not None:
            return res
        free.add(p)
        del assign[name]
        assign.pop(name + "#tim", None)
        if added_exti is not None:
            exti_used.discard(added_exti)
        if added_tim is not None:
            tim_used.discard(added_tim[0])
    return None


def need_count(tiers):
    return 8 + sum(1 for _, _, t in FUNCS if t in tiers)


def validate(mapping, inst, funcs):
    kinds = {n: k for n, k, _ in funcs}
    for name, p in mapping.items():
        if name.startswith("LCD_D") or name.endswith("#tim"):
            continue
        assert candidates(kinds[name], p, inst), (name, p)
    used = [p for n, p in mapping.items() if not n.endswith("#tim")]
    assert len(used) == len(set(used)), "pin used twice"


def describe(name, p, mapping, inst, kinds):
    k = kinds.get(name, "bus")
    note = []
    if k.startswith("af:"):
        _, grp, sig = k.split(":")
        note.append(f"{inst[grp]}_{sig} (AF{af_of(p, inst[grp] + '_' + sig)})")
    if name + "#tim" in mapping:
        note.append(mapping[name + "#tim"])
    if k in ("adc", "adc1", "adc2"):
        note.append(re.findall(r"ADC\d+_IN\d+", PINS[p]["additional"])[0])
    if k == "dac":
        note.append(re.findall(r"DAC\d_OUT\d", PINS[p]["additional"])[0])
    if k == "exti":
        note.append(f"EXTI{int(p[2:])}")
    return ", ".join(note)


def main():
    lines = []
    w = lines.append
    w("STM32G491 pin budget (OI-18). Data: DS13122 Rev 4 Tables 12-13.")
    w("")
    avail = {pk: len(package_pins(pk)) for pk in ("LQFP64", "LQFP80", "LQFP100")}
    for pk, n in avail.items():
        w(f"{pk}: {len([p for p in PINS if pk in PINS[p]['pkg']])} I/O incl. NRST; "
          f"{n} usable after reserving {', '.join(RESERVED)}")
    w("")
    for tiers, label in (((1,), "Rev A tier 1 (Config S)"), ((1, 2), "tier 1+2 (Config S + E)"),
                         ((1, 2, 3), "tier 1+2+3 (all incl. Rev-B reserved)")):
        need = need_count(tiers)
        w(f"Pins needed, {label}: {need}  ->  " + ", ".join(
            f"{pk} margin {avail[pk] - need:+d}" for pk in avail))
    w("")
    kinds = {n: k for n, k, _ in FUNCS}
    for pk, tiers in (("LQFP64", (1,)), ("LQFP64", (1, 2)), ("LQFP80", (1, 2)), ("LQFP80", (1, 2, 3)),
                      ("LQFP100", (1, 2, 3))):
        funcs = [f for f in FUNCS if f[2] in tiers]
        if need_count(tiers) > avail[pk]:
            w(f"== {pk}, tiers {tiers}: INFEASIBLE by count "
              f"({need_count(tiers)} needed > {avail[pk]} usable)")
            w("")
            continue
        mapping, inst = solve(pk, funcs, None)
        w(f"== {pk}, tiers {tiers}: {'FEASIBLE' if mapping else 'INFEASIBLE (AF/ADC constraints)'}")
        if not mapping:
            continue
        validate(mapping, inst, funcs)
        w(f"   peripheral instances: {inst}")
        if pk == "LQFP100":
            spare = sorted(set(package_pins(pk)) - {p for n, p in mapping.items() if not n.endswith('#tim')})
            w(f"   spare pins ({len(spare)}): {' '.join(spare)}")
            w("")
            w(f"   {'Function':<16}{'Pin':<6}{'LQFP100 #':<10}Note")
            order = [f"LCD_D{i}" for i in range(8)] + [n for n, _, _ in FUNCS]
            for n in order:
                if n in mapping:
                    p = mapping[n]
                    w(f"   {n:<16}{p:<6}{PINS[p]['pkg']['LQFP100']:<10}{describe(n, p, mapping, inst, kinds)}")
        w("")
    txt = "\n".join(lines)
    (ROOT / "analysis/pin_budget.out.txt").write_text(txt + "\n")
    print(txt)


if __name__ == "__main__":
    sys.setrecursionlimit(10000)
    main()
