# Datasheet index

Local copies (read-only) are in `reference/datasheets/`. Superseded parts are in `docs/archive/2026-08_design_pack/Datasheets/`.
For new parts, add a row with the manufacturer URL and revision instead of committing the PDF.

| Part | Use | Local file | Manufacturer URL | Rev / date checked |
|---|---|---|---|---|
| STM32G491 | MCU candidate | STM32G491_Datasheet.pdf, STM32G4_RM0440_Reference_Manual.pdf, STM32G4_AN5093_Hardware_Development.pdf | — | Aug 2026 copy; DS13122 **Rev 4** used for the pin budget |
| ADS1262 / ADS1263 | Precision ADC (Config E); ADS1263 proposed (D-064) | ADS1262_Datasheet.pdf (covers both) | ti.com: ACTIVE, Rev C 25-May-2021 = local copy (checked 2026-11-22) | Aug 2026 copy = **SBAS661C, May 2021**; Table 8-1/8-3 noise, INL 3/12 ppm FSR, gain drift 0.5/4 ppm/K used 2026-11-22 |
| LTC6655 | Reference | LTC6655_Datasheet.pdf | — | Aug 2026 copy; B grade 2 ppm/°C max, 0.25 ppm p-p, LS8 hysteresis 30 ppm, 20 ppm/kHr used 2026-11-22 |
| LT3045 | Low-noise LDO | LT3045_Datasheet.pdf | — | Aug 2026 copy |
| OPA2388 / OPA593 | Precision op amp (now also the coil driver, D-066) / former coil driver (not needed) | OPA2388_Datasheet.pdf, OPA593_Datasheet.pdf | — | Aug 2026 copy; OPA2388 ISC ±60 mA used 2026-11-22 |
| VCS1625ZP / Y1606 | Foil shunt (**superseded for the coil**: 0.3–10 Ω range; the ~10 mA coil needs ~150 Ω, D-064) | VPG_VCS1625ZP_Y1606_Datasheet.pdf | — | Aug 2026 copy; TCR ±0.05 ppm/K typ (0–60 °C), PCR 5 ppm at rated power used as family values |
| TMP117 | Temperature sensors (magnet/coil/ambient) | TMP117_Datasheet.pdf | — | Aug 2026 copy |
| TMC2209 | Stepper drivers (×3) | TMC2209_Datasheet.pdf | — | Aug 2026 copy (Rev 1.08, 2022-05-25) |
| Mean Well IRM-20 / IRM-30 | Mains modules (owned) | MeanWell_IRM-20_Datasheet.pdf, MeanWell_IRM-30_Datasheet.pdf | https://www.meanwell.com/Upload/PDF/IRM-20/IRM-20-SPEC.PDF ; https://www.meanwell.com/Upload/PDF/IRM-30/IRM-30-SPEC.PDF | IRM-20-SPEC 2025-11-21 / IRM-30-SPEC 2026-04-03; SHA-256 identical to the local copies (checked 2026-11-22) |
| TPS25947 | eFuse | TPS25947_Datasheet.pdf | https://www.ti.com/lit/ds/symlink/tps25947.pdf | local copy = SLVSFC9C, May 2026 |
| TPS62135 / TPS62172 / TPS61088 | DC/DC | TPS62135_Datasheet.pdf, TPS62172_Datasheet.pdf, TPS61088_Datasheet.pdf | — | Aug 2026 copy |
| TMUX1101 | Analog switch | TMUX1101_Datasheet.pdf | — | Aug 2026 copy |
| TPD2EUSB30 / USB4105 | USB ESD / USB-C receptacle | TPD2EUSB30_Datasheet.pdf, GCT_USB4105_USB-C_Receptacle.pdf | — | Aug 2026 copy |
| TSAL6200 | IR emitter (optical sensor) | Vishay_TSAL6200_Datasheet.pdf | — | Aug 2026 copy |
| Susumu resistor networks | Matched resistors | Susumu_Precision_Resistor_Networks_Catalog.pdf | — | Aug 2026 copy |
| A&D FZ-i / FX-i manual | Config S tier-1 driver (frame, commands, RS-232 pinout, function table) | not committed (linked) | https://aandd.jp/products/manual/balances/fz_fx-i.pdf | fetched + text-extracted 2026-11-21 |
| STM32G491 pin/AF data | Pin budget | derived: `analysis/data/stm32g491_pins.json` | DS13122 Rev 4 (local copy) | extracted 2026-11-21 |
| Arnold Recoma (SmCo5 / Sm2Co17) summary + grade sheets | Magnet grade (Recoma 28 used) | not committed (linked) | https://www.arnoldmagnetics.com/wp-content/uploads/2017/10/Recoma-Combined-160301.pdf | fetched + text-extracted 2026-11-22; summary Rev. 160205a |
| Arnold Recoma Stab (temperature-compensated SmCo) | TC grade option (D-059) | not committed (linked) | https://www.arnoldmagnetics.com/wp-content/uploads/2020/02/Recoma-stab-23.01.2020.pdf | fetched 2026-11-22; table columns realigned by (BH)max check |
| EEC temperature-compensated SmCo material sheet | TC grade option (D-059) | not committed (linked) | https://www.electronenergy.com/hubfs/Electronenergy_December2018/Docs/EEC-SmCo-TC-Material-Sheet.pdf | fetched 2026-11-22; Br column misaligned in extraction, **confirm (OI-27)** |
| Precision Brand 1095 blue-tempered shim, 3 in × 50 in rolls | Hinge blades (D-054) | distributor listings | Zoro 0.002 in https://www.zoro.com/precision-brand-shim-stock-roll-high-blue-stl-00020-in-23110/i/G2549452/ ($26.45); Zoro 0.003 in ($34.89); Penn Tool C-2 0.002 in ($76.95) | prices seen 2026-11-22 in search results, not confirmed in a cart |
| SmCo disc listings | Magnet sourcing (OI-29) | distributor listings | First4Magnets (now Ø25 × 20 and Ø30 × 10, grade not stated; the earlier "Ø25 Sm2Co17 ~$32" listing is gone), Adams 55B0063 (Ø38.1 × 12.7, grade 26), Apex Magnets SmCo30 discs, Stanford Advanced Materials, McMaster-Carr | seen 2026-11-22; neither Ø31.75 × 12.7 nor Ø25.4 × 12.7 is stocked (`research/GATE_B_SOURCING.md`) |
| Carpenter Temperature Compensator 30 (Type 2) | Passive magnet tempco shunt (D-062, OI-30) | not committed (linked) | https://www.carpentertechnology.com/alloy-finder/temperature-compensator-30-type-2 | read 2026-11-22: 30 % Ni-Fe, permeability falls with T, −51..71 °C, bar/strip; **no curves published, request the datasheet** |
| OPR2100 | Position detector (OI-31) | not committed (linked) | https://www.ttelectronics.com/TTElectronics/media/ProductFiles/Datasheet/OPR2100.pdf | Rev G 06/2022, read 2026-11-22; Active, Digi-Key 770 at $11.51 |
| VPG Z201 (Z-foil, through-hole) | Shunt, proposed 150 Ω Y1453150R000T9L (OI-05) | not committed (linked) | https://vpgfoilresistors.com/products/through-hole-leaded-resistors/z201/datasheet (copy read: https://www.farnell.com/datasheets/2007603.pdf) | doc 63187 Rev 22-Oct-13 (a newer revision may exist); TCR 0.2 ppm/K typ, ±0.6 worst-case spread |
| VPG FRSM (Z1 foil, SMD) | Shunt alternative FRSM1206 | not committed (linked) | https://vpgfoilresistors.com/products/surface-mount-resistors/frsm1206/datasheet (bot check; LCSC copy https://atta.szlcsc.com/upload/public/pdf/source/20260129/2756F11B81B15372F0D120D8369D6090.pdf) | doc 4220-EN Rev 25-Mar-2025 |
| VPG PCN FR-005-2024 | VSMP/VFCP end-of-life notice | not committed (linked) | https://www.ttieurope.com/content/dam/tti-europe/products/PCN/VishayPrecisionGroup/VPGFoilResistor-PCN-VPG%20FR-005-2024-Rev-1.pdf | dated 22-Aug-2024: last-time buy Sep 2025, last shipment Jan 2026 |
| Metals Handbook DC magnetisation curves (FEMM digitisation) | Pot steel B-H bracket (OI-28) | not committed (linked) | https://www.femm.info/wiki/SoftMagneticMaterials | MH 8th ed. Vol. 1 (1961) Fig. 17 p. 792; read 2026-11-22 |
| SendCutSend HRP&O / 1008 CR | Pot steel (OI-28) | linked | https://sendcutsend.com/materials/mild-steel/hrpo/ ; https://sendcutsend.com/materials/mild-steel/cold-rolled/ | read 2026-11-22 |
| First4Magnets F3010SC Ø30 × 10 SmCo | Rig magnet (OI-29) | linked | https://www.first4magnets.com/us/product/30mm-dia-x-10mm-thick-samarium-cobalt-magnet-139kg-pull-19173 | $18.21, in stock, grade not stated, 2026-11-22 |
| JLC / Digi-Key stock snapshot | STM32G491VET6 C3209873 (17), TMC2209-LA-T C2150710 (~24k), TMP117AIDRVR C699536 (5,437), ADS1263IPWR C2861295 (74), OPA2388IDR (16k), LTC6655BHMS8-2.5 (0 at JLC, 980 at Digi-Key) | — | jlcsearch.tscircuit.com mirror (unofficial, nightly); Digi-Key product pages | seen 2026-11-22; details in `docs/phase2/research/GATE_B_SOURCING.md` |
| Mean Well PCB installation manual | Module mounting and clearances | — | https://www.meanwell.com/Upload/PDF/PCB_EN.pdf | 2023-11-21 edition, read 2026-11-22 (no fuse guidance) |
| TPS2121 | 5 V / USB power mux | docs/archive/2026-08_design_pack/Datasheets/TPS2121_Datasheet.pdf | https://www.ti.com/lit/ds/symlink/tps2121.pdf | SLVSEA3F, Aug 2020 |
| MAX3232E | RS-232 transceiver | — | https://www.ti.com/lit/gpn/MAX3232E | SLLS664E, Jun 2021; 0.3 / 1 mA read via search, **confirm** |
| Bel Fuse 5HT | Time-lag ceramic fuses (D-076) | — | https://www.belfuse.com/media/datasheets/products/circuit-protection/ds-cp-5ht-5htp-series.pdf | Rev June 2026, read 2026-11-22 |
| Bel 5ST (glass) | Rejected: EOL 2024-09-30, 35 A breaking | — | https://www.belfuse.com/media/datasheets/products/circuit-protection/ds-cp-5st-5stp-series.pdf | Rev Apr 2023 |
| MCUFRIEND 3.5" shield current | Display power (ASSUMED until measured, OI-21) | — | https://forum.arduino.cc/t/mcufriend-3-5-touchscreen-backlight-mod-possible/562286 ; http://www.lcdwiki.com/3.5inch_Arduino_Display-UNO | read 2026-11-22 (400 mA; 1.12 W) |
| StepperOnline 17HS08-1004S | Trickler motor (ASSUMED, not chosen) | — | https://www.omc-stepperonline.com/nema-17-bipolar-1-8deg-16ncm-22-6oz-in-1a-3-7v-42x42x20mm-4-wires.html | search summary 2026-11-22 (1.0 A, 3.5–3.7 Ω) |
| StepperOnline NEMA 8 + 19:1 gearbox | Check-mass motor (ASSUMED, not chosen) | — | https://www.omc-stepperonline.com/nema-8-stepper-motor-bipolar-l-38mm-w-gear-ratio-19-1-planetary-gearbox.html | search summary 2026-11-22 (0.6 A, 10 Ω) |
| Sartorius WZB254-NC | Benchmark weigh cell | Sartorius_WZB254-NC_Benchmark_Datasheet.pdf | — | Aug 2026 copy |

Every row needs re-verification against the current revision before it drives a design decision (CLAUDE.md rule).
