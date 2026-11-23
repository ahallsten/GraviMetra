# Gate B sourcing checks (OI-03, OI-28, OI-29; early looks at OI-05, OI-06, OI-31)

Rev 0 · 2026-11-22 · prices and stock as seen 2026-11-22 unless marked. All recommendations are PROPOSED or CANDIDATE for the lead and the user.

## Bottom line
- **MCU (OI-03):** the STM32G491VET6 is Active but thin. JLC has 17, the franchised channel has 0 with a 52-week lead. Reserve 4 at JLC now; the fallback is the STM32G474VET6TR/VET3 (50/56 at JLC).
- **Steppers (OI-06) and TMP117:** no issue. TMC2209-LA-T has 23 997 at JLC; TMP117AIDRVR has 5 437. OI-06 can close.
- **Measurand parts (OI-05):** ADS1263IPWR is at JLC (74). LTC6655BHMS8-2.5 is not at JLC but Digi-Key has 980, so consign it.
- **Shunt (OI-05):** the VPG **VSMP and VFCP foil families are end-of-life** (LTB Sep 2025). No catalog foil resistor guarantees 0.2 ppm/K as a *maximum*. Proposed: a Z201 150 Ω through-hole part (±0.2 ± 0.6 ppm/K spread), from VPG's prototype service.
- **Pot steel (OI-28):** at 3/16 and 1/4 in, SendCutSend cuts only A36/A1018 HRP&O (±0.127 mm). The ASSUMED B-H curve is conservative against handbook low-carbon steel, and Km is only ~0.25 % sensitive to it, so order HRP&O. Prices need the user's SendCutSend quote.
- **Magnet (OI-29):** no seller stocks Ø31.75 × 12.7 SmCo (nor the Ø25.4 × 12.7 fallback). The only stocked large axial SmCo disc is First4Magnets Ø30 × 10 mm ($18.21). Use it for the rig after a Km re-run; get the production size by RFQ.
- **Position sensor (OI-31):** the OPR2100 is Active (Rev G 06/2022, Digi-Key 770 at $11.51). It is six separate chips (D / C|E / B|F / A), so the fine split is a die-placement gap; the drift test stays on the rig.

Source rules used here:
- **JLC stock** = JLCPCB parts-library stock read through `jlcsearch.tscircuit.com` (unofficial mirror of the jlcparts database; nightly stock-only sync at 05:00 UTC per its README). jlcpcb.com part pages were read directly for Basic/Extended, but they don't render stock to a fetcher. So every JLC stock and price number below is **UNVERIFIED** (mirror, ≤ 1 day old). Confirm in a browser before ordering.
- **LCSC retail stock** (as opposed to JLC stock) could not be read. lcsc.com renders stock by script.
- **Refused or timed out:** Mouser (timeout), Arrow (timeout), Octopart (403), FindChips (500), RS (403), Avnet (400), analog.com (timeout), st.com product page (timeout), VPG's own datasheet server (script challenge). Digi-Key product pages, ti.com, the ST eStore and jlcpcb.com answered.

## Summary

| Item | Candidate part | Source | Stock / price / date | Status | Recommendation |
|---|---|---|---|---|---|
| OI-03 MCU | **STM32G491VET6** (LQFP100) | JLC C3209873 (Extended); Digi-Key 497-STM32G491VET6-ND; ST eStore | JLC **17 pcs**, $7.50 (qty 1–9) / $6.42 (10–29). Digi-Key **0**, 52 wk, $10.95 / $8.51 (10). ST eStore out of stock. 2026-11-22 | Active (Digi-Key, ST eStore) | Keep. Reserve 4 in the JLC parts library now (2 units + 2 spares) |
| OI-03 alt | STM32G474VET6TR / STM32G474VET3 | JLC C1857438 / C2970837 | JLC 50 pcs $7.04 / $6.07; 56 pcs $9.88 / $8.64. Tray -VET6 (C431632) 0 at JLC; Digi-Key tray 0, 52 wk, $12.13 | Active (Digi-Key) | Fallback only; same LQFP100 family, pin map to re-check |
| OI-05 ADC | **ADS1263IPWR** | JLC C2861295; Digi-Key 296-42485-1-ND | JLC 74 pcs, $15.56 / $14.86. Digi-Key 2 614, 26 wk, $29.45 / $23.77 | ACTIVE (ti.com) | OK. JLC-placeable |
| OI-05 ADC alt | ADS1262IPWR | — | 0 at JLC | ACTIVE (ti.com) | Not needed (D-064 picked ADS1263) |
| OI-05 ref | **LTC6655BHMS8-2.5#PBF** | Digi-Key 505-LTC6655BHMS8-2.5#PBF-ND | **0 at JLC.** Digi-Key 980, 21 wk, $19.04 / $15.16 | Active (Digi-Key) | Buy at Digi-Key; JLC places it as a consigned part (or hand-solder MSOP-8) |
| OI-05 ref alt | LTC6655BHLS8-2.5#PBF (LCC-8) | Digi-Key 505-LTC6655BHLS8-2.5#PBF-ND | 0 (576 due 19-May-2027), 25 wk, $22.53 | Active | Not for Rev A (no stock) |
| OI-05 ref alt | LTC6655LNBHMS8-2.5#PBF | JLC C1236879 | JLC 35 pcs, $25.34 | — | JLC-stocked B-grade 2.5 V option; LN adds a noise-filter pin (check footprint) |
| OI-05 op-amp | **OPA2388IDR** / IDGKR | JLC C1517414 / C2059281 | 16 469 / 1 713 pcs, $2.29 | — | OK |
| OI-05 shunt | **VPG Z201 150 Ω ±0.01 %, THT** (Y1453150R000T9L); SMD alternative FRSM1206 150 Ω ±0.02 % (Y4023150R000Q9R) | Digi-Key (non-stocked lines); VPG prototype service | FRSM1206 not stocked at Digi-Key, 47 wk, ~$33 at MOQ 10 (2.8 kΩ example). Z201 100 Ω: 8 pcs, $46.17, 38 wk; Z201 200 Ω: 0 (25 due 24-Dec-2026), $30.66. No 150 Ω listing found | **VSMP and VFCP are EOL** (PCN-VPG FR-005-2024: LTB Sep 2025) | See §2.4. No part meets "TCR ≤ 0.2 ppm/K **max**" off the shelf |
| OI-06 | **TMC2209-LA-T** | JLC C2150710; Digi-Key 175-TMC2209-LA-TCT-ND | JLC **23 997 pcs**, $2.73 / $2.44. Digi-Key 27 138, 24 wk, $5.85 / $4.48 | Active (Digi-Key) | Use the -T (reel) part number. OI-06 closes |
| OI-28 | Pot steel **A36/A1018 HRP&O**, .187 in (4.75 mm) and .250 in (6.35 mm) | SendCutSend material pages | cut tol. ±0.127 mm; min hole 50 % of t; thickness tol., flatness and cert not stated; **price not obtained** (needs upload/app) | only low-carbon grade cut at these thicknesses (1008 CR stops at 3.43 mm) | Order it; keep the ASSUMED B-H (§4) |
| OI-29 | SmCo Ø31.75 × 12.7 axial | 10 sellers checked | **not stocked anywhere**; nearest stock: First4Magnets **Ø30 × 10 mm, $18.21, in stock** (grade not stated) | made to order (Arnold, Adams, Dura, SAM, YXG makers) | Rig: Ø30 × 10 after a Km re-run. Production: RFQ with OI-27 |
| OI-31 | **OPR2100** (TT/Optek, 6-element SMD PIN array) | ttelectronics.com datasheet Rev G 06/2022; Digi-Key 365-1870-ND | Digi-Key **770**, $11.51 (1), 12 wk | **Active** (TT, Digi-Key); only the -HS variant is obsolete | Keep for the rig; Hamamatsu S4349 or OSI QD7-0-SD quadrant as monolithic fallback |
| TMP117 | **TMP117AIDRVR** | JLC C699536 | JLC **5 437 pcs**, $1.06 / $0.93 | — | OK, 3 per Config E unit |

## 1. OI-03 MCU (STM32G491VET6, LQFP100)

| Source | Part / SKU | Stock | Price qty 1 / 2 / 10 | Lead time | Status | URL (2026-11-22) |
|---|---|---|---|---|---|---|
| JLC (via jlcsearch) | C3209873, **Extended** | 17 | $7.50 / $7.50 / $6.42 | — | — | https://jlcpcb.com/partdetail/STMicroelectronics-STM32G491VET6/C3209873 ; https://jlcsearch.tscircuit.com/components/list.json?search=STM32G491VET6 |
| LCSC retail | C3209873 | not readable (a search-engine snippet said ~409, date unknown: UNVERIFIED) | — | — | — | https://www.lcsc.com/product-detail/C3209873.html |
| Digi-Key | 497-STM32G491VET6-ND | 0 (backorder) | $10.95 / $10.95 / $8.51 | 52 wk | Active | https://www.digikey.com/en/products/detail/stmicroelectronics/STM32G491VET6/13592604 |
| ST eStore | STM32G491VET6 (heading shows -VET6TR) | Out of stock | — | — | Active | https://estore.st.com/en/stm32g491vet6-cpn.html |
| Mouser, Arrow, st.com | — | not reachable (timeouts) | — | — | — | — |

Alternatives at JLC (same mirror, same date):

| Part | JLC # | Stock | $ qty 1–9 / 10–29 | Note |
|---|---|---|---|---|
| STM32G474VET6 (tray) | C431632, Extended | 0 (not in the in-stock list) | — | Digi-Key 497-19476-ND: 0, 52 wk, $12.13 / $9.46, Active |
| STM32G474VET6TR | C1857438 | 50 | $7.04 / $6.07 | Reel part number, same die |
| STM32G474VET3 | C2970837 | 56 | $9.88 / $8.64 | −40…125 °C grade |
| STM32G491RET6 (LQFP64) | C3231005 | 87 | $6.80 / $5.94 | Not usable: LQFP64 is 11 pins short for S+E (D-040) |

- **Bottom line:** the G491VET6 is Active but thin everywhere. JLC holds 17; the franchised channel is at zero with a 52-week factory lead time. Two units need 2 MCUs; 17 covers Rev A only if nobody else buys them first.
- **Recommendation (PROPOSED):** keep the G491VET6 (D-040, D-069). The user pre-orders 4 pcs into the JLC private parts library at the next order. Why: JLC is the only channel with stock today, and JLC places all SMT (FAB-01).
- **Fallback (CANDIDATE):** STM32G474VET6TR or -VET3 (50 and 56 at JLC). Why: same LQFP100 G4 family, more SRAM (128 KB) and HRTIM, about the same price. The pin map must be re-run in CubeMX before it is treated as drop-in (UNVERIFIED: not checked here).

## 2. OI-05 early check (no final selection)

### 2.1 ADC
| Part | Source | Stock | Price 1 / 10 | Lead time | Status |
|---|---|---|---|---|---|
| ADS1263IPWR (TSSOP-28) | JLC C2861295 (Extended) | 74 | $15.56 / $14.86 | — | — |
| ADS1263IPWR | Digi-Key 296-42485-1-ND | 2 614 | $29.45 / $23.77 | 26 wk | Active |
| ADS1262IPWR | JLC | 0 | — | — | — |
| ADS1263 / ADS1262 | ti.com | — | — | — | **ACTIVE**; datasheet SBAS661 **Rev C, 25 May 2021** = the local copy |

URLs: https://www.ti.com/product/ADS1263 · https://www.ti.com/product/ADS1262 · https://www.digikey.com/en/products/detail/texas-instruments/ADS1263IPWR/5356687

### 2.2 Reference
| Part | Source | Stock | Price 1 / 10 | Lead time | Status |
|---|---|---|---|---|---|
| LTC6655BHMS8-2.5#PBF | JLC | **0** | — | — | — |
| LTC6655BHMS8-2.5#PBF | Digi-Key 505-LTC6655BHMS8-2.5#PBF-ND | 980 | $19.04 / $15.16 | 21 wk | Active |
| LTC6655BHLS8-2.5#PBF (LS8) | Digi-Key 505-LTC6655BHLS8-2.5#PBF-ND | 0 (576 due 19-May-2027) | $22.53 / $18.03 | 25 wk | Active |
| LTC6655LNBHMS8-2.5#PBF | JLC C1236879 | 35 | $25.34 | — | — |
| LTC6655CHMS8-2.5#PBF (C grade) | JLC C689441 | 13 | $13.08 | — | — |

JLC holds other B-grade voltages (4.096 V 68 pcs, 3.3 V 44, 1.25 V 44, 5 V 9) but no B-grade 2.5 V in MSOP or LS8.
URLs: https://www.digikey.com/en/products/detail/analog-devices-inc/LTC6655BHMS8-2-5-PBF/2138766 · https://www.digikey.com/en/products/detail/analog-devices-inc/LTC6655BHLS8-2-5-PBF/3873217 · https://jlcsearch.tscircuit.com/components/list.json?search=LTC6655

- **Recommendation (PROPOSED):** LTC6655BHMS8-2.5#PBF bought at Digi-Key and consigned to JLC (or hand-soldered). Why: it is the D-064 grade, Digi-Key has 980, and the LS8 package has no stock until May 2027. The JLC-stocked LN-B variant is a fallback only if its extra filter pin fits the footprint (UNVERIFIED: pinout not checked here).

### 2.3 Op-amp
OPA2388IDR, JLC C1517414: 16 469 pcs, $2.29 (1–9). OPA2388IDGKR, JLC C2059281: 1 713 pcs, $2.29. No issue.

### 2.4 Shunt, ~150 Ω VPG foil

**Finding 1 — VSMP and VFCP are end-of-life.** PCN-VPG FR-005-2024 says the following. TTI hosts the file under a "Rev-1" name; its first page reads "Rev 0", 22-Aug-2024.
- Affected: VSMP (0603–2512) and VFCP (0805–2512).
- Last-time buy Sep 2025; last shipment Jan 2026.
- Replacements: **FRSM** (wraparound) and **FRFC** (flip chip), "new generation Z1 foil technology".
- Source: https://www.ttieurope.com/content/dam/tti-europe/products/PCN/VishayPrecisionGroup/VPGFoilResistor-PCN-VPG%20FR-005-2024-Rev-1.pdf

Digi-Key now shows Y1625150R000F9W (VSMP1206 150 Ω) and Y1630100R000Q9R (VFCP1206 100 Ω) as **Obsolete**.

**Finding 2 — no standard part guarantees TCR ≤ 0.2 ppm/K max.** VPG quotes a nominal (typical) TCR plus a "max spread", both over −55…+125 °C with a +25 °C reference:

| Family (datasheet read) | Size, power at 70 °C | TCR typ, 0…60 °C | TCR nominal ± max spread, −55…+125 °C, at 100–250 Ω | PCR | Load life, 70 °C 2000 h at rated P | Tol. at 150 Ω |
|---|---|---|---|---|---|---|
| FRSM (Z1 foil), doc 4220-EN Rev 25-Mar-2025 | 1206: 300 mW | not stated in the copy read | **±0.2 ± 1.8 ppm/K** | line not legible in the copy read (UNVERIFIED; VPG Z-foil family value is 5 ppm at rated P) | ±0.0025 % typ (25 ppm); ±0.005 % at 10 000 h | ±0.02 % |
| Z201 (Z-foil, THT radial, 3.81 mm pitch), doc 63187 Rev 22-Oct-13 (Farnell copy) | 0.6 W | ±0.05 ppm/K nominal | **±0.2 ± 0.6 ppm/K** (100 Ω–600 kΩ) | 5 ppm at rated P | ±0.005 % (50 ppm) | ±0.005 % |
| VSMP (EOL), doc 63060 Rev 25-Mar-10 (RS copy) | 1206: 300 mW | ±0.05 typ | ±0.2 ± 1.8 ppm/K | 5 ppm | ±0.005 % at 150 mW | ±0.02 % |
| VFCP (EOL), doc 63106 Rev 04-Nov-2013 (Farnell copy) | 1206: 250 mW | ±0.05 nominal | ±0.2 ± 1.6 ppm/K | 5 ppm | ±0.005 % at 150 mW | ±0.02 % |

VPG's own datasheet server (vpgfoilresistors.com) served a script challenge, so the newest revisions weren't read. The copies above came from distributor hosts (LCSC, Farnell, RS); their revision dates are listed.

Candidate part numbers (150 Ω; codes built from each datasheet's ordering table):

| # | Part number | Family | Seen at a distributor? | Stock / price / lead time (2026-11-22) |
|---|---|---|---|---|
| 1 | **Y4023150R000Q9R** | FRSM1206, ±0.02 %, reel | No 150 Ω listing found | FRSM1206 at Digi-Key is "not kept in stock": Y40232K80000B5W, MOQ 10 at $32.65, 47 wk. Y4023350R000Q9R listed, 0 stock |
| 2 | **Y1453150R000T9L** | Z201, ±0.01 %, THT | No 150 Ω listing found (Avnet shows -V9L at 0 stock, 224 wk: search snippet, UNVERIFIED) | Neighbours at Digi-Key: Y1453100R000T9L 8 pcs, $46.17 / $33.44, 38 wk; Y1453200R000T9L 0 (25 due 24-Dec-2026), $30.66 / $21.95, 27 wk; Y1453120R000T0L non-stocked |
| 3 | Y1625150R000Q9R | VSMP1206 (EOL) | Newark 0 stock, ~$28–38 (search snippet, UNVERIFIED) | Remnant stock only. **Don't design in** |
| 4 | RWC150R0QL | Alpha/VPG RWC 1206 | Digi-Key: 0 (250 due 02-Feb-2027), $15.72, 22 wk | **TCR ±2 ppm/K: fails the spec**, listed only to rule it out |

URLs: https://www.digikey.com/en/products/detail/vishay-foil-resistors-division-of-vishay-precision-group/Y1453100R000T9L/1888097 · https://www.digikey.com/en/products/detail/vpg-foil-resistors/Y1453200R000T9L/4228201 · https://www.digikey.com/en/products/detail/vpg-foil-resistors/Y40232K80000B5W/21833253 · https://www.digikey.com/en/products/detail/vpg-foil-resistors/RWC150R0QL/27565984 · FRSM datasheet copy: https://atta.szlcsc.com/upload/public/pdf/source/20260129/2756F11B81B15372F0D120D8369D6090.pdf

What this means for the design:
- **Self-heating is small.** At 12.5 mA (120 % FS) the shunt dissipates 23 mW. That is 8 % of the FRSM1206 rating and 4 % of the Z201's. Scaled linearly, PCR is ≈ 0.4 ppm (FRSM1206) or ≈ 0.2 ppm (Z201) at full load (ASSUMED: PCR ∝ P). Both are well under the 3.85 ppm MET-04 allowance.
- **The 0.2 ppm/K figure is a typical, not a maximum.** Worst case is 0.8 ppm/K (Z201) or 2.0 ppm/K (FRSM) over −55…+125 °C. Across 15–30 °C near the 25 °C reference the chord slope is much smaller (the ±0.05 ppm/K 0…60 °C figure), but that is typical too.
  - The electronics span tempco is compensated against a TMP117 anyway (SERVO_MEASURAND §7). So what matters is a repeatable TCR, which in-situ adjustment learns, not a small one.
  - D-064's "TCR ≤ 0.2 ppm/K" should be read as **typical**, or VPG asked for a selected max-TCR part (the datasheets say "for tighter performances … contact").
- **Recommendation (PROPOSED):** **Z201, Y1453150R000T9L.** Why: it has the tightest published spread (±0.6 ppm/K) and the lowest self-heating at 0.6 W, and as a radial THT part it is hand-soldered, which FAB-01 already allows.
  - Source: VPG's prototype service ("prototype quantities in 5 working days", Z201 and VSMP datasheets) or a distributor special order.
  - Second choice: FRSM1206 Y4023150R000Q9R, if an SMD part on the JLC board is preferred. JLC has no VPG foil in stock, so it would be consigned.
  - The value can move to 120–150 Ω if only a stocked value is available. 200 Ω puts 2.5 V across the shunt at 12.5 mA, which is the ADS1263's full scale with a 2.5 V reference, so it is too high.

## 3. OI-06 TMC2209

| Part | Source | Stock | Price 1 / 10 | Lead time | Status |
|---|---|---|---|---|---|
| **TMC2209-LA-T** (QFN-28, reel P/N) | JLC C2150710 (Extended) | **23 997** | $2.73 / $2.44 | — | — |
| TMC2209-LA-T | Digi-Key 175-TMC2209-LA-TCT-ND | 27 138 | $5.85 / $4.48 | 24 wk | Active |
| TMC2209-LA (tray P/N) | JLC | 0 (not in the in-stock list) | — | — | a third-party list marks it "Mature", with -LA-T as the FFF equivalent (UNVERIFIED) |
| TMC2226-SA-T (HTSSOP-28) | JLC C705366 | 1 311 | $5.34 / $4.68 | — | different package, not a footprint drop-in |
| TMC2225-SA-T (HTSSOP-28) | JLC C516358 | 9 340 | $2.81 / $2.42 | — | no StallGuard, different package |
| TMC2208-LA-T (QFN-28) | JLC C115944 | 8 473 | $2.35 / $2.06 | — | same footprint; no StallGuard, no UART address pins (check if needed) |
| TMC2240ATJ+T (TQFN-32) | JLC C7429724 | 3 879 | $7.85 / $6.86 | — | SPI/UART, 36 V; different footprint |

- **Bottom line:** the "21 at LCSC" in OI-06 was the tray part number. The reel part number TMC2209-LA-T has ~24 k at JLC and ~27 k at Digi-Key.
- **Recommendation (PROPOSED):** put **TMC2209-LA-T (C2150710)** in the BOM and close OI-06. Why: same die and footprint, deep stock at the assembler.
- Lifecycle on analog.com could not be read (timeout). Digi-Key shows Active.

## 4. OI-28 Pot steel at SendCutSend

### 4.1 What SendCutSend cuts (read 2026-11-22)

| Steel | Thicknesses | At 3/16 in and 1/4 in? | Source |
|---|---|---|---|
| **"A36/A1018 HRP&O"** (hot-rolled, pickled and oiled; "a blended specification with tighter thickness and surface finish tolerances") | .187, .250, .313 in; .375 and .500 in are listed as "A36 Hot Rolled" | **Yes: .187 in (listed as 4.75 mm) and .250 in (6.35 mm)** | https://sendcutsend.com/materials/mild-steel/hrpo/ |
| 1008 Cold Rolled ("about 0.08 % C") | .030, .048, .059, .074, .104, .119, .135 in | **No.** Thickest is .135 in (3.43 mm) | https://sendcutsend.com/materials/mild-steel/cold-rolled/ |
| 4130 chromoly | .050, .063, .125, .190, .250 in | Yes, but an alloy steel: magnetically worse, not wanted | https://sendcutsend.com/materials/ |
| 1095 high carbon | .125, .187 in | Yes, but semi-hard magnetically: not wanted | same |
| AR400 / AR500, G90 galvanized | — | not suitable | same |

"A1018" in SendCutSend's label is the **ASTM A1018 sheet specification** (hot-rolled heavy-thickness coil), not AISI 1018 composition. SendCutSend's own text puts A36 at "about 0.25 %" C. 1010 and 1020 are not offered.

| Property | SendCutSend statement | Note |
|---|---|---|
| Cut tolerance | "±.005″" for all mild-steel thicknesses (±0.127 mm) | Gap budget check below |
| Thickness tolerance | **not stated** ("tighter thickness … tolerances" than mill finish, no number) | Mill spec. UNVERIFIED; measure every part |
| Flatness | not stated | — |
| Minimum hole | laser: "at least 50 % of material thickness", i.e. 2.4 mm at .187 in and 3.2 mm at .250 in | https://sendcutsend.com/guidelines/sheet-cutting/ |
| Minimum part size | 0.5 × 0.75 in on the min/max page; .25 × .375 in on the HRP&O page (SendCutSend's two pages disagree) | Our parts are ≥ Ø31.75 mm, so either is fine. https://sendcutsend.com/materials/min-max/ |
| Mill cert | not mentioned | ask at order time if wanted |
| Price for a Ø56 disc / 47.75 × 56 ring | **not obtained.** Instant pricing needs a file upload or the Parts Builder inside the customer app; no account was made. The pricing page says no minimum quantity and free US shipping ≥ $39, and gives an example "2 × 2 in mild steel part" at $19.51 (thickness not stated, "general estimate") | https://sendcutsend.com/pricing/ |

**Cut tolerance vs the 0.35 mm clearance.** ±0.127 mm on a diameter is ±0.064 mm on the radius.
- So the pole OD moves the inner clearance by ±0.064 mm, the top-plate ID moves the outer one by ±0.064 mm, and the total radial gap moves by up to ±0.13 mm.
- With the coil centred in the gap and the pole at the ≤ 0.1 mm concentricity target (MAGNET_COIL §9), the tight side drops from 0.35 to ≈ 0.19 mm. That is still positive, but the margin is smaller than assumed.
- Measure the cut pole OD and top-plate ID before winding the coil to the 3.0 mm build. The gap gauge already in the assembly method sets centring, not size.
- Laser kerf taper on 4.75–6.35 mm plate is not quantified by SendCutSend (UNVERIFIED).

**Thickness and the magnet stack.** HRP&O thickness tolerance is unstated, and the "½ in magnet = two ¼ in wall rings" stack depends on it. Micrometer every ring and pair them. The shim under the magnet (MAGNET_COIL §9) takes up the difference.

### 4.2 B-H data vs the ASSUMED curve in `analysis/magnet_coil.py`

- **No published B-H curve was found for ASTM A36 or A1018** (searched IEEE/COMSOL/vendor pages).
- The best citable data is FEMM's digitisation of the *Metals Handbook*, 8th ed., Vol. 1 (ASM 1961), Fig. 17 p. 792, DC magnetisation curves: https://www.femm.info/wiki/SoftMagneticMaterials. The data zip is https://www.femm.info/examples/matlib/scannedcurves.zip (curves 05, 07, 08, 10, 11).
- The ASSUMED curve was read from the script (lines 103–104), not re-run. The comparison was made with a scratch script (interpolation only).

B (T) at H (A/m):

| Curve | 200 | 400 | 600 | 1000 | 2000 | 4000 | 8000 |
|---|---|---|---|---|---|---|---|
| **ASSUMED (magnet_coil.py)** | 0.40 | 0.80 | 1.00 | 1.20 | 1.40 | 1.55 | 1.65 |
| Hot-rolled low-carbon steel (MH) | 0.77 | 1.19 | 1.32 | 1.44 | 1.58 | 1.67 | 1.78 |
| Cold-rolled low-carbon strip (MH) | 0.42 | 0.99 | 1.21 | 1.39 | 1.53 | 1.65 | 1.78 |
| Cold-drawn carbon steel, annealed (MH) | 0.52 | 1.00 | 1.23 | 1.41 | 1.55 | 1.67 | 1.79 |
| Carbon-steel forgings, annealed (MH) | 0.17 | 0.43 | 0.73 | 1.07 | 1.38 | 1.59 | 1.75 |
| Steel castings, as cast (MH) | 0.19 | 0.52 | 0.75 | 1.04 | 1.32 | 1.53 | 1.72 |

H (A/m) needed at the pot's operating flux densities (MAGNET_COIL §5: pole rim 0.80, wall 0.95, base 1.18 T mean; 1.31 T 99th percentile):

| Curve | 0.80 T | 0.95 T | 1.18 T | 1.31 T |
|---|---|---|---|---|
| **ASSUMED** | 400 | 550 | 960 | 1 550 |
| Hot-rolled low-carbon (MH) | 207 | 253 | 393 | 578 |
| Cold-rolled low-carbon strip (MH) | 307 | 375 | 565 | 771 |
| Forgings, annealed (MH) | 661 | 821 | 1 263 | 1 688 |
| Castings, as cast (MH) | 651 | 846 | 1 346 | 1 926 |

- **The ASSUMED curve is 1.7–2.4× conservative** against the handbook low-carbon hot-rolled and cold-rolled curves at 1.18 T. It is ~25 % optimistic against the forgings and castings curves (medium-carbon class).
- A36/A1018 has 0.25 % C and up to ~1 % Mn, so it should sit between "hot-rolled low-carbon" and "forgings" (ASSUMED: no A36 measurement found). The ASSUMED curve is roughly the middle of that bracket.
- **Km sensitivity is small** (hand estimate, outside method: series reluctance from MAGNET_COIL §5 numbers):
  - The steel takes ~40–50 A of the ~1.77 kA external MMF.
  - Magnet internal reluctance is ≈ 11.7 MA/Wb (Ø31.75 × 12.7, μrec 1.09); the external path is ≈ 2.5 MA/Wb.
  - A B-H 50 % worse than ASSUMED (+25 A) costs **≈ 0.25 % of Km**, against the 2.7× Km margin.
  - Confirm by re-running `analysis/magnet_coil.py` with the forgings curve as a lower bound (the lead's action; not done here).
- **Possible error-budget input (UNRESOLVED).** By the same estimate the steel is ≈ 0.5 % of the total circuit reluctance. So a steel-permeability temperature coefficient of X ppm/K enters the span at ≈ 0.005·X ppm/K.
  - No source was found for the permeability tempco of mild steel near 1 T and 20 °C.
  - It is absorbed by the in-situ α learning (D-062) with standard Sm2Co17. It may matter at the 1–5 ppm/K level only with a TC magnet.

### 4.3 Recommendation (PROPOSED)
- **Order A36/A1018 HRP&O, .187 in and .250 in, from SendCutSend. Keep the ASSUMED B-H curve.** Why: it is the only low-carbon steel SendCutSend cuts at those thicknesses, and Km moves ≈ 0.25 % for a 50 % worse curve.
- Optional: a stress-relief or full anneal at a heat treater after cutting, to restore permeability at the cut edges. Not required for Km; only worth it if the rig shows load-history effects.
- **Not recommended:** stacking 1008 CR sheets to make 4.76 / 6.35 mm. 0.135 + 0.119 in = 6.45 mm works for the wall, but the 4.76 plate has no clean combination, and every bond line is an extra gap and tolerance.

## 5. OI-29 SmCo discs

**Bottom line: no seller checked stocks the Ø31.75 × 12.7 mm (1¼ × ½ in) disc, or the Ø25.4 × 12.7 fallback.**
- Both are quote / made-to-order sizes.
- The closest stocked axial SmCo disc is **First4Magnets Ø30 × 10 mm at $18.21** (in stock), with the grade not stated.

| Seller | Size (mm) | Grade / Br / tempco | Coating | Tolerance | Price | Stock / lead | Source (2026-11-22) |
|---|---|---|---|---|---|---|---|
| **First4Magnets** F3010SC(-1) | **Ø30 × 10**, poles on faces | "samarium cobalt"; **grade, Br, α not stated**; 300 °C (572 °F) max suggests 2:17 (UNVERIFIED) | none ("resistant … without any extra coating") | ±0.1 mm | **$18.21** (1), $16.90 (5), $15.82 (10), $13.99 (20) | **In stock**, next-day | https://www.first4magnets.com/us/product/30mm-dia-x-10mm-thick-samarium-cobalt-magnet-139kg-pull-19173 |
| First4Magnets F999N-SC | Ø25 × 20 | not stated | — | — | £24.82 ex VAT (UK) | not shown | https://www.first4magnets.com/circular-disc-rod/samarium-cobalt-magnets |
| First4Magnets / Magnet Expert | 30 × 12, 32 × 12, 25 × 12, 31.75 × 12.7, 25.4 × 12.7 | — | — | — | — | **not listed** | same |
| Adams Magnetic 55B0063 | **Ø38.1 × 12.7** (1.5 × 0.5 in), through thickness | grade 26 (MGOe) | — | — | not shown ("additional sizes … upon request") | not shown | https://www.adamsmagnetic.com/samarium-cobalt/samarium-cobalt-disc-magnets/ |
| Dura Magnetics | stock discs up to Ø19.05 × 12.7 (SC7550) | grade 26 | — | — | not shown | nothing ≥ 1 in | https://www.duramag.com/samarium-cobalt-magnets-smco/samarium-cobalt-stock-magnet-shapes/ |
| McMaster-Carr | SmCo discs < 1 in only (largest 63/64 in, encased) | SmCo 28 (2:17), SmCo 16 (1:5) | Ni-plated cup | — | not shown | — | https://www.mcmaster.com/products/samarium-cobalt-magnets/ |
| Stanford Advanced Materials SMG9208 | Ø25.4 × 9.5 (1 × 3/8 in) | 26M Sm2Co17 | none | not listed | on inquiry | — | https://samaterials.com/smds0016-smco-disc-magnet.html |
| Stanford Magnets | disc pages marked "Discontinued"; custom sizes by quote | — | — | — | quote | — | https://www.stanfordmagnets.com/smco-disc-magnets.html (search snippet, UNVERIFIED) |
| Apex Magnets | SmCo30 discs up to 1 × 1/8 in | SmCo30 | Ni | — | 1 × 1/8 in $18.99 (search snippet; unclear if each or 4-pack, UNVERIFIED) | in stock | https://www.apexmagnets.com/magnets/1-x-1-8-disc-smco-samarium-cobalt |
| Eclipse Magnetics | Ø32 × 7 SmCo5 in a steel pot (E777/E767) | SmCo5 | steel cup | — | — | — | Digi-Key listings (search snippets); **not usable** (cup, 1:5) |
| Arnold, CMS Magnetics | no catalog listing found | Recoma 28 by quote (Arnold) | — | — | quote | — | — |
| Chinese makers (YXG-28/30) | made to order | YXG-28 (28 ± 2 MGOe), 350 °C | — | — | — | MOQ 100 at one maker (made-in-china listing, UNVERIFIED) | — |

Recommendation (PROPOSED, for the lead / error budget):
1. **Rig magnet now: First4Magnets Ø30 × 10 mm**, 2 pcs ($36). Why: it is the only stocked large axial SmCo disc, and the rig needs a magnet before a custom order could arrive.
   - The pot keeps its imperial plates. A 2.7 mm steel spacer under the magnet makes up the 12.7 mm stack (MAGNET_COIL §9 already allows a shim).
   - Km drops with the smaller magnet (≈ 70 % of the volume). **Re-run `analysis/magnet_coil.py` with Ø30 × 10 before ordering.** The ≥ 2.0 Km margin rule is expected to hold, but this is UNVERIFIED.
   - Ask First4Magnets for the grade, Br and α in writing: 1:5 vs 2:17 changes α from −450 to −350 ppm/K.
2. **Production magnet: RFQ for Ø31.75 × 12.7 Recoma 28 / YXG-28 axial.** Arnold, Adams, Dura and SAM all offer custom sizes. Bundle it with the TC-grade quote (OI-27) in one RFQ (user action; Claude can draft it).
3. Adams Ø38.1 × 12.7 grade 26 is a stock size with more flux, but it needs a ~64 mm pot (over the 60 mm selection rule). Keep it in reserve only if the TC grade leaves too little Km.

## 6. OI-31 Optical position sensor (OPR2100)

**What it is.** TT Electronics / Optek "Six-Element SMD Photodiode Array", designed for motor encoders. Datasheet **Rev G 06/2022**, read at https://www.ttelectronics.com/TTElectronics/media/ProductFiles/Datasheet/OPR2100.pdf.
- Six separate PIN chips in an opaque polyimide chip carrier, each on an isolated cathode contact, so every element is wired out on its own.
- Variants: OPR2100 (tube) and OPR2100T (reel) at 55 kHz; OPR2100HST (reel) at 200 kHz; **OPR2100HS is obsolete**.

| Parameter (TA 25 °C) | Value |
|---|---|
| Element layout (drawing) | Four rows: **D** (top, centred) / **C \| E** (side-by-side pair) / **B \| F** (pair) / **A** (bottom, centred). Row pitch 0.071 in (1.8 mm) |
| Element size | 2.73 × 1.34 mm, 3.66 mm² per diode (the ordering table says 3.45 mm²: internal inconsistency) |
| Responsivity | 0.45 A/W min at 890 nm, 0 V |
| Dark current | 10 nA max at VR 10 V |
| Capacitance | 10 pF typ at VR 10 V |
| Bandwidth | 55 kHz (OPR2100/T), 200 kHz (HST) |
| Temperature range | −55 … +125 °C; package θ 142 °C/W |

| Source | Stock | Price | Lead time | Status |
|---|---|---|---|---|
| Digi-Key 365-1870-ND (OPR2100, tube) | **770** | $11.51 (1), $10.31 (100) | 12 wk | **Active** |
| TT Electronics product page | — | — | — | **Active**; mentions PCN 1119 for OPR2100/T/HST (content not read: UNVERIFIED) |
| Digi-Key OPR2100T | 0 (search snippet, UNVERIFIED) | — | — | — |

Fit to D-067 (assessment, not a test):
- The C|E and B|F pairs give two side-by-side splits for the **fine** differential channel. D and A above and below give the **coarse** channel along the column. That matches the "centre cells fine, outer cells coarse" plan, if the vane edge runs along the column.
- The fine split is a **die-placement gap between two separate chips**, not a monolithic split.
  - Null position and its drift then depend on die attach and on the two chips' responsivity match and tempco ("closely matched", no number given).
  - Ratiometric (A−B)/(A+B) removes LED drift, but not chip-to-chip mismatch drift.
  - This is exactly the OI-31 rig test (≤ 1 nm/√Hz, 100 nm per cycle).
- 55 kHz bandwidth meets ≥ 5 kHz. Run at 0 V bias (photovoltaic) for the lowest dark-current drift. Capacitance at 0 V will be above the 10 pF/10 V figure; that is the TIA design's concern.

Alternatives (monolithic segmented detectors, through-hole):

| Part | Type | Stock / price (2026-11-22) | Note |
|---|---|---|---|
| **Hamamatsu S4349** | Si quadrant, 2 × 2, TO-5, 190–1000 nm | RS UK £37.43 (search snippet; RS blocks fetches: UNVERIFIED); a broker claims ~1 855 | Monolithic quad with a defined gap; datasheet PDF text is encrypted, specs not extracted (UNVERIFIED). https://www.hamamatsu.com/content/dam/hamamatsu-photonics/sites/documents/99_SALES_LIBRARY/ssd/s4349_kmpd1007e.pdf |
| **OSI QD7-0-SD** | Si quadrant, 7 mm², TO-5, 0.54 A/W at 900 nm, 15 nA dark | Digi-Key 2640-QD7-0-SD-ND: **67**, **$400.14** (1), 14 wk, Active | In stock but ~35× the OPR2100 price. https://www.digikey.com/en/products/detail/osi-optoelectronics-inc/QD7-0-SD/12323121 |
| Advanced Photonix PDB-C201 | Si bi-cell 2 × 3.23 mm², TO-5 | Digi-Key: not stocked, **500 MOQ** at CAD 59.98, 10 wk | True bi-cell, but MOQ rules it out |
| OSI SPOT-2D | Si bi-cell 1.3 × 2.5 mm, 0.127 mm gap | RS lists it **Discontinued** (search snippet, UNVERIFIED) | — |

- **Recommendation (CANDIDATE):** keep the OPR2100 for the rig (2 pcs, $23). Buy one Hamamatsu S4349 as the monolithic comparison if the split-drift test fails. Why: the OPR2100 is Active, stocked and cheap, and its outer cells give the coarse channel for free. The S4349 is the cheapest monolithic quad found.

## 7. TMP117 (3 per Config E unit)

| Part | JLC # | Stock | $ qty 1–9 / 10–29 |
|---|---|---|---|
| **TMP117AIDRVR** (WSON-6) | C699536 | 5 437 | $1.06 / $0.93 |
| TMP117MAIDRVR (WSON-6) | C2803242 | 250 | $1.74 / $1.47 |
| TMP117NAIDRVR (WSON-6) | C2876169 | 153 | $2.62 / $2.23 |
| TMP117AIYBGR (DSBGA-6) | C2871893 | 1 926 | $2.10 / $1.81 |

- **Recommendation (PROPOSED):** TMP117AIDRVR. Why: it is the base orderable part with deep JLC stock. No sourcing issue.
- The M/N variant differences were not checked here (see the TI variant table if a variant is wanted).

## Contradictions with existing docs

1. **OI-06** says "21 at LCSC". That was the tray part number. The reel part number TMC2209-LA-T has 23 997 at JLC and 27 138 at Digi-Key.
2. **D-064 / OI-05** ask for a shunt with "TCR ≤ 0.2 ppm/K". VPG gives 0.2 ppm/K as a **typical**, with a max spread of ±0.6 (Z201) to ±1.8 ppm/K (FRSM, VSMP) over −55…+125 °C. Also, **VSMP and VFCP are EOL** (LTB Sep 2025), so any VSMP/VFCP part named in the docs must go.
3. **MAGNET_COIL §9** prefers "low-carbon cold-rolled steel … over A36". SendCutSend cuts cold-rolled (1008) only up to 3.43 mm; at 4.75/6.35 mm it offers only A36/A1018 HRP&O. Its "1018" label is the ASTM A1018 sheet spec, not AISI 1018.
4. **MAGNET_COIL §4** uses 4.76 mm for 3/16 in; SendCutSend lists .187 in = 4.75 mm. That is 0.01 mm, negligible, but the nominal in the model is not the stock size.
5. **datasheets.md** "SmCo disc listings" row: "First4magnets (Ø25 Sm2Co17 ~$32)". Today First4Magnets lists a Ø25 × 20 (£24.82) and a Ø30 × 10 ($18.21), plus a Ø25 × 10 only as an M4 pot magnet. None states "Sm2Co17" or a grade.
6. **OPR2100 datasheet**: per-element active area 3.45 mm² (ordering table) vs 3.66 mm² (2.73 × 1.34 mm, characteristics table).
7. **SendCutSend minimum part size**: 0.5 × 0.75 in (min/max page) vs .25 × .375 in (HRP&O page). Irrelevant for our parts.

## Not verified here
- Every JLC stock and price (read through the jlcsearch mirror). LCSC retail stock. Mouser and Arrow (timeouts).
- st.com lifecycle page (timeout; the ST eStore and Digi-Key say Active). analog.com lifecycle for LTC6655 and TMC2209 (timeouts).
- The newest VPG datasheet revisions (script challenge). The FRSM PCR line (illegible in the copy read). Any listing of a 150 Ω Z201 or FRSM part.
- SendCutSend prices, thickness tolerance and flatness. Any measured A36 B-H curve. The permeability tempco of mild steel.
- The First4Magnets grade, Br and α. Hamamatsu S4349 specs and US stock. TT PCN 1119 content. STM32G474 pin compatibility with the G491 map.
