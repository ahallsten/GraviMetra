"""Extract STM32G491 pin numbers, additional (analog) functions and AF0..AF15 from DS13122.

Source: reference/datasheets/STM32G491_Datasheet.pdf (DS13122 Rev 4), Table 12 (pin definition,
pages 52-62) and Table 13 (alternate functions, pages 63-69).
Run:  .venv/Scripts/python analysis/tools/extract_g4_pins.py
Output: analysis/data/stm32g491_pins.json  (derived data, regenerable; not a vendor file)
"""
import json
import re
from pathlib import Path

import pdfplumber

ROOT = Path(__file__).resolve().parents[2]
PDF = ROOT / "reference/datasheets/STM32G491_Datasheet.pdf"
OUT = ROOT / "analysis/data/stm32g491_pins.json"
PKG_COLS = ["UFQFPN32", "UFQFPN48", "LQFP48", "WLCSP64", "LQFP64", "UFBGA64", "LQFP80", "LQFP100"]
PIN_RE = re.compile(r"^(P[A-G]\d{1,2})")

pdf = pdfplumber.open(PDF)
pins = {}
for pno in range(51, 62):  # Table 12
    for table in pdf.pages[pno].extract_tables():
        for row in table:
            cells = [(c or "").replace("\n", " ").strip() for c in row]
            for i, c in enumerate(cells):
                m = PIN_RE.match(c.replace(" ", ""))
                if m and i >= len(PKG_COLS):
                    nums = cells[i - len(PKG_COLS):i]
                    pins[m.group(1)] = {
                        "name": c,
                        "pkg": {k: v for k, v in zip(PKG_COLS, nums) if v not in ("-", "")},
                        "additional": cells[-1].replace("AD C", "ADC").replace(" ", ""),
                    }
                    break
for pno in range(62, 69):  # Table 13
    for table in pdf.pages[pno].extract_tables():
        for row in table:
            cells = [(c or "").replace("\n", "") for c in row]
            for i, c in enumerate(cells):
                if re.fullmatch(r"P[A-G]\d{1,2}", c):
                    afs = cells[i + 1:i + 17]
                    if len(afs) == 16 and c in pins:
                        pins[c]["af"] = [a for a in afs]
                    break
missing = [p for p in pins if "af" not in pins[p]]
OUT.write_text(json.dumps(pins, indent=1))
print(f"{len(pins)} pins written to {OUT.relative_to(ROOT)}; missing AF rows: {missing}")
