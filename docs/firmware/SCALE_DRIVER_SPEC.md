# Scale driver specification: RS-232 port and A&D FX-i / FZ-i driver (Config S)

Rev 0 · 2026-11-21 · Covers SCS-01..06, FW-03 and MET-13. Status: **PROPOSED (Gate B)**. A&D facts are [V] from the FZ-i/FX-i instruction manual (https://aandd.jp/products/manual/balances/fz_fx-i.pdf, text extracted locally 2026-11-21), cited by manual section and printed page. Other models are summarized from `docs/SCALE_SUPPORT_research.md`.

## 1. Layering

```
Uart (HAL, hal/interfaces.hpp)          bytes in/out, buffer ownership per contract
  └─ ScaleDriver (one per protocol)     line framing, parsing, command sequencing, timeouts
       └─ ScaleSource (common interface) consumed by the dispense planner and the HMI model
            ↑ EmfcScaleSource (Config E)  wraps the existing measurement pipeline
```

- **Drivers carry no dosing policy.** They report exactly what the scale said, and never invent a stable flag, an ACK or a stream (research §(c) design rule).
- **Stability for scales that don't report it** is derived above the driver by the existing `measurement::StabilityDetector`, with the optical and coil checks disabled. Such a scale is flagged `degraded` on the HMI (SCS-02).
- **Messages are independent of transport** (firmware invariant). A USB-host transport could replace `Uart` later without touching the parsers.

## 2. Common interface (C++17, no allocation after init)

```cpp
namespace gravimetra::scale {

enum class Stability : std::uint8_t { unknown, stable, unstable };
enum class Condition : std::uint8_t { ok, overload, underload, scale_error, unsupported_unit };

struct ScaleSample {
    std::uint64_t rx_timestamp_us{0};   // time the terminator was received
    std::int32_t  mass_ug{0};           // exact decimal parse; ±2147 g range covers 520 g
    Stability     stability{Stability::unknown};
    Condition     condition{Condition::scale_error};
    std::uint32_t sequence{0};          // +1 per valid frame
};

struct ScaleCapabilities {               // constant per driver, from the manual
    bool stream; bool stable_flag; bool tare_cmd; bool zero_cmd; bool cmd_ack; bool poll_cmd;
    std::uint8_t max_rate_hz;            // 0 = undocumented
    std::uint32_t resolution_ug;         // 1000 for 1 mg balances
};

enum class CommandState : std::uint8_t { idle, sent, acknowledged, completed, failed, timed_out };

struct ScaleHealth {
    bool stale;                          // no valid frame within stale_timeout_us
    std::uint32_t frames_ok, frames_rejected, parity_errors, overruns, scale_error_codes;
    std::uint8_t last_error_code;        // e.g. 3 for EC,E03; 0xFF = none
};

class ScaleSource {
public:
    virtual ~ScaleSource() = default;
    [[nodiscard]] virtual const ScaleCapabilities& capabilities() const noexcept = 0;
    [[nodiscard]] virtual Status start(std::uint64_t now_us) noexcept = 0;   // begin stream
    virtual void poll(std::uint64_t now_us) noexcept = 0;                    // feed bytes, run timeouts
    [[nodiscard]] virtual bool latest(ScaleSample& out) const noexcept = 0;  // false until first valid frame
    [[nodiscard]] virtual Status request_tare(std::uint64_t now_us) noexcept = 0;
    [[nodiscard]] virtual Status request_zero(std::uint64_t now_us) noexcept = 0;
    [[nodiscard]] virtual CommandState command_state() const noexcept = 0;
    [[nodiscard]] virtual ScaleHealth health() const noexcept = 0;
};
}
```

- **Mass is an integer in µg.** Why: the frame is a decimal string, and integer parsing keeps 1 mg exact. The value is converted to the existing `double` mg types only at the measurement boundary. Grains use the exact 64.79891 mg/gr.
- **Tare/zero completion** isn't trusted from an ACK alone. The source confirms it by reading back |mass| ≤ `tare_confirm_band` with `stable` (research §(c)).
- **Stale data fails safe.** If no valid frame arrives within `stale_timeout_us`, `latest()` keeps the old sample but `health().stale` goes true. The planner then **stops motion** and treats the reading as invalid.

## 3. RS-232 port hardware (B2) (SCS-03)

| Item | Spec | Why / source |
|---|---|---|
| Connector | DE-9 **male**, #4-40 jackscrew posts | Our side is DTE, and DTE is male by convention. The A&D DE-9 is also male with #4-40 screws (§14 p.58) |
| Pinout (DTE) | 2 RXD in · 3 TXD out · 5 SG · 7 RTS out · 8 CTS in · 1, 4, 6, 9 NC | A&D pin table, §14 p.58: balance 2 TXD out, 3 RXD in, 6 DSR out, 7 RTS in, 8 CTS out. The A&D is DCE |
| Cable | **Straight-through, female–female, all 9 pins**. Check it with a DMM before first use | The manual calls for a straight-through cable to a DTE (§14, §15.2). Many F-F cables on sale are null-modem |
| Transceiver | MAX3232E-class at 3.3 V, 2 drivers / 2 receivers (TXD, RTS / RXD, CTS) | BOM audit §4: TI recommends the E variant for new designs; ±15 kV HBM and ±8 kV IEC contact on bus pins |
| Extra protection | Footprint for a bidirectional TVS (SM712 class) on pins 2/3/7/8, DNP at first | Meets the "ESD/TVS at every external connector" rule. Fit it if ESD testing calls for it |
| Direction swap | Solder jumpers swap pins 2↔3 and 7↔8 (default: straight) | Torbal, G&G and US Solid direction is undocumented (research (d)4). Also allows a null-modem cable |
| Signal levels | A&D DSR/CTS shown as "Approx. 5 V" (§14 figure) | Above the MAX3232 receiver threshold. No change needed |
| MCU side | USART2 TX/RX/RTS/CTS (`CONTROLLER_IO.md`) | RTS/CTS wired but **hardware flow control off** by default (A&D needs none) |

## 4. A&D FX-i / FZ-i driver (tier 1, SCS-01)

### 4.1 Required balance settings (user sets these once in the function table; HMI shows them)

| Class / item | Factory | Required | Why |
|---|---|---|---|
| `5if bp5` baud | **2 = 2400** | **5 = 19200** | At 20 frames/s the stream needs 17 B × 10 bit × 20 = 3400 bit/s, more than 2400. The manual warns that data drops at too low a baud (§10.5 caution, p.44–45). 19200 gives 18 % load and matches OpenTrickler |
| `5if btpr` bits/parity | **0 = 7 bits, even** | **2 = 8 bits, none** | Simplest UART setup. The driver also supports 7E1 so a factory-set scale can still be identified |
| `5if Crlf` terminator | 0 = CR LF | 0 | Parser accepts CR or CR LF either way |
| `5if type` format | 0 = A&D standard | 0 | Only format with an ST/US/OL header **and** fixed width (§10.6 p.45) |
| `5if erCd` AK/error | **0 = no output** | **1 = output** | Gives AK (06h) and `EC,Exx`. Without it, tare and zero have no reply |
| `5if t-Up` timeout | 0 = none | 0 | — |
| `dout prt` output mode | **0 = key mode** | 3 = stream, *or* leave it and the driver sends `SIR` | Key mode only transmits on the PRINT key. The driver sends `SIR` at start regardless (§16.1), so either works. Prefer `prt 3` [behavior with SIR to be confirmed on the unit] |
| `ba5fnc 5pd` refresh rate | **0 = 5/s** | **2 = 20/s** | The serial rate equals the display refresh rate (§14 p.58). MET-13 needs ≥ 10 Hz |
| Response (FAST/MID/SLOW) | — | FAST, which sets `Cond 0` + `5pd 2` (§7, p.27) | Shortest stabilization (about 1 s, research §2). Re-check if the bench is drafty |
| Unit | — | **g** | The driver rejects any other unit (`unsupported_unit`) instead of converting a rounded value |

### 4.2 Frame grammar (A&D standard format, §10.6 p.45 and §10.7 p.47)

The frame is 15 characters plus a terminator. Header and polarity are always present, and leading zeros are kept.

```
pos  1-2   3   4      5-12              13-15   16-17
     HH    ,   sign   DDDDD.DD (8 chars, contains '.')  UUU     CR LF
HH   = ST stable | US unstable | OL overload
sign = '+' or '-' ('+' when zero)
UUU  = unit, 3 chars, right-justified, space-padded ("  g")
```

Examples from the manual (spacing normalized): `ST,+00001.27  g`, `US,-00183.69  g`. A 1 mg balance gives three decimals, e.g. `ST,+0012.007  g` (§10.6 example). For overload the header is `OL` and the data field is not numeric (it prints as 9s with an exponent, and the extraction is garbled, §10.7). **The parser keys on the `OL` header only and never parses an OL data field.**

Parser rules:
1. Accumulate printable ASCII up to CR. Discard LF. Drop the line if it is over 32 characters, or contains any non-printable byte other than 06h.
2. Line `06h` alone = **AK**.
3. Line `EC,Exx` = scale error code `xx` (two digits). Codes: E00 communications, E01 undefined command, E02 not ready, E03 timeout, E04 excess characters, E06 format, E07 parameter (§16.2 p.63; §18.2 p.67–68).
4. Otherwise the line must be exactly 15 characters, with `,` at position 3 and the header in {ST, US, OL}. Otherwise it is rejected and `frames_rejected` counts it.
5. Data must match `[+-][0-9.]{8}` with exactly one `.`. It is parsed to µg using the decimal count, so 3 decimals is 1 mg.
6. A unit other than `g` → `Condition::unsupported_unit`. Dosing is inhibited and the HMI shows "set balance unit to g".
7. A UART framing, parity or overrun error during a line rejects that whole line.

### 4.3 Commands used (§16.1 p.62; terminator CR LF)

| Purpose | Command | Expected reply with `erCd 1` | Timeout (bring-up value, ASSUMED) |
|---|---|---|---|
| Identify at connect | `?TN` then `?SN` | model name / serial line | 500 ms each |
| Start stream | `SIR` (not needed if `prt 3`) | frames at the 5pd rate | first frame within 500 ms |
| Stop stream | `C` | stream stops | 300 ms |
| Tare | `T` | AK on receipt, then AK on completion (§16.2: R/Z/T send AK twice) | receipt 300 ms; completion 5 s |
| Zero | `Z` (= `R`) | as above | as above |
| Poll one reading | `Q` (= `SI`) | one frame | 300 ms |

Tare sequence:
1. Inhibit motion.
2. Send `T`.
3. Wait for the receipt AK, then the completion AK. An `EC,Exx` → `failed`.
4. Wait for ≥ N consecutive `ST` frames with |mass| ≤ `tare_confirm_band` (default 1 d).
5. Only then report `completed`.

The A&D link is half duplex (§14 p.58). If the bench unit garbles commands sent mid-stream, the driver sends `C` first and `SIR` afterwards. Test this on the unit (an open bring-up check).

Note that the timeout values above are protocol timing for first bring-up, not metrology. They live in the driver configuration record, and the bring-up log replaces them with measured values.

### 4.4 Rates and latency

| Quantity | Value | Source |
|---|---|---|
| Frame rate | 5 / 10 / 20 per s (= `5pd`) | §14 p.58 |
| Frame time at 19200 8N1 | 17 × 10 / 19200 = 8.9 ms | calculation |
| Stale timeout | 3 × frame period = 150 ms at 20 Hz | research §(c) |
| Scale filter latency | unknown; measure with the step test in §6 | — |

## 5. Other drivers (tiers 2–4; outline only, details come after a real trace from each unit)

| Tier | Driver | Frame / commands | Stability | Notes |
|---|---|---|---|---|
| 2 | Mettler MT-SICS | `S S <val> <unit>` / `S D` / `S +` / `S -`. Commands `SIR`, `UPD <rate>` (≤ 22.9/s on MS-S), `T` → `T S …`, `Z` → `Z A` | flag | Default 9600 8N1 XON/XOFF: turn handshake off. PB-S support for `UPD`/`T` is unverified |
| 3 | Shimadzu UW/UX | EB format, optional `S`/`U` prefix; `D01` stream about 12.5/s; `T`, `oL` overload | flag | Default 1200 baud, CR terminator; DB-25 on the balance needs an adapter cable |
| 4 | Torbal AD, Sartorius Entris II, G&G JJ | fixed-column, no flag | **derived** (`degraded`) | Torbal `S T`/`S Z` give no reply. Capture traces before writing |

## 6. Scale simulator (host tests, FW-03)

A host-side `SimulatedBalance` produces byte streams through the same `Uart` interface:

- **Plant model.** Pan mass = ∫ trickler flow, then the balance filter (first-order τ plus dead time) and 1 mg quantization at 5/10/20 Hz. The ST/US flag comes from a band-and-time rule set to imitate A&D FAST.
- **Fault injection:** dropped bytes, parity errors, a truncated line, a stale stream, `OL`, every `EC,Exx` code, a missing or late AK, a wrong unit, 7E1 vs 8N1 mismatch garbage, and a zero shift after tare.
- **Required host tests:**
  - Parse every manual example.
  - Reject the malformed variants.
  - Stale → motion stop.
  - Tare not completed without read-back.
  - `unsupported_unit` inhibits dosing.
  - Overload never yields a mass.
  - The sequence counter is monotonic.
  - Rate/latency budget at 5 Hz vs 20 Hz with the dosing state machine closed around the simulator.

## 7. Bring-up checks on a real A&D (adds to `HARDWARE_BRINGUP.md` in Phase 3)

1. Cable continuity: straight, 9 pins.
2. `?TN` reply at 19200 8N1.
3. Frame rate = 20.0 ± 0.5 /s over 60 s.
4. `T` gives two AKs.
5. Whether commands sent mid-stream are accepted (§4.3).
6. Step-response latency: drop a known mass, log time to the first `ST`.
7. Whether `SIR` and `prt 3` behave the same.
