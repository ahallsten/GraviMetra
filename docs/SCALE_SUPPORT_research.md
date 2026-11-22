# Precision balance data interfaces for a DIY powder-trickler controller

Research date: 2026-11-21. Method: WebSearch/WebFetch only. Page numbers are as reported by the fetch tool's text extraction of the manufacturer PDF; tables were often garbled, so treat per-cell values as "verify on the printed page" where flagged.

Legend: **[V]** verified in a manufacturer/official document. **[R]** retailer text only. **[C]** community/OpenTrickler evidence only. **[?]** not found / could not verify. **[~]** garbled extraction or internally inconsistent source.

Important scoping notes:
- The Torbal AD520-specific PDF (`.../files/AD520_Manual.pdf`) could not be fetched (host path not on the tool allowlist). The Torbal **AD60 Manual** (https://cdn.shopify.com/s/files/1/0489/6990/8374/files/AD60_Manual.pdf) explicitly covers AD60/120/220/320/520/1200/2200/3200/6200 (p. 3), so the AD-series protocol below is from that manual. The AD520 per-model numbers (0.001 g / 0.001 g / +-0.002 g / 3 s / 15-30 C / external cal) are **[R]** from San Diego Scale; the AD-series manual p. 3 confirms 520 g, "< 3 s", +15 to +30 C, but its readability/repeatability/linearity rows are garbled and could not be mapped to the AD520 column.
- Mettler "MS-S" 1 mg is an older line (MS303S); the current 1 mg line is MS-TS (MS303TS). I could not fetch MS-TS specs.
- No model's weighing-sensor type is stated for Torbal, G&G (only "industrial-class high precision sensors" for JJ-B), Sartorius Entris II, Mettler PB-S. See matrix.

---

## (a) Summary matrix

| Model | Sensor (source) | Cap x d | Repeat / Lin | Max stream rate | Stable flag in frame? | RS-232 connector / pinout / null-modem | USB | Commands | Doc URL |
|---|---|---|---|---|---|---|---|---|---|
| Torbal AD520 (AD series) | Not found [?] | 520 g x 0.001 g [R]; 520 g cap [V p.3] | 0.001 g / +-0.002 g [R]; stab "<3 s" [V p.3]; 15-30 C [V p.3]; cal: external [R], manual also mentions internal auto cal [~] | "Cont" mode exists; rate **not stated** [?] | **No** (16-byte frame has no flag) [V p.32] | "RS232 Port Bidirectional - Standard" [V p.3]; connector/pinout not in text (cable diagrams WK-1/WD-1 are images) [?]; 4800/8N1 for LONG protocol [V p.32] | Optional USB-B, FTDI FT232R UART bridge (vendor-class, needs VCP driver, not CDC) [V p.3, 31-32] | `S I`, `S T`, `S Z`, `S S`, `S F`, `S L`, `S H` + CR LF [V p.32-33] | AD60 manual (above), p.3, 28-33 |
| A&D FX-120i / FX-300i (FZ-i differs only by internal mass) | C-SHS "Compact Super Hybrid Sensor" = spring material + electromagnetic balance element (A&D dev story) [V]; not stated in manual | 122 g / 320 g x 0.001 g [V p.73] | 0.001 g / 0.002 g [V p.73-74 table] | **20 / 10 / 5 per s** (menu `5pd`), >=4800 bd at 20/s [V p.43-44, 58] | **Yes** header ST / US / OL [V p.45-47] | D-Sub 9 male; pin 2 TXD(out), 3 RXD(in), 5 SG, 6 DSR(out), 7 RTS(in), 8 CTS(out); balance is **DCE -> straight cable to a PC (DTE)** [V p.58, 60] | Optional FXi-02 "Quick USB", one-way to PC; class not stated [V p.3, 59-61; product page] | Q/SI, S, SIR, C, T, Z/R, PRT, CAL, ?ID/?SN/?TN, ?PT [V p.62-66] | https://aandd.jp/products/manual/balances/fz_fx-i.pdf |
| G&G JJ223BF | Not found [?] | Not found [?] (JJ223BC-family, 220 g x 0.001 g assumed [?]) | Not found [?] | Not documented; OpenTrickler reads it at 9600 [C] | Not documented [?] | DE-9; pin 2/3/5 only roughly; DCE/DTE unclear [~] | Not found [?] | ESC(1BH)+p/q/r/s/t [V JJ-BC p.15 / sec.12.3] | https://www.manualslib.com/manual/3862769/GAndg-Jj223bc.html (JJ-BC manual; JJ223BF data p.4, RS232 p.14-15, not extractable) |
| G&G JJ100B | Manual: "industrial-class high precision sensors" (strain-gauge type implied; not EMFC) [V JJ-B p.1] | 110 g x 0.001 g listed [V p.1-2 ~] | Not stated [V] | Not documented | Not documented [?] | 9-pin socket; "2 RXD, 3 TXD, 5 GND" [~ p.4]; 8N1, 600 default (600-9600) [V] | None found | ESC+0x70 ("send data") only documented [V p.4] | https://www.manualslib.com/manual/1833431/GAndg-Jj100b.html |
| Sartorius Entris II 1 mg (BCE223/323/423/623/653, "-1S") | Not found [?] ("weigh cell" only) | 220-650 g x 1 mg [V p.47 TOC] | 0.5 mg (5% load) / 1 mg; lin +-2 mg limit, +-0.6 mg typ. [V]; settling <=1.0 s typ. [V]; env. 5-40 C, spec 10-30 C [V p.42]; internal (I models) or external cal [V] | **Not stated**; SBI auto-print interval 0-9999 s [V p.26]; "continuous" not documented [?] | **Not defined** in 16-char frame (stability only via print-mode setting) [V tech note p.2-3] | D-Sub 9; pin 2 TxD, 3 RxD, 5 GND, 7 CTS, 8 RTS, 9 "universal key"; default 9600, odd parity, 8 bits, 1 stop, HW handshake [V tech note p.5; manual p.24] | USB-C "USB UTL", class not stated [V p.25] | `Esc P`, `Esc T`, `Esc U/V`, `Esc f3_/f4_`, `Esc x1_`.. [V tech note p.4-5] | https://www.sartorius.hr/media/dypfvdsn/entris-ii-technical-note-en-sartorius.pdf ; https://documents.thermofisher.com/TFS-Assets/ANZ/manuals/sartorius-entris-II-essential-line-operating-instructions.pdf |
| Mettler PB303-S (PB-S) | Not found [?] | 310 g x 0.001 g [V p.20-21 / FR p.29] | 0.001 g / +-0.002 g; stab 2.0 s; 10-30 C spec, 5-40 C operable; external cal (non-certified) [V] | `S.Cont` send mode; rate **not stated** in OI [?] | PM format flag char (unexplained) or MT-SICS S/D status [V p.17, 31] | Optional RS232C, 9-pin socket; pinout unclear [~ p.22/30]; 600-19200, 7E/7N/8N/7O, handshake off/soft/hard [V p.18] | None | MT-SICS S, SI, SIR, SR, SU, Z, @ listed in OI; T per SICS manual p.181 [V] | https://manualmachine.com/mettlertoledo/ab54s/20840268-operating-instructions-manual ; SICS RM https://b2b.wagnermunz.com/download/Mettler-Toledo%20GmbH/RM_Advanced_and_Standard_Level_Balances_SICS_EN.pdf |
| Mettler MS303S (MS-S) | **MonoBloc** weighing cell [V datasheet] | 320 g x 0.001 g [V] | 0.001 g / 0.002 g; settling 1.5 s; FACT internal adjust [~] | **SIR + UPD: 0.1 to 22.9 values/s** (need baud >= 220 x rate) [V SICS RM p.193] | Yes: SIR status S/D [V RM p.153] | RS232C present; defaults 9600, 8N1, XON/XOFF, CR LF; 600-38400; pin 9 powers 2nd display (do not connect) [V BA p.36-39]; pinout/DCE not in extract [?] | USB device port, MT-SICS default; class not stated [V p.39-41] | MT-SICS (S, SI, SIR, T, Z, UPD...) [V RM] | https://wikis.mit.edu/confluence/download/attachments/83628254/MS-S_MS-L_BA_en_11781259E.pdf |
| Shimadzu UW/UX 220H/420H/620H (1 mg) | **UniBloc** [V manual p.1] | 220/420/620 g x 0.001 g [V p.5-6] | <=0.001 g / +-0.002 g (first column, mapping inferred); response 1.5-2.5 s; 5-40 C; UW internal motor weight, UX external [V] | **~80 ms per reading (12.5/s)** with D01 [V p.84] | Yes: "S"/"U" prefix [V p.81-82] | Balance side DB-25 numbering: 1 FG, 2 TXD, 3 RXD, 4 RTS, 5 CTS, 6 DSR, 7 SG, 20 DTR (A-4 p.105); manual's PC cable is wired as "null-modem" style [V p.80]; DCE/DTE unlabeled [?] | Not found | D01/D03 stream, D05/D07 single, TARE/T, PRINT, UNIT, Z-equivalents, plus Mettler/Sartorius emulation [V p.83-88] | https://www.marsden-weighing.co.uk/storage/files/downloads/UWX-User-Manual.pdf |
| U.S. Solid USS-DBS 600 g x 1 mg ("6003") | "Electromagnetic force compensation" [V manual sec.1, DBS62(63)] | 600 g x 1 mg [V] | +-1 to +-4 mg across 1 mg models, per-model mapping garbled [~]; stab 1-3 s [V] | Continuous at fixed 0.5/1/2/3 s intervals => **<=2 readings/s** [V] | Not documented [?] | DB-9; RXD pin 2, TXD pin 3, GND 5 "same on both ends" (ambiguous, likely null-modem) [~] | "Optional", no details [V] | **None documented** [V] | https://file.ussolid.com/content/JFDBS/Muanual%20-%20U.S.%20Solid%20USS-DBS62%2863%29%20Balance.pdf |

---

## (b) Per-model sections

### 1. Torbal AD520 / AD series
Source: Torbal AD60 manual (covers AD520), https://cdn.shopify.com/s/files/1/0489/6990/8374/files/AD60_Manual.pdf

- Specs vs. your checklist:
  - 520 g: confirmed in the AD-series table, p. 3 (tare range -520 g).
  - 0.001 g readability / 0.001 g repeatability / +-0.002 g linearity: **[R]** (San Diego Scale product page); manual rows are garbled across 9 models [~].
  - "<3 s": [V p.3]. 15-30 C: [V p.3].
  - Calibration: the checklist says "external only". The manual (p.3, ch.7.1) lists external calibration masses of 100/200/500 g for AD520 but also describes an internal automatic calibration. **Conflicts with "external only"; unresolved** [~].
- Send modes (Ports menu, p.30): Stab, NoStab, Auto, Cont, Remove, Clean. Cont = continuous without key or stability; no rate stated.
- Serial settings: baud/bits/parity are menu options (p.28-29). The LONG protocol is documented as 4800, 8 data, 1 stop, no parity (p.32). Default baud list not legible [?].
- Frame ("LONG", 16 bytes, p.32): byte1 sign or space; byte2 space; bytes 3-10 digits/decimal/space; byte11 space; bytes 12-13 unit letters (e.g. k+g, l+b, c+t, p+c, %); byte14 space; byte15 CR; byte16 LF. No stable flag and no overload code (overload shows "H" on the display only). **A stable flag must be derived in firmware.**
- Commands (p.32-33), all "S"+letter+CR LF: `S I` (print/weigh, replies 16-byte frame), `S T` tare (no reply), `S Z` zero (no reply), `S S` power, `S F` menu, `S L <val>` / `S H <val>` threshold set (up to 8 chars).
- Because tare/zero produce no reply, completion must be detected by watching the stream.
- Interface: RS232 "bidirectional - standard" (p.3, rear panel p.4). Connector type and pinout not given in text. Cable part "RS232 PC Cable" 301058 (p.61-62). USB-B uses FTDI FT232R (p.31-32): an STM32 USB host would need an FTDI vendor-class driver (not CDC-ACM). Use RS-232 instead.
- Known DIY support: none found for Torbal.

### 2. A&D FX-120i / FX-300i (and FZ-i)
Source: https://aandd.jp/products/manual/balances/fz_fx-i.pdf (one manual for FZ-i and FX-i)

- Specs p.73-74: FX-120i 122 g, FX-300i 320 g; d = 0.001 g; repeatability (SD) 0.001 g; linearity 0.002 g; stabilization approx. 1 s typical in FAST; 5-40 C; sensitivity drift 2 ppm/C. FX = external cal (100/50 g or 300/200/100/50 g); FZ = internal mass plus external. Only difference found: FZ has internal mass and EXC command is FZ-only.
- Sensor: manual does not state. A&D development story (https://aandd.jp/support/dev_stories/story3.html) says FZ-i/FX-i use the Compact Super Hybrid Sensor (C-SHS), a load-cell-style spring material combined with an electromagnetic balance element. This is not pure EMFC by A&D's own wording.
- RS-232C (p.58-60): D-Sub 9 male; asynchronous, half duplex, ASCII, 1 stop bit; pins as in matrix; balance is DCE so a straight cable to a PC. Baud 600-19200 (menu `bp5`); data 7E/7O/8N (`btpr`); terminator CR LF or CR (`Crlf`); timeout `t-Up` none or 1 s; `5if erCd` 1 enables AK/EC responses. RTS/CTS are on the connector but flow-control behavior is not described [?]. Whether a minimal 3-wire cable works was not found; community uses 3-wire [C].
- Output modes (`dout prt`, p.44): 0 key (stable), 1 auto print A, 2 auto print B, 3 **stream (continuous)**, 4 key B (immediate), 5 key C, 6 interval.
- Rate: `5pd` 5/10/20 per s (p.43). FAST response sets 20/s; MID/SLOW set 5/s (p.27). At 20/s use >=4800 bd. Data may be dropped if baud is too low (p.41, 44). Frame is 15 characters + terminator (17 bytes with CR LF), so 20/s needs about 3.4 kB/s... i.e. >= 9600 bd is safer; 19200 matches the OpenTrickler setting.
- Frame (A&D standard, p.45-47): 2-char header ST (stable) / US (unstable) / OL (overload), then sign, 6-digit value with decimal point, then 3-char unit. Example per p.47: `ST,+00001.72 g` + CR LF (spacing approximate due to garbled figure). Format variants DP, KF, MT, NU, CSV.
- Commands (p.62-66), terminator per `Crlf`: `Q` / `SI` immediate weight; `S` stable weight; `SIR` continuous request; `C` cancel S/SIR; `T` tare; `Z` or `R` re-zero; `PRT` same as PRINT; `CAL`; `?ID`, `?SN`, `?TN`; `PT:<n>` set preset tare, `?PT`. Control commands return `AK` (0x06) on receipt and again on completion when `erCd`=1; failures return `EC,Exx` (E00 protocol, E01 undefined, E02 cannot process now, E03 timeout, E04 excess chars, E06 format, E07 range; p.67).
- Unit change: not found as a command in the extracted portion [?] (units set via key/menu).
- Overload: `OL` header. Error: `EC,Exx`.
- USB: optional FXi-02, "one-way" transfer to PC, WinCT Plus, class not stated; **not usable as a host-readable device without testing** [?]. Ethernet FXi-08 optional.
- DIY support: OpenTrickler RP2040 controller README lists "A&D fx-i series (Std Format)" read + force zero at 19200 8N1 (https://github.com/eamars/OpenTrickler-RP2040-Controller#supported-scales); config guide PDF: https://github.com/eamars/OpenTrickler-RP2040-Controller/blob/main/manuals/OpenTrickler%20manual%20for%20ADFX%20scale.pdf (content not read). FZ-i is not listed by OpenTrickler [C].

### 3. G&G JJ223BF and JJ100B
Sources: JJ-BC manual https://www.manualslib.com/manual/3862769/GAndg-Jj223bc.html ; JJ-B manual https://www.manualslib.com/manual/1833431/GAndg-Jj100b.html ; mirror https://manuals.online/gandg/scale/jj-bc-series/user-manual/1lt69. Some mirrors were blocked (Anubis), so extraction is poor.

- JJ223BF specs: **not recovered.** The JJ-BC manual has a "JJ223BF-JJ1523BF (3 digits)" technical-data section (p.4 or p.2, sources disagree), but the table text was garbled. Stabilization "<3 s"/"<4 s" and temp "15-30 C / 18-23 C" appear without labels [~]. Warm-up 1 h [V]. Calibration: external weight (manual p.10-11 title) [V].
- JJ100B: 110 g x 0.001 g listed (p.1-2); repeatability, linearity, stabilization not stated; calibrating weight 2000 g appears in the table (column mapping dubious) [~]. Sensor wording: "industrial-class high precision sensors" (p.1).
- RS232C (JJ-BC sec.12, JJ-B sec.5): baud 600/1200/2400/4800/9600, default **600**, set in menu C3; 8 data bits, 1 start, 1 stop, no parity; DE-9. Pin text: 2 RxD, 3 TxD, 5 GND with ambiguous direction (balance vs PC) [~]. Max cable 2 m.
- Output: manual describes print-key or remote "print" command output; **no continuous mode documented** [V JJ-BC sec.12.2]. Frame described as sign, value, unit/"Pcs", stop char, CR (field widths garbled) [~]. No stable flag documented.
- Commands: ESC (0x1B) followed by `p` send data, `q` CAL, `r` counting, `s` unit select, `t` tare; valid when device number C4 = 27 (default) [V JJ-BC p.15; JJ-B p.4]. Syntax for a stable-gated response not documented.
- Conflict: OpenTrickler README lists "G&G JJ / JJB series" read-supported at 9600 8N1 with force-zero "unreliable" [C]. This implies a continuous output exists on at least some JJ firmware but I could not verify it from G&G manuals. Treat as undocumented.
- Whether JJ223BF has its own manual separate from JJ-BC: not found.

### 4. Sartorius Entris II 1 mg (Essential: BCE223 / 323 / 423 / 623 / 653, "-1S")
Sources: operating instructions https://documents.thermofisher.com/TFS-Assets/ANZ/manuals/sartorius-entris-II-essential-line-operating-instructions.pdf ; RS-232 technical note https://www.sartorius.hr/media/dypfvdsn/entris-ii-technical-note-en-sartorius.pdf . Page numbers are the TOC's; body numbers differ. Advanced Line (BCA) not covered.

- Specs (p.47): readability 1 mg; repeatability 0.5 mg typical at 5% load, 1 mg near max; linearity limit +-2 mg, typical +-0.6 mg; settling <=1.0 s typ.; measurement time <=1.5 s; metrological range +10 to +30 C, environment +5 to +40 C (p.42); warm-up ~2 h (p.44); I-models have internal motorized adjustment (isoCAL), others external.
- Sensor: not found.
- RS-232 (tech note p.1-5): 600-115200 baud; 7/8 bits; parity odd (default)/even/none; 1 stop; handshake HW (default), software, none; full duplex; 10 m max. Default 9600 / odd / SBI mode. Odd parity default: set 8N1 or match.
- Data frame (tech note p.2-3): 16-char: pos1 sign/space; 2 space; 3-10 value; 11 space; 12-14 unit; 15 CR; 16 LF. 22-char adds a 6-char ID prefix. No stable-flag position defined. `High`/`Low` for over/underload, `Err nnn` for faults, `Cal. Ext.` prompts. Example: +123.56 g.
- Print/output (manual sec.7.3): manual print with/without stability, AUTO.W/O and AUTO W/ (stability), AUTO.LC (on load change), and interval auto print. Max stream rate not stated [?].
- Commands (tech note p.4-5): `Esc ! CR LF` form, e.g. `Esc P` print/auto-print toggle, `Esc kP` PRINT key, `Esc T` zero/tare, `Esc U` key TARE, `Esc V` key ZERO; `Esc f3_` ZERO key, `Esc f4_` TARE key; `Esc x1_` model, `x2_` serial no., `x5_` ID. XBPI binary protocol is referenced but undocumented in these files; community reverse-engineered docs exist at https://graysonbellamy.github.io/sartoriuslib/ (community, unofficial).
- USB-C: "USB UTL"; class not stated.
- DIY support: OpenTrickler lists "Sartorius series" read-only at 9600 7N1 [C].

### 5. Mettler PB303-S / MS303S (and note on MS-TS)
Sources: PB-S OI https://manualmachine.com/mettlertoledo/ab54s/20840268-operating-instructions-manual ; FR OI https://www.mt.com/content/dam/mt_ext_files/Editorial/Generic/8/PB_0x0002467000027246000a1e32_files/ab-s-pb-s-ba-f-11780441.pdf ; MT-SICS manual (standard/advanced balances) https://b2b.wagnermunz.com/download/Mettler-Toledo%20GmbH/RM_Advanced_and_Standard_Level_Balances_SICS_EN.pdf ; MS-S BA https://wikis.mit.edu/confluence/download/attachments/83628254/MS-S_MS-L_BA_en_11781259E.pdf ; MS datasheet https://wikis-mit-edu.ezproxyberklee.flo.org/confluence/download/attachments/83628254/Newclassic_MS-Datasheet.pdf

- PB303-S (OI p.20-21): 310 g x 0.001 g; repeatability 0.001 g; linearity +-0.002 g; typical stabilization 2.0 s; ambient 10-30 C spec, 5-40 C operable; 200 g adjusting weight (accessory); external cal on non-certified models, internal weight on certified ones. Sensor not stated. Whether the PB-S SICS command set is the "Standard Level" in the SICS manual is [?].
- RS232C optional (p.17-18, 22): modes S.oFF / S.Stb / **S.Cont**; formats S.SICS or S.PM; baud 600-19200; 7E/7N/8N/7O; handshake off/soft/hard. Defaults Host: 9600, 8N, XON/XOFF (p.17). Pinout diagram not extractable; cables RS9-RS9 etc. listed (p.22) [~].
- PM-format examples (OI p.17): stable `/////1.67890/g`; continuous `S////1.67890/g`, `SD///1.39110/g` where flag characters are not defined in the OI.
- MT-SICS (OI p.31-32 FR): `S` stable, `SI` immediate, `SIR` repeated, `SR`, `SU`, `Z`, `@`; terminator CR LF. SICS manual: response `S S <value> <unit>` style, e.g. `SVSVVVVV14.256Vg` pattern with V=space (p.7); `S S`=stable, `S D`=dynamic, `S I`=busy, `S +`/`S -`=over/underload (p.9, 153); errors `ES`/`ET`/`EL` (p.10). Tare `T` -> `T S <val> <unit>` (p.181); zero `Z` -> `Z A` (p.194). Update rate: `UPD <rate>` sets 0.1 to 22.9 values/s for SIR/SIRU; balance rounds; needs baud >= 220 x rate (p.193). **Whether the PB-S supports T/UPD is unverified**; the PB-S OI only lists S, SI, SIR, SR, SU, ST, Z, @.
- MS303S (BA p.9-10, 35-41; datasheet): 320 g x 0.001 g; repeatability 0.001 g; linearity 0.002 g; settling 1.5 s; MonoBloc cell; temp drift 3 ppm/C; FACT internal adjust likely but footnote ambiguous [~]. RS232 default 9600 8N1, XON/XOFF, CR LF, IBM/DOS charset; options 600-38400; SEND.CONT / SEND.AUTO / SEND.STB / SEND.ALL; MT-SICS default, MT-PM and Sartorius (SART) emulations selectable. USB device port, class not stated.
- DIY support: none found for Mettler in OpenTrickler README. Generic scale driver "Depends".

### 6. Shimadzu UW/UX 1 mg (220H/420H/620H/820H/1020H)
Source: https://www.marsden-weighing.co.uk/storage/files/downloads/UWX-User-Manual.pdf (vendor manual hosted by a distributor; confirm against Shimadzu's own PDF).

- Specs (ch.3 p.5-6): 0.001 g d; repeatability <=0.001 g and linearity +-0.002 g for the first column; response 1.5-2.5 s; 5-40 C, 30-85% RH; sensitivity temp coefficient +-3 ppm/C (10-30 C). UW has built-in motor-driven weight; UX external. Sensor: patented UniBloc (p.1).
- RS-232C (ch.15): baud 300-38400, default 1200; 7/8 bits with odd/even/none; 1 or 2 stop bits; delimiter CR/LF/CR+LF (default CR); handshake none/XON-XOFF/DTR/timed (p.91-93). Pinout on balance: 1 FG, 2 TXD, 3 RXD, 4 RTS, 5 CTS, 6 DSR, 7 SG, 20 DTR (DB-25 numbering) (A-4 p.105). DCE/DTE not stated; the printed PC-to-balance cable wiring (p.80) is the manual's "null-modem" cabling [V].
- Frame, "EB type" (p.81-82): 12 bytes base: pos1 sign/space; 2-9 value; 10-11 unit; 12 delimiter; optional leading S (stable) / U (unstable), optional LF; overload `oL` / `-oL`.
- Output commands (p.84): D01 continuous (~80 ms interval with handshake off/hard), D03 continuous with stability, D05 single, D07 single with stability, D06/D09 (not described in extract). Key commands (p.85): TARE/T, UNIT, PRINT, POWER; unit commands g, kg, mg, ct, etc. Sartorius/Mettler emulation (S, SI, SIR, SR, T, TI, Z, ESC P, ESC T) (p.87-88).
- Shimadzu USB: none found. DIY support: none found.

### 7. U.S. Solid 500 g / 600 g x 1 mg
Source: https://file.ussolid.com/content/JFDBS/Muanual%20-%20U.S.%20Solid%20USS-DBS62%2863%29%20Balance.pdf (also USS-DBS 0.1 mg manual, different product).

- Models in the manual: 5003/5003N (500 g), 6003/6003N (600 g), 1 mg; "N" = internal cal. Stable time 1-3 s; repeatability/linearity +-1 to +-4 mg across the 1 mg models, per-model mapping garbled [~]. Operating temperature: not given numerically [V].
- Sensor: "electromagnetic force compensation" (sec.1) [V]. A retailer's "electromagnetic force sensor" naming matches.
- RS-232 (sec.4): DB-9; RXD pin 2, TXD pin 3, GND pin 5 on both ends; 1200-9600, default 9600; 8N1, 10-bit frame; modes press / timed / continuous (default); timed interval 0.5, 1, 2, 3 s.
- Output string and commands: format garbled; **no remote command set documented**; USB listed only as optional.
- DIY support: OpenTrickler lists "U.S.Solid JFDBS" read-only, 9600 8N1, no force zero [C]. Models are the same family in the OpenTrickler list but I did not verify that the 6003 shares the JFDBS format.

Other community leads (not verified, for the driver author): OpenTrickler (https://github.com/eamars/OpenTrickler, firmware https://github.com/eamars/OpenTrickler-RP2040-Controller), Ammolytics OpenTrickler peripheral (https://www.github.com/ammolytics/open-trickler-peripheral), AutoTrickler V3 compatibility notes (https://cdn.shopify.com/s/files/1/0525/7235/6768/files/1_-_AutoTrickler_V3.pdf, not read).

---

## (c) Recommended common firmware scale-driver interface

Design rule: the MCU must never assume a stable flag, a stream, or a command reply. The driver reports what the scale gave and the *application* does stability detection when the flag is missing.

```c
typedef enum { SCALE_OK, SCALE_NO_DATA, SCALE_OVERLOAD, SCALE_UNDERLOAD,
               SCALE_ERROR, SCALE_UNSUPPORTED } scale_status_t;

typedef enum { STAB_UNKNOWN, STAB_STABLE, STAB_UNSTABLE } scale_stab_t;

typedef struct {
    int32_t  value_ug;          /* weight in micrograms, signed; converted from unit */
    char     unit[4];           /* "g","ct","gn", ... raw unit text */
    scale_stab_t stable;        /* STAB_UNKNOWN if protocol has no flag */
    scale_status_t status;
    uint32_t t_rx_ms;           /* timestamp of last CR/LF received */
    uint32_t seq;               /* increments each valid frame */
} scale_sample_t;

typedef struct {
    bool has_stream;            /* continuous output available */
    bool has_stable_flag;       /* frame carries stability */
    bool can_tare, can_zero;    /* remote command exists */
    bool tare_zero_has_ack;     /* AK / "T S" / "Z A" reply exists */
    bool has_poll;              /* on-command read (Q/SI/S I/ESC p) */
    float max_rate_hz;          /* 0 if unknown/undocumented */
    uint32_t baud_default;
    uint8_t  data_bits, parity, stop_bits;   /* UART config */
    bool needs_null_modem;      /* wiring note; verify on hardware */
    bool needs_hw_flow;         /* e.g. Sartorius default HW handshake: disable in menu */
} scale_caps_t;

typedef struct scale_driver {
    const char *name;
    const scale_caps_t *caps;
    void (*init)(scale_ctx_t *c, uart_t *u);
    void (*rx_byte)(scale_ctx_t *c, uint8_t b);        /* ISR/ring feed; line parser */
    bool (*get_sample)(scale_ctx_t *c, scale_sample_t *out);  /* latest valid frame */
    void (*start_stream)(scale_ctx_t *c);   /* send SIR / D01 / set menu note; no-op if menu-set */
    void (*stop_stream)(scale_ctx_t *c);    /* C / @ / etc. */
    void (*request_sample)(scale_ctx_t *c); /* Q / SI / S I / ESC p / D05 */
    scale_status_t (*tare)(scale_ctx_t *c); /* may be async; verify via zero-drift */
    scale_status_t (*zero)(scale_ctx_t *c);
    void (*poll)(scale_ctx_t *c, uint32_t now_ms);  /* timeouts, retries */
} scale_driver_t;
```

Per-model parsers (all line-oriented, terminator CR or CR LF, tolerate both):
- A&D: regex `^(ST|US|OL),?([+-]\d+\.\d+)\s*(\w+)`.
- Mettler SICS: `S (S|D) <val> <unit>`; overload `S +`.
- Shimadzu EB: optional `S`/`U`, sign, 8-char value, 2-3 char unit; `oL`.
- Sartorius SBI: 16-char fixed columns; stable = UNKNOWN.
- Torbal AD / G&G / US Solid: fixed-column or `[+-]?\d+\.\d+ *unit`; stable = UNKNOWN.

Application-layer helpers (shared):
- `stable_by_window(samples, N, tol_ug)`: compute stability when `stable==STAB_UNKNOWN`. Needs enough rate (at 1-2 Hz expect 1-3 s latency).
- Stale-data watchdog: if `now - t_rx_ms` > 3 x expected period, treat as fault and stop dispensing.
- Frame sanity: reject non-printable bytes, mismatched length, parity errors; never act on one corrupted frame.
- Tare/zero verification: after `tare()` read back and confirm |value| < tol; if no ACK exists do not assume success.
- UART layer: configurable baud/bits/parity/stop per driver; hardware flow control disabled unless driver requires; MAX3232 board wired as DTE (pin 2 = RX in, pin 3 = TX out) for A&D/Sartorius-style balances (balance TXD on pin 2); provide a DB9 swap jumper for balances (JJ, US Solid, Torbal) whose direction is ambiguous.
- USB host: only as a second-tier option; Torbal (FTDI vendor class) and A&D Quick USB (one-way, class unknown) are not CDC-guaranteed. Mettler/Sartorius USB class not stated. Start with RS-232.

---

## (d) Blockers and risks (ranked for a trickler)

1. **Update rate and latency.** Trickling control wants >=5-10 readings/s. Verified: A&D FX-i 5/10/20 per s; Mettler MS-S/PB-S via SIR+UPD up to 22.9/s on MS (UPD on PB-S unverified); Shimadzu ~12.5/s. Not verified/undocumented: Torbal AD520 (Cont rate not stated), JJ-B/BC, Sartorius Entris II (only seconds-level interval documented). **US Solid DBS (<=2/s, fixed 0.5-3 s intervals) is likely too slow** for closed-loop trickling.
2. **No stable flag:** Torbal AD (16-byte frame), Sartorius 16-char, US Solid, G&G. Firmware must infer stability from the sample window, which is also dependent on scale internal filtering.
3. **Print-key / on-command only:** G&G JJ-BC/JJ-B manuals document only print-key or ESC p output, no continuous mode (in conflict with the OpenTrickler "read OK" report). Sartorius Entris II documents manual/auto-load-change/interval print, not a documented continuous stream.
4. **Undocumented pinout/direction:** Torbal AD, G&G, US Solid (ambiguous), Mettler PB-S/MS (pinout images not extracted). Check with a multimeter before connecting a MAX3232.
5. **Tare/zero without reply:** Torbal `S T`/`S Z` return nothing; G&G tare ("unreliable" per OpenTrickler); US Solid has none.
6. **Handshake defaults:** Sartorius defaults to hardware CTS/RTS and odd parity; Mettler MS defaults to XON/XOFF; Shimadzu may use handshake (DTR/timed). Menu changes required.
7. **USB is not a usable path** for the major options (see (c)).
8. **Documents missing / partial:** AD520-specific PDF, JJ223BF technical data, PB-S/MS pinout, Mettler PB-S SICS level, Shimadzu official PDF (distributor copy used), Entris II stream rate, all USB device classes, sensor type for Torbal/G&G/Sartorius/PB-S, OpenTrickler ADFX guide content.
9. **Source conflicts:** Torbal AD520 external-only calibration vs. manual's internal-cal wording; A&D connector male/female ambiguity (p.58 vs p.60).

Recommended order of work: (1) A&D FX-i driver (best documented, DCE, 20 Hz, ST/US flag, ack on tare/zero, field-proven in OpenTrickler); (2) Mettler SICS driver (SIR/UPD, S/D flag); (3) Shimadzu D01 driver; (4) Torbal/Sartorius/G&G as "read-only, firmware-derived stability" drivers after capturing real traces from a unit.
