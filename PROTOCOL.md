# Gree AC UART protocol

Reverse-engineered byte-level description of the protocol these Gree mainboards speak on their internal service UART (the same connector documented in [`README.md`](README.md#wiring-observed-on-the-tested-units-above)). This is independent of ESPHome and this component's C++ — everything here is plain byte offsets and values, so you can implement a client for it in any language.

**This is reverse-engineered, not from official documentation.** It covers exactly what was needed to get full remote-equivalent control working on the tested units (see README) using an off-the-shelf sniffer between the mainboard and its own remote/receiver. Fields not listed below are genuinely unknown — see **Unknowns** at the end rather than assuming they're safe to ignore.

## Physical layer

- UART, **4800 baud, 8 data bits, even parity, 1 stop bit** (8E1).
- Half-duplex-style exchange over a single logical link: the mainboard periodically sends unsolicited status frames, and accepts command frames from the controller (ESP) at any time.

## Frame format

Every frame, in both directions, has this shape:

| Offset | Size | Field |
|---|---|---|
| 0-1 | 2 bytes | Start marker: always `0x7E 0x7E` |
| 2 | 1 byte | Data length — number of bytes following this length byte, **not including** the 2 start bytes |
| 3.. | variable | Data bytes (see below) |
| last byte | 1 byte | Checksum |

So a frame's total size on the wire is `3 + data_length`, and the checksum is always the very last byte of the frame.

### Checksum

```
checksum = (sum of all bytes from offset 2 up to, but not including, the checksum byte) mod 256
```

In other words: sum the length byte and every data byte (everything except the two `0x7E` start bytes and the checksum byte itself), then take mod 256. Recompute and compare on receipt; the tested implementation silently drops any frame that fails this check.

### Frame type

**Data byte 0 (absolute offset 3)** identifies the frame's purpose. Only two values are used by this implementation:

| Value | Meaning |
|---|---|
| `0x01` | Command frame (controller → AC) |
| `0x31` (49) | Status frame (AC → controller) |

Other values may exist (the mainboard likely uses more frame types for things this project never needed, e.g. pairing/config) — anything else is currently ignored.

## Command frame (controller → AC)

Fixed 47 bytes total (data length = `0x2C` / 44). Byte offsets below are **absolute from the start of the frame** (i.e. include the 2 start bytes + length byte).

| Offset | Field | Notes |
|---|---|---|
| 0-1 | `0x7E 0x7E` | Start marker |
| 2 | `0x2C` | Data length (44) |
| 3 | `0x01` | Frame type: command |
| 4-6 | `0x00 0x00 0x00` | Unused/reserved in this implementation |
| 7 | Force-update flag | `0xAF` (175) briefly, to make the AC reply with a fresh status frame right after processing this command; `0x00` normally. Set to `0xAF` for one command, then back to `0x00`. |
| 8 | **Mode/fan/sleep byte** | See below |
| 9 | **Target temperature** | `raw = (temp_celsius - 16) * 16`. Range 16-30°C in this implementation (only whole-degree steps are set — the low nibble is always 0, so a finer-grained sub-degree field may exist here that isn't used). |
| 10 | **Feature flags byte** | See below |
| 11 | Unknown, observed default `0x02` | Not read back from status frames, not know to affect behavior when left at this value. See **Unknowns**. |
| 12 | **Louver/swing position** | Raw enum value, see table below |
| 13 | `0x20` when sending any command | Empirically makes the AC briefly show the target temperature on its own front-panel display when a command arrives. Not set outside of `control()` calls in the tested implementation. |
| 14-45 | `0x00` | Unused/reserved in this implementation. One prior note in the source flags offset 41 as sometimes non-zero (`12`) on captured traffic with unknown meaning — this implementation always sends `0x00` there. |
| 46 | Checksum | See **Checksum** above |

### Mode/fan/sleep byte (offset 8)

| Bits | Field | Values |
|---|---|---|
| 7-4 (`0xF0`) | AC mode | `0x1` = Off, `0x8` = Auto, `0x9` = Cool, `0xA` = Dry, `0xB` = Fan only, `0xC` = Heat (i.e. byte values `0x10`/`0x80`/`0x90`/`0xA0`/`0xB0`/`0xC0`) |
| 1-0 (`0x03`) | Fan speed | `0` = Auto, `1` = Low, `2` = Medium, `3` = High |
| 3 (`0x08`) | Sleep | `1` = on, `0` = off |
| others | unused/unknown | not set by this implementation |

Dry mode only supports Low fan speed on the tested units — the fan speed field is forced to `1` whenever mode is Dry.

### Feature flags byte (offset 10)

| Bit | Field |
|---|---|
| 0 (`0x01`) | Turbo — `1` = on |
| 1 (`0x02`) | Display/front-panel light — `1` = on |
| others | unused/unknown |

### Louver/swing position (offset 12)

| Raw value | Position |
|---|---|
| `0x00` | Off |
| `0x10` | Full Swing |
| `0x20` | Top |
| `0x30` | Upper |
| `0x40` | Middle |
| `0x50` | Lower |
| `0x60` | Bottom |
| `0x70` | Lower Swing (middle↔bottom) |
| `0x90` | Middle Swing (above-middle↔below-middle) |
| `0xB0` | Upper Swing (middle↔top) |

`0x80` and `0xA0` are not used by any known position — worth investigating if you're extending this.

For a simple on/off swing toggle instead of the exact position, this implementation treats `Full Swing`/`Lower Swing`/`Middle Swing`/`Upper Swing` as "swing on" and everything else as "swing off".

## Status frame (AC → controller)

Sent unsolicited by the AC periodically, and always right after a command frame with the force-update flag set. Variable length; only the fields below are read by this implementation (all offsets absolute, same as above):

| Offset | Field | Decoding |
|---|---|---|
| 3 | Frame type | Must be `0x31` (49) to be treated as a status frame |
| 8 | Mode/fan/sleep byte | Same layout as the command frame |
| 9 | Target temperature | Same encoding as the command frame |
| 10 | Feature flags byte | Same layout as the command frame |
| 12 | Louver/swing position | Same encoding as the command frame |
| 46 | **Current (indoor) temperature** | `celsius = raw - 40` |
| last byte | Checksum | Same algorithm, computed over this frame's actual length |

Note offset 46 means something different in each direction: in a command frame it's always the checksum (since the command frame is fixed at 47 bytes), but a status frame can be a different length, so offset 46 lands on the indoor-temperature field instead and the checksum is wherever the frame actually ends.

### A well-behaved client should preserve unknown fields

Whenever a status frame arrives, this implementation copies the mode/fan/sleep byte, target temperature, feature flags byte, and louver byte straight back into its own outgoing command template before merging in whatever the user just asked to change. This matters because the physical remote can change any of these at any time — if your client doesn't track the AC's actual current state and blindly resends stale values, the next command you send will silently undo whatever the remote just did.

## Unknowns

Documented honestly rather than guessed at:

- **Offset 11** in the command frame: default `0x02` in this implementation, never read back from status frames, purpose unknown.
- **Offset 41**: sometimes observed non-zero (`12`) in real traffic captures; this implementation always sends `0x00` and hasn't identified what it controls.
- **Frame types other than `0x01`/`0x31`**: the mainboard almost certainly uses more frame types (pairing, extended sensor data, error codes, etc.) that this project never needed and hasn't decoded.
- **Sub-degree temperature bit**: the `raw/16` scaling on the temperature byte leaves 4 low bits unused by this implementation (always 0) — that range hints at possible half-degree or finer resolution support that hasn't been explored.

If you figure any of these out, contributions are very welcome.
