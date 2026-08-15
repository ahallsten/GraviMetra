# Assembly and Reflow Plan

## Objective

Keep the first prototype affordable while letting JLCPCB/PCBWay place the components that benefit most from automated paste printing, pick-and-place, controlled reflow, and X-ray/inspection where available.

This document does not define one universal oven profile. The PCB assembler must use a lead-free profile compatible with the exact manufacturer/package revision and moisture-sensitivity condition of every populated part.

## Recommended assembly split

### Have the assembler populate

- STM32G491RET6 LQFP64.
- ADS1262.
- OPA593 package selected for PCB assembly.
- OPA2388 devices.
- TMP117 devices.
- LTC6655 reference.
- LT3045.
- TMUX1101.
- MAX77960B.
- TPS61088.
- TPS62135.
- TPS62172.
- TPS2121.
- TPS25947 where used.
- TCAN1042-family transceiver.
- THVD1450.
- TPD2EUSB30.
- USB-C receptacle if the assembler stocks/accepts the exact connector.
- normal 0402/0603/0805/1206 passives.
- small inductors and switching-regulator passives if available through the assembler.
- matched resistor network.
- precision reference passives.

The exposed-pad/QFN/WSON-style power components are exactly the parts where automated paste/reflow is most valuable. Use manufacturer paste-window guidance and sufficient thermal vias, while avoiding excessive solder that can float the package.

### Prefer post-reflow hand installation / secondary assembly

- four BigTreeTech TMC2209 modules.
- female headers/sockets for the TMC modules.
- Mean Well IRM-30-12 and IRM-20-5 modules if mounted to a separate mains assembly.
- IEC-C14 inlet and double-pole switch wiring.
- protective-earth hardware.
- pluggable screw terminals.
- large battery/tool-battery connectors.
- E-stop field wiring connectors if mechanically large.
- large electrolytic/bulk capacitors if assembly-house pricing is unfavorable.
- the EMFC voice coil, photodiode/mechanical optical assembly, and mechanism harnesses.
- any expensive precision part the assembler cannot source economically; consignment can be evaluated against hand soldering.

Do not send a preassembled BTT StepStick module through another ordinary SMT reflow cycle.

## Mean Well soldering note

The Mean Well IRM modules are not treated as ordinary SMT parts. Their manufacturer documentation includes through-hole soldering limits; keep them in the mains assembly and install them after SMT reflow. Follow the current IRM-series datasheet for wave/manual-solder limits for the exact module.

## Precision shunt assembly

The VPG/Vishay four-terminal foil shunt deserves special handling:

- use the manufacturer's recommended four-pad land pattern,
- do not distort the body by forcing misaligned pads,
- keep heavy copper mechanically symmetric where practical,
- avoid board flexure after soldering,
- clean flux residue according to the resistor/manufacturer/assembler process,
- Kelvin sense pads must not carry actuator current.

If the exact Y1606/VCS1625ZP part is expensive or unavailable from the assembly house, hand placement with paste/hot plate or a controlled soldering process is acceptable, but avoid large thermal gradients and mechanical stress.

## Moisture sensitivity and storage

For production release, add the exact supplier/manufacturer MSL for every moisture-sensitive IC to the BOM/assembly notes. The MSL is package-specific and may not be reliably inferred from the generic family datasheet.

If a reel/tray has exceeded its floor life, follow the manufacturer's/JEDEC bake requirements rather than improvising a bake temperature.

## Board finish and cleanliness

For the precision sensor board:

- ENIG is a reasonable prototype finish.
- Specify high-quality board cleanliness.
- Clean ionic/flux residues, especially around photodiode/TIA high-impedance nodes, ADC inputs, and reference circuitry.
- Avoid conformal coating over optical parts or mechanically sensitive flexure interfaces unless the coating process has been validated.
- If conformal coating is later required for environmental protection, characterize leakage and mechanical/thermal effects first.

## First-run quantity / cost strategy

For the first revision, a practical strategy is:

- order 5 bare sensor PCBs and 5 controller PCBs,
- have 2-3 of each SMT assembled,
- retain bare boards for rework/experiments,
- install expensive modules, terminals and mechanical connectors yourself,
- order precision parts separately if JLCPCB/PCBWay procurement markup is large.

This reduces prototype cost while preserving spare PCBs for destructive/debug changes.

## DFM checks before ordering

Before uploading fabrication files:

1. run Altium ERC and DRC with zero unexplained violations,
2. verify every footprint against the local manufacturer drawing,
3. verify pin 1 on schematic, footprint and pick-and-place rotation,
4. verify exposed-pad paste openings,
5. verify USB-C board-edge offset,
6. verify TMC module socket orientation using a physical module,
7. confirm no tall part collides with a TMC heatsink/module,
8. confirm every hand-installed connector has accessible solder joints,
9. verify fiducials and tooling requirements against the selected assembler,
10. export and visually inspect Gerbers/ODB++, drill files, centroid/pick-and-place file and BOM,
11. inspect the assembler's online 3D/placement preview before payment,
12. explicitly DNP all parts intended for hand installation.
