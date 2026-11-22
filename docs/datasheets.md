# Datasheet index

Local copies (read-only) are in `reference/datasheets/`. Superseded parts are in `docs/archive/2026-08_design_pack/Datasheets/`.
For new parts, add a row with the manufacturer URL and revision instead of committing the PDF.

| Part | Use | Local file | Manufacturer URL | Rev / date checked |
|---|---|---|---|---|
| STM32G491 | MCU candidate | STM32G491_Datasheet.pdf, STM32G4_RM0440_Reference_Manual.pdf, STM32G4_AN5093_Hardware_Development.pdf | — | Aug 2026 copy; DS13122 **Rev 4** used for the pin budget |
| ADS1262 | Precision ADC (Config E) | ADS1262_Datasheet.pdf | — | Aug 2026 copy |
| LTC6655 | Reference | LTC6655_Datasheet.pdf | — | Aug 2026 copy |
| LT3045 | Low-noise LDO | LT3045_Datasheet.pdf | — | Aug 2026 copy |
| OPA2388 / OPA593 | Precision op amp / coil driver (OPA593 out of stock, OI-04) | OPA2388_Datasheet.pdf, OPA593_Datasheet.pdf | — | Aug 2026 copy |
| VCS1625ZP / Y1606 | Foil shunt | VPG_VCS1625ZP_Y1606_Datasheet.pdf | — | Aug 2026 copy |
| TMP117 | Temperature sensors (magnet/coil/ambient) | TMP117_Datasheet.pdf | — | Aug 2026 copy |
| TMC2209 | Stepper drivers (×3) | TMC2209_Datasheet.pdf | — | Aug 2026 copy (Rev 1.08, 2022-05-25) |
| Mean Well IRM-20 / IRM-30 | Mains modules (owned) | MeanWell_IRM-20_Datasheet.pdf, MeanWell_IRM-30_Datasheet.pdf | — | Aug 2026 copy |
| TPS25947 | eFuse | TPS25947_Datasheet.pdf | — | Aug 2026 copy |
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
| SmCo disc listings | Magnet sourcing (OI-29) | distributor listings | First4magnets (Ø25 Sm2Co17 ~$32), Apex Magnets SmCo30 discs, Stanford Advanced Materials 1 × 3/8 in 26M, McMaster-Carr SmCo | seen 2026-11-22; exact 1¼ × ½ in not yet found |
| Carpenter Temperature Compensator 30 (Type 2) | Passive magnet tempco shunt (D-062, OI-30) | not committed (linked) | https://www.carpentertechnology.com/alloy-finder/temperature-compensator-30-type-2 | read 2026-11-22: 30 % Ni-Fe, permeability falls with T, −51..71 °C, bar/strip; **no curves published, request the datasheet** |
| Sartorius WZB254-NC | Benchmark weigh cell | Sartorius_WZB254-NC_Benchmark_Datasheet.pdf | — | Aug 2026 copy |

Every row needs re-verification against the current revision before it drives a design decision (CLAUDE.md rule).
