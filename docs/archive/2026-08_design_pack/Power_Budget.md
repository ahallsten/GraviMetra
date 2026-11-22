# Power Budget

## Wall supplies on hand

- Mean Well IRM-30-12: 12 V at 2.5 A, 30 W.
- Mean Well IRM-20-5: 5 V at 4 A, 20 W.
- IRM-20-12 and IRM-10-12 remain spares/alternates.

## Design-use estimate

The following values are conservative design allowances, not measured prototype values.

| Load | Rail | Design allowance |
|---|---:|---:|
| One active NEMA17 + BTT TMC2209 | 12 V | 8-15 W typical design envelope; validate with actual motor/current |
| EMFC coil + OPA593 | 9-12 V | <1 W expected |
| Precision analog electronics | 5 V | approximately 0.25-0.5 W |
| Nextion NX8048T050 | 5 V | budget 5 W maximum supply capacity |
| STM32 + USB + logic | 3.3/5 V | approximately 0.5-1.5 W design allowance |
| CAN + RS-485 | 3.3/5 V | <0.5 W typical |
| Battery charger | 12 V input | program around 1 A battery charge initially; dynamically reduce if needed |

Only one auger motor is permitted to run at a time. This is an explicit power-budget and thermal-design assumption.

## Battery sizing

Recommended baseline: 2S, 7.4 V nominal, 10 Ah = approximately 74 Wh.

Allow converter loss and reserve; do not divide Wh by runtime and assume 100% usable energy.

Practical guidance:

- approximately 40-50 Wh: suitable starting size for around 2 hours of mixed/intermittent operation,
- approximately 70-80 Wh: recommended baseline for 2-4 hours of mixed operation,
- approximately 90-100 Wh: preferable if long periods of motor operation or high display brightness are expected.

The real runtime must be measured after auger motor current is tuned. TMC2209 current should be set only high enough to provide reliable auger torque.

## Wall-power priority

While plugged in:

- 12 V Mean Well directly supplies the motor rail through source selection.
- 5 V Mean Well directly supplies HMI/system 5 V through source selection.
- MAX77960B charges the 2S pack from the 12 V source.
- Firmware may reduce/suspend charging during precision settling.

While unplugged:

- 2S battery -> TPS61088 -> 12 V motor rail.
- 2S battery -> TPS62135 -> 5 V system/HMI rail.
- 5 V/other system rail -> TPS62172 -> 3.3 V digital rail.
- precision LT3045 derives the clean 5 V analog rail from a suitable system input with adequate headroom.
