# mech/ — mechanical CAD (build123d)

Planned layout (Phase 2+):
- `params/`: one parameter file per assembly
- `src/`: build123d sources (cell, counterweight, tricklers, hopper, draft shield, enclosure, Config S adapters)
- `exports/step/`, `exports/stl/`, `exports/dxf/` (SendCutSend flats), `drawings/` (PDF)
- `print_notes.md` (Bambu Studio settings per part) and `sendcutsend_order.md`

Rule: no printed parts in the weighing cell or anything supporting it (D-004).
