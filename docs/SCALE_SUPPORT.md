# Scale support matrix (SCS-05)

Rev 0 · 2026-11-21. Driver detail: `docs/firmware/SCALE_DRIVER_SPEC.md`. Evidence: `docs/SCALE_SUPPORT_research.md` and the A&D manual (extracted 2026-11-21).

**Verification states:** DOC = from the manual only · SIM = passes host tests against the simulator · HW = verified on a real unit (rate, flag, tare, cable logged).

| Tier | Model | Protocol / format | Max rate | Stable flag | Tare/zero reply | Balance connector | Cable from B2 (DE-9 M, DTE) | Balance menu changes from factory | Pan adapter (SCS-04) | Driver | Verified |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | A&D FX-120i / FX-300i, FZ-i | A&D standard, 15 chars + CR LF; `SIR`/`Q`/`T`/`Z`/`C` | 20/s | ST / US / OL | AK twice (`erCd 1`) | DE-9 **male**, DCE | Straight-through **F-F**, 9 pins | 19200, 8N1, `erCd 1`, `prt 3`, `5pd 2` (FAST), unit g | TBD (Phase 4, per-model printed) | spec'd | DOC |
| 2 | Mettler MS-S / PB-S | MT-SICS `S S/D` | 22.9/s (MS-S, UPD) | S / D | `T S`, `Z A` | RS-232C 9-pin; pinout [?] | TBD after DMM check | handshake off; 8N1 | TBD | outline | DOC |
| 3 | Shimadzu UW/UX 1 mg | EB, `D01` stream | ≈ 12.5/s | S / U prefix | [?] | DB-25 | DB-25 adapter cable, wiring TBD | baud ≥ 9600, CR LF | TBD | outline | DOC |
| 4 | Torbal AD520 | 16-byte, `S I`/`S T`/`S Z` | not stated | **none** → derived, `degraded` | none | RS-232 (pinout [?]) | swap jumpers if needed | 4800 8N1 (LONG) | TBD | not started | — |
| 4 | Sartorius Entris II | SBI 16-char | not stated | none → derived | [?] | DE-9 | TBD | HW handshake off, parity | TBD | not started | — |
| 4 | G&G JJ-B/BC | ESC p only | not documented | none | unreliable | DE-9 | swap jumpers if needed | — | TBD | not started | — |
| ✗ | US Solid DBS | fixed 0.5–3 s interval | ≤ 2/s | none | none | DE-9 | — | — | — | **not supported**: too slow for MET-13/SCS-02 | — |

Config E's EMFC module appears to the firmware as one more `ScaleSource` (internal, ≥ 10 Hz, firmware stability), with no cable.
