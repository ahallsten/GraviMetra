# EMFC Scale BOM audit (rev 2026-11-21)

Date checked: **2026-11-21** (all stock/price/lifecycle figures below were pulled from the pages named, today, with WebSearch/WebFetch only).
Source BOM: `EMFC_Scale_Octopart_BOM_1.xlsx` (25 lines; "Sourcing Audit" sheet = 2026-08-17 snapshot, used only for comparison).

## 0. Read this first

**Legend.** `[V]` = read on the page today. `[U]` = unverified / not retrievable / inferred. `Ext` / `Basic` = JLCPCB library type as printed on the JLC part-detail page. "LCSC" = lcsc.com listing (the "C" number is also the JLCPCB part number). JLC does not publish its own assembly stock on the part-detail page I could fetch, so **"JLC stock" below is LCSC stock, which is the closest proxy [U]**.

**Method limits you should know about**
- Digi-Key regional sites were the only DK URLs the tools would fetch for many parts (.ca = CAD, .cn = CNY incl. 13 % VAT, .co.uk = GBP, .it = EUR). Currency is stated on every figure. Stock on a regional page may not equal digikey.com stock. Re-check on digikey.com at order time.
- Mouser product pages could not be fetched. Mouser rows come only from Octopart pages (aggregator, cache age unknown). Most lines have **no Mouser data**; marked `[U]`.
- PCBWay: no public per-part stock page exists that I could query; availability is quote-time only. **PCBWay = not determinable [U] for every line.** Use the LCSC C-numbers as a hint only.
- JLC type (Basic/Extended) was verified on the JLC page for the lines marked `[V]`; the rest are `[U]`. Every IC in this BOM that I did verify is Extended. JLC charges a per-part-type fee for Extended parts; check the current fee on jlcpcb.com [U].

**Requirement conflict to resolve.** Your message sets AD520-class: 1 mg readability, +/-2 mg linearity (about 1.9 ppm and 3.85 ppm of 520 g). The project context file (`EMFC_Project_Context_and_Design_Audit.md`, section 3) says 0.01 grain (0.648 mg) readability and +/-0.5 mg accuracy. I used your latest message (2 ppm / 4 ppm). The 0.648 mg version is 1.25 ppm resolution and would be tougher on the reference, shunt and ADC noise. Please confirm.

**Engineering note that affects the alternates (my analysis, not a datasheet claim).** The force reading comes from the ADS1262 measuring the voltage across the shunt. The coil driver sits inside the position servo (integral action), so its offset and offset drift do not enter the mass reading. What does matter for the driver: noise, stability into the inductive coil, self-heating near the precision board, and hardware coil-current limiting (a safety item in the context file). The shunt and reference are in the measurement path directly (span).

## 1. Design changes applied

| Change | Effect on BOM |
|---|---|
| TMC2209-LA 4 -> 3 | qty 3 |
| Nextion NX8048T050 | excluded (HMI undecided) |
| RS-232 transceiver (3.3 V, MAX3232-class) | added, line 26, three options in section 4 |
| IRM-30-12, IRM-20-5 | lifecycle/spec check only (owned) |
| Precision target | 520 g / 1 mg / +/-2 mg = ~1.9 ppm resolution, ~3.85 ppm linearity of FS |

## 2. Full BOM table

Prices are qty-1 unless noted. Currency in brackets where not USD.

| # | MPN (qty) | Lifecycle | Digi-Key (today) | Mouser | LCSC # / JLC type / LCSC stock + price | PCBWay | Flags |
|---|---|---|---|---|---|---|---|
| 1 | **STM32G491RET6** (1) | Active [U: ST page not fetched; Octopart "Production" [V]] | **0 in stock** ("out of stock, can be backordered"), mfr lead **52 wk** [V, digikey.com]. Aug snapshot said 1,859. | 31 @ $9.80, tray [V via Octopart, cache age unknown] | C3231005 / type [U] / **53** @ $11.57 [V] | [U] | **LOW STOCK, LONG LEAD.** Same-family fallbacks seen on JLC (not stock-checked): STM32G491RCT3 C6829929, STM32G491CCT6 C5270322, STM32G491VET6 C3209873, STM32G491REI6 C3228560 [U]. |
| 2 | **ADS1262IPW** (1) | Active, 20 wk lead [V] | **343** @ $27.77 (tube) [V] | 111 @ $27.77 [V via Octopart] | IPW tube: C122460 / **Ext** [V] / 85 @ $10.79 [V]. **IPWR reel: C2669963 / type [U] / 1,806 @ $9.47 [V]** | [U] | Single source (TI). Use the **IPWR reel C2669963** for JLC; it is the same silicon in a stocked reel. Metrology-chain part. |
| 3 | **OPA593DNTT** (1) | Active (TI page) | **0**; 250 expected 27-May-2027; $13.90 (USD) / C$22.01 [V]. DNTR listing: only a 5,000-pc reel tier at $7.87 shown, stock not shown [V] | not retrievable [U] | DNTR: C7430247 / **Ext** [V] / **out of stock**, $10.85 [V]. No DNTT at LCSC. | [U] | **PRODUCTION HOLD.** Single source, zero stock everywhere checked. See section 3A. |
| 4 | **OPA2388IDR** (2) | Active [V, TI page] | **2,680** @ $3.84 [V, .com.br page, USD] | [U] | C1517414 / type [U] / **16,429** @ $1.34 [V] | [U] | Max supply 5.5 V (2.5-5.5 V) [V]. Metrology chain. |
| 5 | **LTC6655BHMS8-2.5#PBF** (1) | PRODUCTION (all 2.5 V models) [V, ADI page] | **601** @ $19.04 [V via Octopart, cache age unknown]; DK .com.au 980 @ A$28.12 [V]; lead 21 wk [V] | not listed [V via Octopart] | B grade: C107983 / type [U] / **out of stock**, $13.90 [V] | [U] | No longer "25 in stock" at DK; **LCSC out**. Single source. See 3B. |
| 6 | **LT3045IMSE#PBF** (1) | Active, 20 wk [V] | **6,828** @ $11.37 [V] | [U] | I-grade: C580014 / **Ext** [V] / stock [U]. E-grade: C580012 / type [U] / 1,086 @ $5.61 [V] | [U] | Metrology chain (analog supply). If I-grade (-40 C) is not needed, E-grade C580012 is the stocked JLC part. |
| 7 | **TMUX1101DBVR** (1) | Active [V] | 5,263 @ CNY 20.67 (incl. VAT), lead 12 wk [V, .cn] | [U] | **No genuine TI part at LCSC found.** Only a "TECH PUBLIC" clone C47117642 [V in search]. Do not use a clone in the reset/leakage path. | [U] | **Hand-source.** |
| 8 | **TMP117AIDRVR** (3) | Active [V] | **0** on DK.com page; page lists restocks 3 Nov and 16 Nov 2026, both already past, so page is stale [V]. $6.29 [V] | not on Octopart snapshot [V] | C699536 / **Ext** [V] / **6,000** @ $1.07 [V] | [U] | DK stock unreliable today; LCSC deep. Metrology chain. |
| 9 | **OPR2100** (1) | Active [V] | **770** @ C$18.22 (tube), 12 wk [V, .ca] | [U] | none found | [U] | **Hand-source.** New mechanical footprint (BOM note). Single source (TT). |
| 10 | **TSAL6200** (1) | Active (Vishay page title; status not read) [U] | Aug snapshot 45,482 @ $0.50 [not re-checked] | [U] | C55528 / type [U] / **3,790** @ from $0.075 (tier unspecified) [V] | [U] | 5 mm through-hole: JLC THT assembly is a separate/manual process [U]; consider hand-soldering. |
| 11 | **Y160610R0000E9W** (1) | Active | **0**, mfr lead **39 wk**, qty-50 price tier GBP 24.45 on sibling D9R page [V]. E9W page itself not fetchable [U] | [U] | none found | [U] | **PRODUCTION HOLD.** Siblings E9R and D9R: 0 qty. See 3C. |
| 12 | ~~NX8048T050~~ | excluded | - | - | - | - | Removed per design change. |
| 13 | **USB4105-GF-A** (1) | Active | **86,010** @ $0.80 [V] | 397,607 @ $0.80 [V via Octopart] | C3020560 / **Ext** [V] / **8,815** @ $0.56 [V] | [U] | Healthy. |
| 14 | **TPD2EUSB30ADRTR** (1) | Active | **81,759** @ $1.15 [V] | [U] | Genuine TI: C94934 (JLC page, type not stated) / "available order qty 4,268" [V]. Clones C3011197 (Tech Public), C2997271 (Born) exist. | [U] | Use genuine C94934. |
| 15 | **TCAN1044AVDRQ1** (1) | Active [U] | **0**; 2,500 expected 26-Jan-2027; CNY 9.38 [V, .cn] | [U] | C3234993 / type [U] / **4,334** @ $0.57 [V] | [U] | DK out until Jan; LCSC fine. |
| 16 | **THVD2450DR** (1) | Active, 26 wk [V] | **0**; 2,380 expected 28-Dec-2026 and 120 on 8-Mar-2027 [V, .cn]. Price not readable [U] | [U] | C2672621 / type [U] (THVD1450DR sibling is Ext) / **24,145** @ $0.59 [V] | [U] | DK out until late Dec; LCSC fine. |
| 17 | **BQ25792RQMR** (1) | Active (spec table) but DK .hk page also says "no longer carried by DigiKey" [V, conflicting] | stock [U] | [U] | C2862876 / type [U] / **5,897** @ $1.26 [V] | [U] | Redesign item already flagged in BOM. DK carriage unclear: treat LCSC as the source. |
| 18 | **TPS61088RHLT** (1) | Active (Octopart "Production") | **54** @ C$18.27 [V, .ca, price looks inconsistent, treat [U]] | RHLR reel 991 @ $2.74 [V via Octopart] | RHLR reel: C87357 / **Ext** [V]. RHLT: C92483 / type [U]. LCSC stock [U] | [U] | Use **RHLR** (reel) for JLC. DK RHLT thin (54). |
| 19 | **TPS62135RGXT** (1) | Active [V] | **0**, 250 expected 8-Jul-2027, C$4.89 [V, .ca]. **RGXR cut-tape**: 5,441 @ $2.32 [V via Octopart] | RGXR 13,020 @ $2.32 [V via Octopart] | RGXR: C167238 / type [U] / **114** @ $1.28 [V] | [U] | **Switch to RGXR** (T-suffix is unstocked). LCSC stock low (114). |
| 20 | **TPS62172DSGT** (1) | Active [U] | **3,465** @ C$2.96 [V, .ca] | [U] | DSGR: C59873 / type [U] / **2,601** @ $1.03 [V] | [U] | Healthy. |
| 21 | **TPS2121RUXT** (2) | Active [U] | **908** @ C$4.96 [V, .ca] | [U] | RUXR: C485916 / **Ext** [V] / **31,728** @ $0.94 [V]. RUXT: C2156025 [U] | [U] | Healthy. |
| 22 | **TPS259470ARPWR** (2) | Active [U] | stock [U]; price CNY 16.13 [V, .cn] | [U] | C3662799 / type [U] / **323** @ $1.30 [V] | [U] | LCSC stock modest. |
| 23 | **IRM-30-12** (1, owned) | Production [V Octopart]; DK status Active, 17 wk lead [V] | stock/price not readable [U] | Octopart lists Sager 2,298 @ $12.90 etc. [V] | C193294 / **Ext** [V] / **4,107** @ $8.86 [V] | [U] | Spec [V]: 85-305 VAC, 12 V 2.5 A, 30 W, 88 %, -30 to +70 C with derating, CB/CE/cURus/TUV. Check only. |
| 24 | **IRM-20-5** (1, owned) | Production [V Octopart]; DK Active [V] | **2,504** @ $9.50 [V, .com.mx page] | [U] | C6280121 / type [U] / **753** @ $6.37 [V] | [U] | Spec [V]: 85-305 VAC, 5 V 4 A, 20 W. Check only. |
| 25 | **TMC2209-LA** (3, was 4) | **Recommended for new designs** [V, ADI page] | **4,006** @ C$9.89 (tray) [V, .ca]; 27 wk lead [V] | [U] | C465949 / **Ext** [V] / **21** @ $1.72 [V] (price looks like a volume tier) | [U] | **LCSC stock 21**: enough for 3 pcs but fragile. Needs sense resistors, exposed-pad copper. |
| 26 | **RS-232 transceiver (new)** | see section 4 | - | - | see section 4 | [U] | Added line. |

Changes vs the Aug-2026 snapshot worth noting: STM32G491RET6 (DK 1,859 -> 0), TPS62135RGXT (2,000 -> 0), TCAN1044AVDRQ1 (24,369 -> 0), THVD2450DR (6,879 -> 0), TMP117 (1,327 -> 0 on DK.com), TPS61088RHLT (1,269 -> 54), TPS2121RUXT (3,150 -> 908), IRM-20-5 (7,452 -> 2,504). Some of these may be regional-page effects; re-check on digikey.com.

## 3. PRODUCTION HOLD alternates

### 3A. OPA593DNTT (coil driver)

**What the original part gives** [V, TI datasheet SBOS659C, product page]: 8-85 V (+/-4 to +/-42.5 V), 250 mA output, Vos +/-20 uV typ / +/-100 uV max, drift +/-0.4 uV/C typ / +/-2 uV/C max, input bias +/-1 pA typ, 7 nV/rtHz at 10 kHz, **2.9 uVpp** 0.1-10 Hz, 10 MHz GBW, ILIMIT pin (resistor to V-, 25-250 mA settings with accuracy table), open-drain current flag and thermal flag, thermal alarm ~170 C (reset ~150 C), E/D enable pin, WSON-12 4x4, RthetaJA 40.8 C/W, Iq 3.25 mA.

**Suffix codes.** TI lists OPA593DNTT (250-pc small reel), OPA593DNTR (large reel), and OPA593DNTRG4 [V]. All are the same WSON-12 DNT die/package. Digi-Key: DNTT 0 stock with 250 expected 27-May-2027; DNTR shows only a 5,000-piece tape-and-reel tier at $7.87 with stock not shown [V]. LCSC/JLC carry the DNTR as C7430247 (Ext, out of stock, $10.85) [V]. **No other package option for OPA593 was found.** So this is a supply problem, not a pinout problem.

**Side-by-side** (all figures [V] from the datasheet/page named unless marked):

| Part | Iout | Supply | Vos (typ / max), drift | Noise | Thermal protection | Current-limit pin | Package | JLC availability |
|---|---|---|---|---|---|---|---|---|
| **OPA593** ([TI](https://www.ti.com/lit/gpn/OPA593)) | 250 mA | 8-85 V | 20 / 100 uV; 0.4 / 2 uV/C | 7 nV/rtHz @10 kHz; 2.9 uVpp 0.1-10 Hz | yes, flag, ~170 C | **yes, ILIMIT (25-250 mA)** + flag | WSON-12 4x4 | DNTR C7430247 Ext, **out of stock** |
| **OPA192** ([TI](https://ti.com/document-viewer/OPA192/datasheet)) | ~+/-65 mA (feature line; short-circuit not read [U]) | 4.5-36 V | +/-5 uV typ (max not read) ; +/-0.2 uV/C typ | 5.5 nV/rtHz @1 kHz; 0.1-10 Hz not read | not read [U] | no | SOIC-8, SOT-23-5, VSSOP-8 (dual OPA2192 SOIC-8) | OPA192IDR C2861286, OPA192IDBVR C139648, OPA2192IDR C110074 listed; stock/type [U] |
| **BUF634A** in the OPA192 loop ([TI](https://ti.com/document-viewer/BUF634A/datasheet/GUID-4A3AE191-7C69-48AA-8709-23D7C36929C6)) | 250 mA | "36 V device" (range not read [U]) | in-loop, so n/a | not read [U] | yes, thermal shutdown + internal current limit (not adjustable) | no (fixed internal) | SOIC-8, VSON-8 3x3 (DRB), HSOIC-8 (DDA) | BUF634AIDR C2058426, BUF634AIDDAR C2864582 listed; stock/type [U] |
| **OPA569** ([TI](https://ti.com/lit/gpn/opa569)) | 2 A (2.4 A max DC) | **2.7-5.5 V only** | 0.5 mV / 2 mV; 1.3 uV/C typ | 12 nV/rtHz @1 kHz; 8 uVpp 0.1-10 Hz | yes, flag at ~147-150 C, can be tied to enable | **yes** (0.2-2.2 A, RSET) | SO-20 PowerPAD (DWP) | OPA569AIDWPR C529573 listed; Active per TI addendum 13-Feb-2026; stock/type [U] |
| **OPA564** ([TI](https://ti.com/document-viewer/OPA564-Q1/datasheet/GUID-B04677FE-99F0-4E66-892B-B079F06195BD)) | 1.5 A | 7-24 V (+/-3.5 to +/-12 V) | **not retrievable [U]** | not retrievable [U] | yes, flag + TSENSE | yes, adjustable + flag | SO-20 PowerPAD (DWP) | LCSC C201690 (OPA564AIDWDR) listed; stock/type [U] |
| **OPA548** ([TI](https://www.ti.com/document-viewer/OPA548/datasheet/electrical-characteristics-sbos0702318)) | 3 A cont., 5 A peak | 8-60 V (+/-4 to +/-30 V) | 2 mV / 10 mV; **30 uV/C** | **90 nV/rtHz** @1 kHz | yes, 160 C shutdown, reset 140 C, status pin | yes, resistor-set | TO-220-7, DDPAK-7 | only OPA548FKTWT C32710510 seen [U] |
| **ADA4870** ([ADI](https://static.chipdip.ru/lib/319/DOC022319376.pdf)) | 1 A, limit ~1.2 A | 10-40 V | -1 mV typ, -15/+10 mV; 4 uV/C (14 at +/-5 V) | 2.1 nV/rtHz | yes, TFL flag + shutdown pin | no (fixed ~1.2 A) | PSOP-20 | ADA4870ARRZ C654272, ARRZ-RL C157584 listed; stock/type [U] |
| **LT1210** ([ADI](https://sites.units.it/carrato/didatt/ESD_web/doc/cfa/LT1210.pdf)) | 1.1 A min | +/-5 to +/-15 V | 3 / 15 mV; 10 uV/C | 3 nV/rtHz; inv. current noise 40 pA/rtHz | thermal shutdown (cycling) | no | DDPAK-7, TO-220-7, SO-16 | not checked [U] |
| **TCA0372** (onsemi, via [datasheets.com](https://www.datasheets.com/onsemi/tca0372bdp1)) | 1 A/ch, dual | 5-40 V single | 15 mV; drift/noise not read [U] | not read [U] | not confirmed [U] | not confirmed [U] | PDIP-8, SOIC-16 | not checked [U] |

OPA2991 + discrete stage: not researched, no data retrieved. The OPA192 + buffer route covers the same idea with parts I could verify.

**Ranking (my judgment)**
1. **OPA593DNTR, same die and footprint.** Zero redesign, keeps ILIMIT and flags. Risk is purely supply: every suffix is out of stock today, so buy through TI.com / franchised backorder, and for the EVT build use option 2 so the project is not blocked.
2. **Precision amp + in-loop BUF634A (or a discrete complementary emitter follower), for example OPA192 or one half of OPA2192.** Keeps in-loop precision and low noise (5.5 nV/rtHz) with parts that JLC lists. **Changes:** schematic and footprint (two SOIC-8 instead of WSON-12), loop compensation with the coil inductance must be re-simulated, **no ILIMIT pin, so you need an external hardware current clamp and an enable path**, no flags. Also keep total supply low: Iq of BUF634A is 1.5-8.5 mA [V]. OPA2388 cannot be used here: 5.5 V max supply [V].
3. **OPA569AIDWP**, only if coil + shunt + headroom fit under 5.5 V total supply. One IC, keeps current limit, thermal flag, and enable. Cost: Vos 0.5 mV (irrelevant inside the loop), 8 uVpp noise (about 2.8x OPA593), SO-20 PowerPAD footprint, and the 5.5 V ceiling. **Not suitable if the coil needs the 12 V rail.**
4. **Not recommended:** OPA548 (30 uV/C, 90 nV/rtHz), ADA4870 (32 mA quiescent = 0.3-0.65 W of self-heating next to a 2-ppm measurement), LT1210 (current-feedback topology, 10 uV/C), TCA0372 (15 mV offset, class-AB crossover), OPA564 (data not retrievable). They are cheaper/larger but compromise noise or heat for a 60 mA job.

**Open item.** Coil compliance voltage and current limit are not frozen in the context file (it says ~50-60 mA, preliminary). Options 2 and 3 depend on that number.

### 3B. LTC6655BHMS8-2.5#PBF (2.5 V reference)

All specs [V] from the LTC6655/LTC6655LN datasheet ([ADI](https://www-cce.cldnet.analog.com/media/en/technical-documentation/data-sheets/ltc6655-6655ln.pdf)) unless noted.

**Grades/suffixes.** B: +/-0.025 %, tempco 1 typ / **2 max** ppm/C. C: +/-0.05 %, 2.5 typ / 5 max. Noise, drift, hysteresis, current are the same for B and C. MS8 plastic: 0.25 ppm p-p (625 nV p-p at 2.5 V), 0.1-10 Hz; long-term drift 60 ppm/rt-kHr; hysteresis 20 ppm (0-70 C), 30 ppm (-40 to 85 C), 60 ppm (-40 to 125 C). LS8 ceramic: drift 20 ppm/rt-kHr, hysteresis 5/30/80 ppm, tray only. LN: 0.12 ppm p-p (about 300 nV at 2.5 V) with a 100 uF noise-reduction cap, no separate drift/hysteresis numbers published. Suffix `#TRPBF` = reel, `#TRMPBF` = 500-pc reel. Package MSOP-8 (300 C/W) or LS8 5x5 mm LCC (120 C/W). Voltage options include 2.048 V in MS8 only.

| Part | Initial acc. | Tempco max (typ) | Noise 0.1-10 Hz | LT drift | Hysteresis | Package | Supply / Iout | JLC / stock [V] |
|---|---|---|---|---|---|---|---|---|
| **LTC6655BHMS8-2.5** (current) | 0.025 % | **2 ppm/C** (1) | 0.25 ppm (625 nV) | 60 ppm/rtkHr | 20-30 ppm | MSOP-8 | up to 13.2 V; +/-5 mA | C107983 **out**, $13.90. DK 601 @ $19.04 (Octopart) |
| **LTC6655LNBHMS8-2.5** | 0.025 % | 2 (1) | **0.12 ppm** (~300 nV, with CNR 100 uF) | not published | not published | MSOP-8 | same | **C1236879, 52 in stock, $23.10.** DK 1,512 @ $19.04 [V]. Pinout/CNR network compat with non-LN: [U], check datasheet |
| **LTC6655CHMS8-2.5** | 0.05 % | **5** (2.5) | 0.25 ppm | 60 | 20-30 | MSOP-8 | same | C689441 / 13 stock @ $11.80. DK CT 5,867 @ $12.50 (TRPBF) [V]. JLC page for C689442 (TRPBF) = **Ext** [V] |
| **LTC6655BHLS8-2.5** | 0.025 % | 2 (1) | 0.25 ppm | **20 ppm/rtkHr** | 5/30/80 ppm | ceramic LCC 5x5 mm | same | not stock-checked [U]; footprint change; tray only |
| **LTC6655-2.048 (B)** | 0.025 % | 2 (1) | 0.25 ppm (512 nV at 2.048 V) | 60 | 20-30 | MSOP-8 | needs >=3 V | not stock-checked [U]. Cuts ADC FS by 18 %; only helps if the analog rail is low |
| **ADR4525 (A/B)** ([ADI](https://analog.com/adr4525)) | 0.02 % (B/C/D) | A 4 / **B 2** (-40 to 125); C 1, D 0.8 (0-70 C) | ~1 uVpp (~0.4 ppm; page conditions ambiguous [U]) | 8 ppm typ at 4,500 h | **not stated [U]** | SOIC-8 / LCC | 3-15 V; +/-10 mA | ADR4525ARZ C579306 / **Ext** [V] / 162 @ $3.90; ADR4525BRZ-R7 C403914 / **4,503 @ $5.60** [V] |
| **LT6657-2.5** ([ADI](https://www.analog.com/media/jp/technical-documentation/data-sheets/j6657f.pdf)) | 0.1 % | **A 1.5 / B 3** | **0.5 ppm** | 30 ppm/rtkHr | 20-40 ppm | MSOP-8 | to 40 V; +/-10 mA | C673441 (A) and C673451 (B): **both out of stock** [V]. DK [U] |
| **MAX6126 -2.5** ([ADI/Maxim](https://cdn.sparkfun.com/assets/5/2/0/f/d/MAX6126_Datasheet.pdf)) | 0.02 % (A, SO) | A 3 (-40 to 85) / 5 (-40 to 125) | 1.45 uVpp (~0.58 ppm) | 20 ppm/1000 h (SO), 100 (uMAX) | 35 ppm (SO), 80 (uMAX) | SO-8 / uMAX | +/-10 mA | MAX6126AASA25+ C1575568 **Ext** [V]; stock [U] |
| **REF5025** ([TI](https://ti.com/lit/gpn/REF5010)) | 0.05 % hi grade | 3 (hi) / 8 (A) | **3 uVpp/V = 7.5 uVpp (3 ppm)** | 22 ppm/1000 h (SOIC) | 50-70 ppm (SOIC) | SOIC-8 / VSSOP-8 | 2.7-18 V; +/-10 mA | REF5025AIDR C11341 **Ext** [V]; stock [U] |
| **REF6025** ([TI](https://ti.com/lit/gpn/ref6050)) | 0.05 % | 5 | 7.5 uVpp (3 ppm) | 80 ppm (0-1000 h) | 33 / 8 ppm | VSSOP-8 | 3-5.5 V; +/-4 mA | not checked [U] |

**Ranking (my judgment, against ~2 ppm resolution / ~4 ppm linearity)**
1. **LTC6655LNBHMS8-2.5 (or the LTC6655BHMS8-2.5 if you can get it).** Best noise (0.12 ppm), 2 ppm/C max, same MSOP-8 family. Verified stock today: DK 1,512 (USD), LCSC 52. Change: LN needs the CNR noise-reduction capacitor network; confirm pin function against the datasheet before swapping into a footprint [U].
2. **LTC6655CHMS8-2.5.** Same noise and drift, tempco max 5 ppm/C instead of 2, best stock (DK 5,867). Acceptable if the reference sits in a temperature-controlled zone and you compensate span against TMP117; otherwise the 5 ppm/C max is larger than the whole 4 ppm linearity budget for a 1 C swing.
3. **ADR4525BRZ.** Verified on JLC (Ext) with 4,503 in stock, the only second-source alternate that is stocked and has 2 ppm/C max. Noise ~0.4 ppm p-p per the ADI page and 8 ppm/4,500 h drift look good on paper, but hysteresis is not stated on the page I read and the noise condition is ambiguous. Change: SOIC-8 footprint, different pinout, no CNR network. Bench-qualify before committing.
4. **LT6657 / MAX6126.** Good parts, but LT6657 is out of stock at LCSC and MAX6126 stock is unverified. MAX6126 SO-8 at 1.45 uVpp is ~2.3x LTC6655 noise.
5. **REF5025 / REF6025: do not use** for this resolution. 3 ppm p-p noise is 12x LTC6655; REF6025 also has 5 ppm/C and large hysteresis.
Note on hysteresis: the plastic LTC6655 shifts 20-30 ppm after a temperature cycle. That is a span shift handled by the internal check-mass calibration, but it is larger than the linearity spec.

### 3C. VPG Y160610R0000E9W (10 ohm Z-foil Kelvin shunt)

**Decode of the part number** [V, VCS1625ZP datasheet]: Y | 1606 (= VCS1625ZP) | 10R0000 (10 ohm) | E (+/-0.2 %) | 9 (Pb-free) | W (waffle pack). `R` as last character = tape and reel. Datasheet: 4 pads for Kelvin, 6.35 x 4.06 x 1.02 mm, 1 W at 70 C, TCR **+/-0.05 ppm/C typ (0-60 C)**, +/-0.2 ppm/C typ (-55 to 125 C), max spread +/-2.8 ppm/C, **PCR 5 ppm at rated power**, load life 150 ppm typ / 250 ppm max (2,000 h, 70 C, 1 W), tolerances +/-0.2/0.5/1 % standard.

**Tolerance does not matter for this job.** The span calibration absorbs the absolute value, so D (0.5 %) or F (1 %) grades carry no metrology penalty. They matter only for availability. DK pages for **D9R** and **D9W** show a 39-week lead time, "not kept in stock", with a qty-50 price tier (GBP 24.45 on the D9R page; E9R GBP 21.62, qty 0) [V]. **So the same-die options do not solve the lead time.**

| Option | Terminals / size | TCR typ (max) | Power coeff. | Tolerance (10 ohm) | Long-term stability | Availability today | Verdict |
|---|---|---|---|---|---|---|---|
| **VCS1625ZP** Y160610R0000E9W (current) | 4-pad, 6.35x4.06 mm, 1 W | 0.2 (2.8) ppm/C | 5 ppm at 1 W | 0.2 % | 150 ppm typ / 2,000 h | 0 stock, 39 wk, MOQ 50 | keeps ppm-level stability, unobtainable now |
| **VCS1625Z** Y160710R0000D9W ([DK](https://www.digikey.co.uk/en/products/detail/vpg-foil-resistors/Y160710R0000D9W/4229709)) | same 4-pad footprint, 0.5 W | 0.2 ppm/C typ [V on DK] | 5 ppm at rated power per VPG title [U] | 0.5 % | 150 ppm class [U] | Active, 39 wk, tray; **stock not read [U]** | **Best drop-in** if any stock exists. 0.5 W is plenty for 36 mW. |
| **VCS1625P** Y085610R0000F9W ([DK.it](https://www.digikey.it/en/products/detail/vpg-foil-resistors/Y085610R0000F9W/4225126)) | same 4-pad footprint, 1 W | **2 (5)** ppm/C | not specified | 1 % | 0.015 % typ (150 ppm) | **59 in stock** (EUR page), restock 39 wk | In-stock fallback, **keeps footprint but compromises TCR 10x** |
| **VCS1625** (non-Z/P) | same footprint, 0.5 W | 2 (5) ppm/C | not specified (0.1 ppm/V voltage coeff.) | up to 0.1 % | 0.02 % load life | [U] | same compromise as P |
| **VCS1610Z** ([Mouser DS](https://www.mouser.com/datasheet/2/428/vcs1610z-541393.pdf)) | 4-pad, 4.06x2.54 mm, 0.25 W | 0.2 (4.8) ppm/C | not given | 0.5 / 1 % | 150 ppm typ | [U] | Z-foil, smaller footprint. 36 mW is fine at 0.25 W. |
| **VSMP Z-foil** (0805-2512) ([Mouser DS](https://www.mouser.com/datasheet/2/428/VSMP-3247725.pdf)) | **2-terminal** wraparound, 750 mW at 2512 | 0.2 (3.8, for 10 to <50 ohm) | 5 ppm at rated power | 0.25 % (10 to <25 ohm) | 50 ppm typ / 100 max (2,000 h) | RS lists 1206 10 ohm; stock [U] | Keeps ppm-level stability. Needs a Kelvin land pattern on a 2-terminal part. At 10 ohm, 1 mohm of pad resistance is 100 ppm and moves with copper tempco (~0.4 ppm/C), so route sense taps at the inner pad edge. Footprint change. |
| **VFCP Z-foil** flip chip ([xdevs DS](https://xdevs.com/doc/VPG/VFCP.pdf)) | terminal count not stated | 0.2 (up to 7.8 below 10 ohm) | 5 ppm at rated | to 0.01 % (value-dependent) | 50 ppm typ / 100 max | [U] | Same family idea as VSMP; 5 ohm minimum puts 10 ohm at the low end of the range. |
| **Vishay Sfernice P-series thin film** 0603-1206 ([Mouser DS](https://www.mouser.com/datasheet/2/427/p-240034.pdf)) | 2-terminal | **5 ppm/C (C or Z code)** | not given | 0.1-5 % at 10 to <50 ohm | **0.02 % typ (200 ppm) / 8,000 h** | widely stocked family [U] | Compromises stability. Probably adequate for EVT, not for the 4 ppm class without calibration and temperature compensation. |
| **Riedon PCS** ([datasheet](https://riedon.com/media/pdf/PCS.pdf)) | 4-terminal, 3 W, aluminum substrate | 10 standard, 2 on request | not given | 0.1-5 % | **0.1-0.5 % / 2,000 h** | [U] | **Not suitable**: stability is 10-30x worse. |
| **Ohmite FC4T/FC4L** ([datasheet](https://www.mouser.hk/ds/2/303/res_fc4t-1220659.pdf)) | 4-terminal | 50 (100 at 5 mohm) | - | - | - | - | **Not applicable**: 5-100 mohm range only. |
| **Isabellenhutte** | - | - | - | - | - | - | The BVB/BVN parts that came up in search are milliohm shunts. I found no 10 ohm part [U]. Not applicable. |

**Self-heating check (my estimate, assumption-labelled).** Shunt power at 60 mA is 36 mW. For Z-foil, PCR is 5 ppm at 1 W, so 36 mW contributes about 0.2 ppm. For VCS1625P (2 ppm/C), assume 50-100 C/W board thermal resistance (assumed, not from a datasheet): rise 1.8-3.6 C, so about 4-7 ppm of full-scale non-linearity. That is as large as the +/-4 ppm linearity spec. **Therefore: VCS1625P is an EVT stand-in and should not be treated as an AD520-class final part unless you add shunt temperature compensation from a TMP117 on the shunt.**

**Ranking.**
1. VCS1625Z (Y1607...) or VCS1625ZP in D/F tolerance: ppm-level, same footprint. Check stock on digikey.com / Mouser / Avnet before assuming the 39 weeks.
2. VCS1610Z (smaller footprint) or VSMP/VFCP with a Kelvin land pattern: ppm-level, footprint change.
3. **VCS1625P** in stock now: only compromise that keeps the footprint; pair it with a temperature sensor.
4. Thin film (Sfernice P): compromises stability; EVT only.
5. Riedon PCS, Ohmite FC4T, Isabellenhutte: not applicable.

## 4. RS-232 transceiver options (3.3 V, MAX3232-class) [added line 26]

Common to the MAX3232 family [V, TI pages]: 3-5.5 V VCC, up to 250 kbit/s, 2 drivers/2 receivers, four 0.1 uF charge-pump capacitors, +/-15 kV HBM and +/-8 kV contact IEC 61000-4-2 on the bus pins (MAX3232E page), 16-pin packages. TI states MAX3232 is Active and recommends **MAX3232E** for new designs ("improved direct drop-in") [V].

| Option | Package | JLC type | LCSC # | LCSC stock / price | Notes |
|---|---|---|---|---|---|
| **MAX3232CDR** (TI) | SOIC-16 (D, 150 mil) | **Ext** [V] | C45843 | **17,684** @ $0.46 [V] | Cheapest and best stocked. C grade = 0-70 C [U on suffix temperature]. |
| **MAX3232IPWR** (TI) | TSSOP-16 | **Ext** [V] | C113205 | **28,367** @ from $0.22 [V] | -40 to 85 C. Smallest footprint. Likely the best pick. |
| **MAX3232ECPWR / EIPWR / EIDR** (TI, E = enhanced) | TSSOP-16 / SOIC-16 | ECPWR **Ext** [V] | C11317 (ECPWR), C406863 (EIPWR), C193666 (EIDR) | stock [U] | TI's recommended new-design variant. |
| **SP3232EBCY-L/TR** (MaxLinear/Exar) | SOIC-16 | page type not read | C365761 | stock [U] | Second source, not TI. |
| MAX3232CDBR (TI) | SSOP-16 | **Ext** [V] | C7257 | stock [U] | Alternative footprint. |

Recommendation: **MAX3232IPWR (C113205)** for a mass-produced build, or **MAX3232ECPWR (C11317)** if you want TI's recommended-for-new-design E variant, both TSSOP-16. They share the same pinout as the SOIC-16 parts, so schematic is unchanged; only the footprint differs. Add four 0.1 uF caps and a DB9 connector (not in BOM).

## 5. JLC-Basic-friendly substitutes (task 3)

Only **one** Basic part turned up that is a plausible equivalent for this BOM. I checked JLC pages for ESD, USB-C and the other converters; none of those were Basic.

| BOM line | Substitute | JLC type [V] | LCSC | What changes | Performance impact |
|---|---|---|---|---|---|
| U_BUCK5V1 TPS62135RGXT (4 A sync buck) | **TPS5430DDAR** (TI): 5.5-36 V in, 3 A, 500 kHz fixed, Active ([TI](https://www.ti.com/product/tps5430)); TI also points to LMR33630 as the newer replacement | **Basic** (C9864) [V] | C9864, **188,897 in stock** @ $0.36 [V] | Non-synchronous: add a Schottky catch diode and a new inductor; different footprint (SO-8 PowerPAD); different FB divider; no firmware change. Fits only if the 5 V rail is <=3 A. Check your 5 V load first. | Lower efficiency (diode loss). 500 kHz ripple is fine ahead of the LT3045. Not for the metrology rail. |
| U_BUCK3V3_1 TPS62172DSGT (3.3 V, 500 mA buck) | **AMS1117-3.3**, 1 A LDO, SOT-223 | **Basic** (C6186) [V] | C6186 | Replace with linear LDO: footprint, input/output caps (check ESR needs), no inductor. Needs ~4.4 V input (1.1 V dropout at 800 mA, per page). | Loses efficiency (heat = (5-3.3) V x load). Quiet (no switching). Fine for MCU logic at <=300 mA. |
| U_ESD1 TPD2EUSB30ADRTR | USBLC6-2SC6 (ST) | **Ext** [V] (C7519) | C7519 | Footprint SOT-23-6 | No Basic ESD found. Keep the genuine TI C94934, no gain from swapping. |
| J_USB1 USB4105-GF-A | TYPE-C-31-M-12 (C165948) | **Ext** [V] | C165948 | Footprint | No Basic USB-C found, so no gain. USB4105-GF-A is already stocked (8,815) on JLC. |
| TPS61088 boost, TPS2121 mux, TPS259470 eFuse, BQ25792, TCAN1044A, THVD2450, TMC2209 | none | - | - | - | No Basic equivalent found; all are Extended. |

## 6. Parts that need hand-sourcing (not available at JLC today)

1. **OPA593DNTT/DNTR**: LCSC C7430247 is out of stock (DK 250 due May 2027).
2. **LTC6655BHMS8-2.5#PBF**: LCSC C107983 out of stock. Alternates at LCSC are only 13 (C grade) or 52 (LN-B) pieces, effectively hand-sourced from DK.
3. **VPG shunt Y160610R0000E9W** (and D9W/E9R siblings): not found at LCSC. Hand-source or order direct from VPG.
4. **OPR2100** photodiode array: not found at LCSC.
5. **TMUX1101DBVR**: only a clone at LCSC; hand-source genuine TI.
6. **TSAL6200** (5 mm THT): LCSC lists it (C55528, 3,790) but through-hole is a manual step [U]; hand-solder.
7. **IRM-30-12, IRM-20-5**: owned; through-hole encapsulated modules are in the segregated mains assembly, hand-installed. (LCSC has both: C193294, C6280121.)
8. **Not at JLC / not stock-verified, from the "TBD and Notes" sheet**: Susumu RM3216 matched network, precision 33.2 k TIA resistors, voice coil and magnet, mains inlet/fuse/switch, E-stop, safety relay, pluggable terminal blocks, NEMA17 motors. None researched.
9. **At risk of becoming hand-sourced**: STM32G491RET6 (DK 0, LCSC 53), TMC2209-LA (LCSC 21), TPS62135RGXR (LCSC 114), BQ25792 (DK "no longer carried" note).
10. **PCBWay**: not determinable from public pages for any line [U].

## 7. Items to verify before buying (all `[U]` above)

- Digi-Key stock on digikey.com (not regional sites) for lines 1, 8, 15-19, 22.
- Mouser stock/price for every line (not retrievable).
- JLC library type for lines 1, 4, 10, 14-17, 19-20, 22, 24.
- LTC6655LN pin compatibility with the existing MSOP-8 footprint and its CNR capacitor requirement.
- ADR4525 hysteresis and noise conditions (datasheet Rev G).
- OPA564 offset/noise (datasheet tables not retrievable); OPA192/BUF634A electrical tables.
- VCS1625Z actual stock (DK page showed no quantity).
- Coil compliance voltage and current limit, which decide the driver choice.
