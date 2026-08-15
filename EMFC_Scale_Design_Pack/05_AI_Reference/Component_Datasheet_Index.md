# Component Datasheet Index and Design-Critical Notes

## How to use this index

The local PDF filenames below are included in `../01_Electrical_Design/Datasheets/`. Use the local files first so the project remains usable offline. URLs are included to check for later revisions before ordering production hardware.

This index summarizes **design-critical** information from the source material. It is not a substitute for the complete datasheet. Before releasing a schematic or PCB, compare the local copy's revision/date with the manufacturer's current revision.

| Function | Selected component / family | Local source | Design-critical use in this project |
|---|---|---|---|
| MCU | STM32G491RET6 | `STM32G491_Datasheet.pdf`; `STM32G4_RM0440_Reference_Manual.pdf`; `STM32G4_AN5093_Hardware_Development.pdf` | 3.3 V MCU; five USART/UART + LPUART, USB FS device, two FDCAN controllers. Use LQFP64. Decouple every VDD pair locally; treat VDDA/VREF separately; expose SWD/NRST/BOOT. FDCAN still requires external transceiver. |
| Precision ADC | TI ADS1262 | `ADS1262_Datasheet.pdf` | 32-bit delta-sigma ADC. Use AVDD=5 V, DVDD=3.3 V, SPI+DRDY. Primary differential shunt measurement. Follow reference/input-decoupling and layout sections exactly. |
| Coil/current amplifier | TI OPA593 | `OPA593_Datasheet.pdf` | Linear high-current op amp. Use about 9-12 V actuator supply; normal scale current about 5-60 mA; hardware limit near 80 mA. Implement manufacturer current-limit equation and thermal layout. |
| Zero-drift AFE/servo | TI OPA2388 | `OPA2388_Datasheet.pdf` | Dual zero-drift op amp for photodiode TIAs and low-frequency servo. Confirm common-mode/output swing at chosen 5 V supply. |
| Precision reference | ADI LTC6655-2.5 | `LTC6655_Datasheet.pdf` | Very-low-noise 2.5 V reference. Follow exact input/output capacitor ranges and layout. Keep thermally quiet and lightly/deterministically loaded. |
| Precision analog LDO | ADI LT3045 | `LT3045_Datasheet.pdf` | Ultra-low-noise 500 mA-class LDO for analog rail. Follow SET capacitor, output capacitor, dropout and thermal guidance. |
| Analog loop reset | TI TMUX1101 | `TMUX1101_Datasheet.pdf` | Low-leakage switch to reset/clamp servo integrator. Keep analog signal within switch supply rails. |
| Temperature sensor | TI TMP117 | `TMP117_Datasheet.pdf` | Digital precision temperature measurement for magnet, flexure, and AFE. Verify I2C address strategy for multiple sensors. |
| Split photodiode | Hamamatsu S3096-02 | `Hamamatsu_S3096-02_Datasheet.pdf` | Two-segment Si PIN photodiode for optical null detector. Design opaque mechanical cavity and minimize leakage/noise at TIA inputs. |
| IR emitter | Vishay TSAL6200 | `Vishay_TSAL6200_Datasheet.pdf` | 940 nm emitter. Use continuous DC current for low EMI; set current from 5 V rail within optical/thermal ratings. |
| Precision shunt | VPG/Vishay VCS1625ZP / Y1606, 10 ohm target | `VPG_VCS1625ZP_Y1606_Datasheet.pdf` | Four-terminal Z-foil current sense. 10 ohm is at top of family range. Use true Kelvin routing and manufacturer's large four-pad land pattern. Initial tolerance calibrates out; TCR/PCR/long-term stability matter. |
| Matched resistor network | Susumu RM3216 family, 4x10k matched variant | `Susumu_Precision_Resistor_Networks_Catalog.pdf` | Use matched network for optical-difference amplifier where ratio matching/relative TCR dominate absolute tolerance. Verify exact suffix and footprint before ordering. |
| Stepper driver | TMC2209 IC on BigTreeTech StepStick module | `TMC2209_Datasheet.pdf` | Carrier PCB receives module VM, GND, VIO, STEP, DIR, EN, UART. Four sockets; one motor active at once. Use local VM bulk capacitance; do not hot-plug. Verify actual BTT module pinout/orientation before footprint release. |
| HMI | Nextion NX8048T050 | `Nextion_NX8048T050_Dimensions.pdf` plus official Nextion product documentation | 5 V HMI, UART interface. Budget 1 A branch capacity. Short cable expected. UI is not safety control. Verify exact connector pinout on owned unit before harness release. |
| USB-C receptacle | GCT USB4105 or equivalent | `GCT_USB4105_USB-C_Receptacle.pdf` | USB2-only receptacle. Verify board-edge geometry and shell pads. Device mode uses 5.1k Rd on both CC pins. |
| USB ESD | TI TPD2EUSB30 | `TPD2EUSB30_Datasheet.pdf` | Place immediately at connector; very short ESD ground path; route USB D+/D- through/near protector as recommended. |
| CAN transceiver | TI TCAN1042H / compatible VIO option | `TCAN1042H_Family_Datasheet.pdf` | External CAN physical layer for STM32 FDCAN. Choose exact VIO-compatible suffix. Use selectable 120 ohm termination and connector-side surge/ESD provisions. |
| RS-485 transceiver | TI THVD1450 | `THVD1450_Datasheet.pdf` | Half-duplex RS-485 physical layer. Dedicated UART. Selectable 120 ohm termination; leave bus-bias footprints but coordinate bus-wide biasing. |
| Li-ion charger / power path | ADI MAX77960B | `MAX77960B_Datasheet.pdf` | 2S Li-ion buck-boost charging/power path. Requires exact inductor, capacitor, thermistor/current setup and high-current switching layout from datasheet. Do not design this block by generic intuition. |
| Battery 12 V boost | TI TPS61088 | `TPS61088_Datasheet.pdf` | 2S battery -> 12 V motor rail. Sized for one active NEMA17 channel, unlike superseded TPS63070 concept. Select inductor, switch frequency, compensation and current limit from actual worst-case motor power. |
| Battery/system 5 V buck | TI TPS62135 | `TPS62135_Datasheet.pdf` | 2S battery -> 5 V HMI/system rail. Confirm output-current and thermal margin with Nextion at up to 1 A plus digital loads. |
| 3.3 V digital regulator | TI TPS62172 | `TPS62172_Datasheet.pdf` | 3.3 V logic rail. Recalculate final MCU+digital current before release; replace with higher-current part if margin is inadequate. |
| Source mux | TI TPS2121 | `TPS2121_Datasheet.pdf` | Wall/battery rail source prioritization where used. Configure priority/current-limit/soft-start from actual rails and prevent backfeed. |
| Input protection/eFuse | TI TPS25947 family | `TPS25947_Datasheet.pdf` | Low-voltage DC input inrush/OV/current protection where appropriate. This does not replace the mains fuse or mains switch. |
| Wall 12 V AC/DC | Mean Well IRM-30-12 | `MeanWell_IRM-30_Datasheet.pdf` | 12 V, 2.5 A, 30 W wall rail. Treat as mains-voltage device; segregate creepage/clearance and PE enclosure design. Do not run exposed mains on precision board. |
| Wall 5 V AC/DC | Mean Well IRM-20-5 | `MeanWell_IRM-20_Datasheet.pdf` | 5 V, 4 A, 20 W wall HMI/system rail. Mains segregation requirements same as above. |
| Benchmark EMFC cell | Sartorius WZB254-NC | `Sartorius_WZB254-NC_Benchmark_Datasheet.pdf` | Commercial benchmark near project capacity; demonstrates sub-mg readability/repeatability is commercially achievable at 250 g with EMFC. Not electrically copied into custom design. |

## Source/revision URLs

Manufacturer pages should be checked immediately before ordering because distributor mirrors may lag current revisions:

- STM32G491: https://www.st.com/en/microcontrollers-microprocessors/stm32g491re.html
- ST G4 hardware-development guide AN5093: https://www.st.com/resource/en/application_note/an5093-getting-started-with-stm32g4-series-hardware-development-stmicroelectronics.pdf
- ADS1262: https://www.ti.com/product/ADS1262
- OPA593: https://www.ti.com/product/OPA593
- OPA2388: https://www.ti.com/product/OPA2388
- TMP117: https://www.ti.com/product/TMP117
- TMUX1101: https://www.ti.com/product/TMUX1101
- TPS61088: https://www.ti.com/product/TPS61088
- TPS62135: https://www.ti.com/product/TPS62135
- TPS62172: https://www.ti.com/product/TPS62172
- TPS2121: https://www.ti.com/product/TPS2121
- TPS25947: https://www.ti.com/product/TPS25947
- TCAN1042H family: https://www.ti.com/product/TCAN1042H
- THVD1450: https://www.ti.com/product/THVD1450
- TPD2EUSB30: https://www.ti.com/product/TPD2EUSB30
- LTC6655: https://www.analog.com/en/products/ltc6655.html
- LT3045: https://www.analog.com/en/products/lt3045.html
- MAX77960B: https://www.analog.com/en/products/max77960b.html
- TMC2209: https://www.analog.com/en/products/tmc2209.html
- VPG VCS1625ZP: https://vpgfoilresistors.com/products/current-sense-resistors/vcs1625zp/datasheet
- Hamamatsu S3096-02: https://www.hamamatsu.com/us/en/product/optical-sensors/photodiodes/si-photodiode-array/segmented-type-si-photodiode/S3096-02.html
- Nextion NX8048T050: https://nextion.tech/datasheets/nx8048t050/
- Mean Well IRM series: https://www.meanwell.com/webapp/product/search.aspx?prod=IRM-30 and https://www.meanwell.com/webapp/product/search.aspx?prod=IRM-20

## Reflow / assembly note

Do not assume every part has the same reflow profile. JLCPCB/PCBWay should assemble standard SMT parts according to their controlled lead-free process, but the exact ordered package's moisture-sensitivity level, peak reflow temperature, storage/bake requirements, and number of allowed reflow cycles must be checked in current manufacturer/package documentation and distributor labeling.

The encapsulated Mean Well IRM mains modules, BTT TMC2209 modules, large pluggable terminals, battery/tool connectors, IEC inlet/switch wiring, and other bulky/mechanical parts are intentionally good candidates for manual/post-reflow installation rather than being subjected to ordinary board SMT assembly.
