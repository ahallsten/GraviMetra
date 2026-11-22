# Used and new 1 mg balance search: AD520-class for an automatic powder trickler

Search date: 2026-11-21. Buyer is in Wyoming. Prices are in USD, before Wyoming sales tax, which eBay collects at checkout.

## Reference: the target spec

**Torbal AD520 (new):** 520 g × 0.001 g, repeatability 0.001 g, linearity ±0.002 g, stabilization 3 s, RS232 + USB. San Diego Scale's sale price is $1,004.61 ([source](https://sandiegoscale.com/products/torbal-ad520-lcd-precision-balance-w-rs232-usb-clock-backlit-520-g-capacity-g-readability)). A Torbal reseller says "All Torbal laboratory scales operate with high quality electromagnetic load cells" ([labsup.net](https://labsup.net/products/ad-precision-series-balances)). That is reseller text, so treat the sensor type as **unverified**.

## Method and limits (read first)

- **eBay:** search, sold and completed pages are blocked (`robots_disallowed`). I could only open item pages, product (`/p/`) pages and browse (`/b/`) pages, and only when a web search had surfaced them first. A few item pages would not open; those rows say "details unverified".
  - eBay's delivery estimates were quoted to a default ZIP (94104 or 43230), not Wyoming. Free shipping is still free. Paid shipping to WY may differ a little.
- **Sold prices:** these come from individual ended eBay listings and from "SOLD" threads on AccurateShooter. True eBay sold-price history was not reachable.
- **Other sites:**
  - Sniper's Hide threads, Machinio and Picclick returned fetch errors.
  - The one LabX hit (PB303-S) was inactive.
  - EquipNet auctions (an XP603S and an Ohaus PX523/E) need a login to see bids.
  - No GovDeals hits came up for 1 mg balances.
  - I did not try r/Reloadingsales directly, and no relevant hits came up for it in search.
  - **Facebook Marketplace was not searched**, as instructed.
- **Sensor type:** I verified it from manufacturer literature at the series level where marked. "Unverified" means I found no manufacturer statement.

## 1. Best current listings (best value first)

| # | Model | Sensor (verified?) | Cap × d | Repeatability / linearity (mfr) | Interface and streaming | Price + ship | Condition / included | Seller / location | URL | Red flags |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | **A&D GF-200P** (pharmacy GF) | SHS, magnetic force restoration + parallelogram (GX/GF data sheet: **verified, series level**) | 210 g × 1 mg | 0.001 g SD / ±0.002 g (GF-200); about 1 s on FAST | RS-232C, DB-25, A&D format with ST/US header. Stream mode `prt 3`; display refresh 5 or 10 Hz (GF manual) | **$445 + $49.74 = $494.74** (Best Offer) | **New**. Pan, breeze break, AC adapter, WinCT disc. No cal weight | nowski (Bradley's Buys), 100%, about 1.9K sold. Palm Harbor FL. 30-day returns, seller pays | [ebay 168578837297](https://www.ebay.com/itm/168578837297) | NTEP Class II "P" model: the cal switch may be sealed (unverified). DB-25 connector. Last updated 2026-11-06 |
| 2 | **A&D FX-300iN** | SHS (FX-i): **verified** (A&D FZ-i/FX-i product page lists "Super Hybrid Sensor"). RS-232C standard; Quick USB is option FXi-02 | 320 g × 1 mg | 0.001 g / ±0.002 g; about 1 s FAST (FX-i manual) | RS-232C DE-9 (DCE), stream at 5/10/20 Hz. OpenTrickler-supported family (per your brief, not re-checked) | **$383.40 + $5.23 = $388.63** | Pre-owned, "W/AC Adapter" (title). Rest **unverified** (page would not open) | Unknown | [ebay 307264613926](https://www.ebay.com/itm/307264613926) | N = NTEP version (e = 0.01 g; cal may be sealed). Seller rating and returns not seen. Ask for photos and video |
| 3 | **Mettler PG203-S** | MonoBloc (EMFC). **Likely but not confirmed** for PG-S. Mettler states PB-S uses MonoBloc | 210 g × 1 mg | 0.0008 g / ±0.002 g; 1–2.5 s (PG-S manual) | RS232C 9-pin **built in**; MT-SICS (`SIR` stream) | **$179.99 + $42.41 = $222.40** (Best Offer) | Pre-owned, "Tested", **no power supply** | Unknown (page would not open) | [ebay 257833173628](https://www.ebay.com/itm/257833173628) | Needs a supply: the stock adapter is 12 VAC 1.25 A, and the balance also accepts 9–20 VDC 7 W (manual). Check the DC jack polarity. Draft shield not mentioned |
| 4 | **A&D FX-123** (Fortis, FX-120i successor) | C-SHS: **verified** (A&D FX/FZ data sheet) | 122 g × 1 mg | 0.001 g / ±0.002 g; about 1 s | RS-232C D-sub 9 **and** USB standard; refresh 5/10/20 Hz (data sheet; per-model mapping not confirmed) | **$539.97, free ship, import fees included** (Sushi-boy). Also $498 + $45 (137772867214), import fees not stated | New, Japan-market (JA) unit. Draft shield. Adapter included on the sushi-bomber listing | Sushi-boy, 99.6%, Japan | [ebay 920140969154](https://www.ebay.com/itm/920140969154) | Over the $500 target. Japan-domestic unit: check the adapter is rated 100–240 V, and ask about warranty and manual language. Some are "presale". sushi-bomber sold 7 at $480.99 + $95 |
| 5 | **Shimadzu UX420H** | UniBloc: **verified** (Shimadzu catalog) | 420 g × 1 mg | ≤0.001 g / ±0.002 g; 1.5–2.5 s (Shimadzu spec page) | RS-232C (or Shimadzu USB-serial kit); protocol in manual §15 | **$299 + $75 = $374** (Best Offer) | Pre-owned. Contents **unverified** | Unknown | [ebay 257322433372](https://www.ebay.com/itm/257322433372) | Contents and test status unknown. UX power is 12 V 1 A; the UW series is centre-negative. Discontinued series |
| 6 | **Mettler PG503-S DeltaRange** | MonoBloc (likely, as #3) | 510 g; **1 mg only in a 100 g fine range**, 10 mg above it | PG-S manual lists 1–2 s stabilization | RS232C built in; MT-SICS | **$264 + $50.17 = $314.17** | Used, "tested and found working". Photos show 2.004 g and 10.002 g readings. Draft shield, cord. Test weights not included | warenhandler, 100%, about 9.9K sold. Springfield IL | [ebay 198243861749](https://www.ebay.com/itm/198243861749) | **No returns**. "Basic testing only". DeltaRange fine range is OK for charges under 100 g. MT lists this model as phased out in 2001 (old) |
| 7 | **Ohaus Explorer Pro EP213C** | **Unverified** (no Ohaus statement found) | 210 g × 1 mg | 0.0005 g / ±0.002 g; 3 s (dealer spec table) | RS232 standard. Ohaus continuous-print (CP) mode | **$389.89, free ship** (Best Offer) | Pre-owned. Contents **unverified** | Unknown | [ebay 157695687751](https://www.ebay.com/itm/157695687751) | Sensor not confirmed. Older platform |
| 8 | **Mettler PB303-S** | MonoBloc: **verified** ("All PB-S … equipped with MonoBloc", MT brochure) | 310 g × 1 mg | 0.001 g / ±0.002 g; 1.5–2.0 s (AB-S/PB-S manual) | **RS232C is an optional module** (MT 11103678). MT-SICS if fitted | **$89.97 + $19.95 = $109.92** (Best Offer) | Used, "pulled from working environment", tested. **No power adapter** | tvrsales2, 99.4%, about 79K sold. Stockton CA | [ebay 235707351510](https://www.ebay.com/itm/235707351510) | **No returns.** Confirm the 9-pin RS232 module is fitted (rear photo) or budget for the option. Supply: 12 VAC, or 9.5–20 VDC 6 W |
| 9 | **A&D FX-300** (old, VFD, not FX-i) | **Unverified** (old FX manual does not say) | 310 g × 1 mg | 0.001 g / ±0.002 g; about 2.5 s (FX/FY manual) | **RS-232C needs the OP-03 board** (HDB-25). Stream about 5 Hz at ≥1200 bps; ST/US/OL header | **$350 + $50 = $400** (Best Offer) | Used, "good working condition". Glass draft shield. Adapter not confirmed | fedtech804, 100%, about 6.9K sold. Chico TX. 30-day returns, buyer pays | [ebay 145693936821](https://www.ebay.com/itm/145693936821) | OP-03 likely absent. Slower. Old. Not a good buy unless the OP-03 is shown |
| 10 | **Mettler XS603S** | MonoBloc (XS Excellence; **unverified** here) | 610 g × 1 mg | Not fetched | RS232 standard on XS; MT-SICS | **$605** (shipping not shown); $649.99 + $5.55 (137839125913) | Pre-owned. Details **unverified** | Unknown | [ebay 206305489600](https://www.ebay.com/itm/206305489600) | Over budget. Listing pages not opened. Big, high-performance unit |
| 11 | **Mettler PB303-S/FACT** | MonoBloc: **verified** (PB-S) | 310 g × 1 mg | As #8 | RS232 optional, **must confirm** | **$599, free ship** | Used, "very clean", cover fitted all its life. Adapter in photos | mikep777, 100%, about 16K sold. Northborough MA. **60-day returns, seller pays** | [ebay 366669982835](https://www.ebay.com/itm/366669982835) | Overpriced next to #8. RS232 fitment unknown |
| 12 | **Sartorius Entris 323-1S** | Sartorius says the series has a "monolithic weigh cell" (**verified, series level**) | 320 g × 1 mg | 1 mg / 2 mg; 1.1 s (Entris data sheet) | RS232C **25-pin** standard; SBI protocol; YCC01-USBM2 USB cable option | **$599.99, free ship** (Best Offer) | Used, "good working condition". Adapter in photos. Pan and shield not stated | Davliegogo LLC, 99.8%, about 42K sold. Baldwin Park CA. **60-day returns, seller pays** | [ebay 286694294277](https://www.ebay.com/itm/286694294277) | Over budget. Make an offer. Last updated 2026-01-07 (stale) |
| 13 | **A&D FX-300i** (local) | SHS: verified | 320 g × 1 mg | 0.001 g / ±0.002 g | As #2 | **$550** | "Like new". Box, foam, pan, AC adapter, manual, **plus an 8-piece 10–500 g cal set** | Craigslist, Portland OR. Local pickup preferred | [craigslist](https://www.craigslist.org/view/d/portland-ad-fx-300i-precision-lab/jF71c6HrbCMWCacrAuef7s) | About 900+ mi from WY. Ask the seller to ship. Posted 2026-06-21, updated 2026-11-01 |
| 14 | **U.S. Solid USS-DBS74-6** | Seller **claims** electromagnetic force sensor (**unverified**; no OEM literature) | 600 g × 1 mg | ±0.001 g / ±0.001 g claimed; ≤3 s | RS232 + USB. Stream rate and data format not published | **$524** (new, direct) | New. 500 g cal weight, USB cable, pan | ussolid.com | [ussolid](https://ussolid.com/products/u-s-solid-precision-balance-0-001-g-x-600-g-1-mg-lab-analytical-scale-for-science) | Claimed specs are better than the A&D or Mettler units here, which is implausible at this price. The protocol is undocumented. Budget baseline only |
| 15 | **A&D FX-120i-EX** | SHS (FX-i) | 122 g × 1 mg | 0.001 g / ±0.002 g | RS-232C DE-9; stream up to 20 Hz | **$559.99 + $24.99 = $584.98** | New, open box. Instructions, warranty card, accessories | Lauries Store, 99.6%. Henrico VA. 30-day returns, seller pays | [ebay 298737182996](https://www.ebay.com/itm/298737182996) | The "-EX" suffix is unexplained, and origin is listed as China. Ask what "-EX" means |

**Excluded or rejected:**

- **A&D EJ-303 / EJ-123** ($299 new, MNMscales). Load cell, per A&D's EJ launch article (the-scientist.com). Repeatability 0.003 g and linearity ±0.003 g, with RS-232C as an option. **Fails the spec.**
- **Shimadzu TXC323L** ($275.99 + $25.55). This is a carat model: 320 ct (64 g) × 0.001 ct. **Fails capacity.**
- **U.S. Solid 500 g × 1 mg at $184.99.** Strain gauge, ±0.002 g. Fails.
- **U.S. Solid USS-DBS89-500G at $467.** EMF claimed, but repeatability is only ±0.002 g. Marginal.
- **Shimadzu UW220H** ($194.82 + $50 + import fees, Thailand). "Without calibration", vague listing. Ask for a test video before considering it.
- **A&D FX-300 "fair"** ($311.99 + $39.99, [225130658745](https://www.ebay.com/itm/225130658745)). Rusted door, no charger.
- **New A&D listings I saw over $650:** FZ-300i at $879, FZ-120i at $889, FX-300i at $780–799, GX-200 at $1,398.

## 2. Typical used and new price per model

| Model | Used or sold evidence | New (eBay or dealer) |
|---|---|---|
| A&D FX-120i | AccurateShooter classifieds: two threads marked **SOLD**, each asking **$400** (one $400 shipped, with a V1 AutoTrickler and 50 g check weight; one "everything that came with it"). Dates not shown. One active ask of $675 with V1 trickler + 100 g weight. eBay browse card showed "used $238" (unverified, probably a stale listing). **Typical: about $400–450** | $560–$910 (9 listings on [ebay /p/670809120](https://www.ebay.com/p/670809120)) |
| A&D FX-123 | No used listings seen | $480–$600 + $45–$143 shipping from Japan; sushi-bomber sold 7 at $480.99 + $95 ([ebay /p/6092314426](https://www.ebay.com/p/6092314426)) |
| A&D FX-300i / FX-300iN | Active: $388.63 (iN), $550 local with cal kit. eBay card "used $225" (unverified) | $780–$799 |
| A&D GF-200/300 (old GF) | GF-300 used at $199.99 + $15.46 ([198224358193](https://www.ebay.com/itm/198224358193)), ended unsold on Aug 11. Scales Plus used GF-300P at $660 with a 90-day warranty | GF-200P $445 (new old stock, #1) |
| A&D FX-200/FX-300 (old, non-i) | $350–$531 asks ([152978480587](https://www.ebay.com/itm/152978480587) at $470 + $71.89, last updated 2023; FX-200 at $531.25 + $54) | n/a |
| Mettler PB303-S | $90–$250 for as-is or no-PSU units (parts unit $149 + $18.80; a "works great, no right door" unit at $248 ended Aug 2026); $599 clean; LabX $650 (inactive) | n/a (discontinued) |
| Mettler PG203-S / PG503-S / PG603-S | **PG603-S SOLD for $363.75, free ship, on 2025-08-29** ([356788866705](https://www.ebay.com/itm/356788866705)). PG203-S $180; PG503-S DR $264 | n/a |
| Mettler XS603S | Asks $575–$650 used; $1,100 for an XS603S DeltaRange; $699 + $119 + import (Israel) | n/a |
| Sartorius Entris 323-1S | $599.99 used | Entris II BCE323 about $1,344 |
| Shimadzu UX420H / UW | UX420H $299 + $75; UW220H about $195 + $50 + import; UW620H $1,200 + $300 (outlier) | n/a (discontinued) |
| Ohaus Explorer Pro EP213/EP413 | EP213C $389.89; EP413 $500; EP413 parts-only $99.99; EP613C $649 + $67.70 | n/a |

## 3. Top 3 recommendations

1. **A&D GF-200P, new, $494.74 delivered (before tax)** ([168578837297](https://www.ebay.com/itm/168578837297)).
   - New, with 30-day seller-paid returns, so there is no used-EMFC risk.
   - It has the A&D SHS sensor and the fully documented A&D protocol: ST/US header, `prt 3` stream mode, `SIR`/`Q`/`Z`/`T` commands. That is the same protocol family OpenTrickler and AutoTrickler talk to.
   - Specs: 1 s stabilization, 0.001 g SD, ±0.002 g linearity.
   - Make an offer (Best Offer is enabled).
   - Downsides: DB-25 connector; the NTEP pharmacy variant (ask whether the CAL switch is sealed); 210 g capacity.
2. **A&D FX-300iN, used, $388.63** ([307264613926](https://www.ebay.com/itm/307264613926)).
   - It is the same platform as the FX-120i the reloading world uses, with 320 g capacity, a DE-9 port and 20 Hz streaming.
   - It is the cheapest real FX-i I found.
   - I could not open the page, so get the seller rating, a returns policy, a weighing video and a photo of the adapter label before buying.
   - The "N" (NTEP) unit may have a sealed cal switch. Ask.
3. **Mettler PG203-S, $222.40 + about $15 for a 12 V supply** ([257833173628](https://www.ebay.com/itm/257833173628)).
   - The cheapest true lab EMFC here (0.0008 g repeatability), with built-in RS232 and the publicly documented MT-SICS protocol (`SIR` stream, stable/dynamic flag).
   - The balance accepts 9–20 VDC, so a Mean Well 12 V module you already own would do.
   - Good as a test-rig or benchmark scale. Stabilization is slower (1–2.5 s).
   - Confirm the seller's returns policy (the page was not readable).

**Stretch pick:** a **new A&D FX-123 at about $540–575 delivered**, if you will go over $500. It has the newest C-SHS sensor, RS-232C plus USB as standard, and about 1 s stabilization. Buy a listing that says "import fees included", and confirm the adapter input rating.

## 4. Used EMFC balance buying checklist

1. **Video proof:** power on, let it warm up 30+ min if possible, tare, place a known mass (ideally 20/50/100 g class F1/M1), and show the stable reading. Remove the mass, show it return to 0.000, then repeat 5 times on camera.
2. **Corner load:** ask for the same mass at the four pan quadrants (corner-load error shows a bent flexure or crooked pan support).
3. **Shipping lock / transport screw:** many lab balances (notably Mettler XS/XP and Sartorius) have a transport lock or need the pan support removed before shipping. Ask how it will be packed. Insist the pan, pan support and shield come off and are packed separately, and the unit is double-boxed with foam.
4. **Parts present:** pan, pan support/spider (often lost), draft shield or breeze break with all doors (look for the missing right door, as on the PB303-S), and the leveling feet with their bubble.
5. **Power adapter:** model, output voltage, AC vs DC, current and polarity.
   - A&D FX-i/GF: 12 V DC adapter (A&D TB-series).
   - Mettler PB-S and PG-S: 12 VAC adapter. The balance also takes 9–20 VDC; check the jack polarity.
   - Shimadzu UW: 10–15.5 VDC, **centre-negative**.
   - Japan-market units: check the adapter is rated for 120 V, not 100 V only.
6. **RS-232 actually present and working:**
   - Old A&D FX needs the OP-03 board.
   - Mettler PB-S/AB-S needs the optional RS232 module.
   - Ask for a rear-panel photo, and ideally a screenshot of a terminal receiving data.
7. **Cal weight:** ask whether one is included and its class. A&D FX-i external cal uses 100 g (FX-120i) or 200 g (FX-200i/300i); confirm the value for your model in the manual.
8. **Overload or drop damage:**
   - Zero drift that won't settle, readings that wander by several mg, or slow return to zero.
   - Dents on the housing corners, a bent pan support, cracked feet.
   - On A&D, an `E` or `-E` display at no load.
9. **Error codes:** ask the seller to power-cycle and report any error code. A&D: `-E`/`E` (out of zero range or overload), `CAL E`. Mettler: `Err`, `L`/`H` messages.
10. **Firmware lock / legal-for-trade:** on N/P/NTEP models the CAL switch may be sealed and some menu items locked. Ohaus disables CP (continuous print) when LFT is ON.
11. **Returns policy:** prefer 30+ day returns or eBay Money Back Guarantee ("not as described"). Many cheap units are "no returns" (#6, #8), so price in that risk.
12. **Environment on arrival:** let it sit 24 h at room temperature before judging it (Wyoming winter shipping means a cold box, and condensation risk).

## 5. RS-232 protocol documentation and cabling

- **A&D (best documented):**
  - **FX-i/FZ-i** ([manual](https://www.ricelake.com/media/3medkdkg/m_a-d_fxi_precision_series_op-1.pdf)):
    - Connector and wiring: D-Sub 9 female, DCE, straight-through cable to a PC. Pins: 2 TXD out, 3 RXD in, 5 SG, 6 DSR, 7 RTS, 8 CTS.
    - Serial settings: 600–19200 bps; 7E/7O or 8N, 1 stop bit.
    - Stream mode: `dout prt 3`, display refresh `5pd` = 5/10/20 Hz (use ≥4800 bps for 20 Hz).
    - Commands: Q, SI, S, SIR, C, Z, R, T, PT, CAL, ON/OFF, ?ID, ?SN.
  - **A&D standard format** ([GX-A/GF-A comm manual](https://aandd.jp/products/manual/balances/gx-a_gf-a_communicationmanual.pdf)):
    - 15 characters + CR LF.
    - Header ST = stable, US = unstable, QT = counting, OL = overload.
    - Example: `ST,+00123.45  g<CR><LF>`.
  - **GX/GF (old):** RS-232C on a **DB-25** (pins 2 RXD in, 3 TXD out, 7 SG, 1 FG, plus 18 PRINT / 19 RE-ZERO contact inputs) per the [GF-P manual](https://www.ricelake.com/media/t2ylvwh3/m_a-d_gfp_op-1.pdf). Same A&D format; stream `prt 3`; refresh 5 or 10 Hz. You need a DB-25 to DE-9 serial cable or adapter.
  - **Old FX/FY (non-i):** optional OP-03 board, HDB-25. 600–4800 bps, 7E1. 17-character format with ST/US/OL. Stream about 5 Hz at ≥1200 bps.
  - **FX/FZ Fortis (FX-123):** D-Sub 9 + USB standard.
- **Mettler Toledo:** MT-SICS is publicly documented (separate "MT-SICS Reference Manual", English).
  - PB-S/AB-S: RS232C is an **option** (11103678). Cables RS9–RS9 / RS9–RS25. 600–19200 bd, host default 9600. Send modes "S.Stb" / "S.Cont". Formats SICS or PM.
  - PG-S: RS232C 9-pin **built in**.
  - Streaming: `SIR` streams. My understanding is that responses carry `S S` (stable) / `S D` (dynamic); **confirm in the MT-SICS manual** (not fetched).
  - Cable: straight DE-9 for PG-S. Pinout not given in the fetched manuals.
- **Sartorius Entris (I):** RS232C **25-pin** standard; optional YCC01-USBM2 RS232-to-USB cable. Uses the Sartorius SBI protocol, which Sartorius publishes in its manuals. The unstable-flag format is **unverified** here.
- **Shimadzu UW/UX:** RS-232C. Protocol, commands and format are in the instruction manual (§15, Appendix A-4), but that content was not readable in the copy I fetched. Shimadzu sells an RS-232C cable and a "USB–Serial Replacement Kit". Get the cable pinout from the manual before wiring.
- **Ohaus Explorer (EX):**
  - DB9 pins: 2 TxD, 3 RxD, 5 GND, 7 CTS, 8 RTS.
  - Commands: `CP` continuous print, `SP` print on stable, `xP` interval print. CP is blocked when Legal for Trade is ON.
  - That pinout is from the Explorer EX service manual ([pennscale PDF](https://www.pennscale.com/wp-content/uploads/2014/01/Explorer-Manual.pdf)). **Explorer Pro (EP) is assumed similar but unverified.**
- **U.S. Solid:** RS232 (and USB on the 600 g model). "Continuous print" is claimed, but the stream rate and format are not published online. **Undocumented.**
- **Torbal AD520:** bidirectional RS232 + USB-B. Protocol not checked.

**For an MCU-based trickler:**

- All of these use RS-232 voltage levels, so put a MAX3232 (or similar) between the balance and a 3.3 V MCU.
- The A&D units are DCE: wire balance TXD to the MCU's RX through the level shifter.
- Tie RTS/CTS as the manual requires, or set handshake off. On A&D FX-i, CTS/DSR are outputs.
