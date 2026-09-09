# Instructions for AI agents

If a human pointed you at this repository and asked you to help them get their Gree AC talking to WiFi/MQTT/Home Assistant, read this file first. It tells you what's here, what order to read things in, and what NOT to do without asking.

## What this repo actually is

An ESPHome external component for Gree-based air conditioners, controlled over the AC's own internal UART service port (not IR). Fork of [bekmansurov/esphome_gree_hvac](https://github.com/bekmansurov/esphome_gree_hvac), adding sleep/display/turbo switches and an exact-position louver select that upstream didn't have.

There is no separate "build system" beyond ESPHome itself — the component is Python (config schema) + C++ (`components/gree/`), compiled by ESPHome's own toolchain when the user runs `esphome run`/`esphome compile`. You are not building a HomeKit bridge, a server, or anything else here — just flashing one small ESP8266/ESP32 board per AC unit.

## Read in this order

1. **[`README.md`](README.md)** — full human-facing guide: safety warning, wiring (with photo), what-you-need, quick start, flashing commands, troubleshooting, phone-control options (IoT MQTT Panel / Home Assistant). This is the primary source of truth; don't restate it from memory, actually read it.
2. **[`examples/d1-mini.yaml`](examples/d1-mini.yaml)** — the one file a user needs to get a working unit. Self-contained: every credential is a `CHANGE_ME_...` placeholder in the `substitutions:` block at the top.
3. **[`PROTOCOL.md`](PROTOCOL.md)** — only needed if you're touching `components/gree/gree.cpp` itself (new features, bug fixes) rather than just deploying the existing component. Byte-level protocol reference, independent of the code.
4. **[`MQTT_TOPICS.md`](MQTT_TOPICS.md)** — only needed if the user wants a phone dashboard (IoT MQTT Panel) or a Home Assistant automation referencing raw topics instead of its auto-discovered entities.

## The fast path to a working unit (what the user actually wants)

1. Confirm the user has: an ESP8266/ESP32 board (D1 Mini assumed unless they say otherwise), a USB cable, physical access to the AC's `CHU4`/"Wi-Fi"-labeled connector (see README **Wiring**), and their WiFi + (optionally) MQTT broker details.
2. Get [`examples/d1-mini.yaml`](examples/d1-mini.yaml) onto their machine. Fill in the `CHANGE_ME_...` substitutions with real values they give you — never invent placeholder-looking real credentials, and never ask them to paste real passwords into this chat if you can help it; have them edit the file directly.
3. Confirm `esphome` is installed (`pip install esphome` or the HA add-on).
4. First flash is always over USB: `esphome run d1-mini.yaml`. Every flash after that can be OTA: `--device gree_ac_<location>.local`.
5. If it doesn't work, go to README **Troubleshooting** before improvising — it already covers the likely failures (wrong USB driver, TX/RX swapped, mDNS blocked, MQTT discovery prefix mismatch).

Don't over-engineer this. The user wants one AC talking to their network within a couple of hours, not a rewrite.

## Hard constraints — do not cross these without the user explicitly asking

- **Never invent wiring/pinout details you don't have.** The `CHU4` table in the README is from real measurement on specific tested units; wire colors are not a universal standard. If a user's board doesn't match, say so and ask them to verify continuity themselves — don't guess a pinout to sound helpful.
- **Never suggest or perform work with the AC powered on**, or downplay the mains-voltage warning in the README's Safety section.
- **Never put real credentials (WiFi, MQTT, OTA passwords) into anything that gets committed or published** — not in this repo, not in a gist, not in a message you post anywhere public. If you're helping edit `d1-mini.yaml`, that edited copy is the user's local file, not something to commit back here.
- **Don't touch `components/gree/gree.cpp`'s command-frame preservation logic** (copying mode/temp/flags/louver bytes back from the last status frame before merging a new command) unless you understand why it's there — removing it makes the physical remote's changes get silently undone by the next API command. See PROTOCOL.md's "well-behaved client" note.
- **Don't re-enable `BuildTemperatureAccessory`-style patterns or similar "helpful" additions to a HomeKit bridge that might be using this component** — that's a different repository's concern, but if you find yourself in one, check for the same kind of "known bad idea, kept as a marker" comment before assuming unused code is a bug to fix.

## If something is unclear or missing

Say so plainly rather than guessing — this hardware talks to a live appliance near mains voltage, and a wrong guess here isn't just a bug, it can damage the unit or hurt someone. If the README/PROTOCOL.md genuinely doesn't cover what you need, tell the user that's a gap rather than fabricating an answer to sound complete.
