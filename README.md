# esphome_gree_hvac-extended

Extended fork of [bekmansurov/esphome_gree_hvac](https://github.com/bekmansurov/esphome_gree_hvac) — an ESPHome component for GREE-based HVAC units (air conditioners made by Zhuhai Gree Group Co., Ltd.), talking to the AC over its internal UART control link.

All credit for the original protocol reverse-engineering and base component goes to [@bekmansurov](https://github.com/bekmansurov). This fork adds control of AC features the upstream component didn't expose yet.

The component is tested on the following units:
- Kentatsu Turin (KSGU26HZAN1/KSRU26HZAN1, KSGU35HZAN1/KSRU35HZAN1)
- Lessar Enigma (LS-HE12KDE2/LU-HE12KDE2)
- Gree GWH12QB
- Gree GWH18QD

## What's new in this fork

Upstream exposed climate mode, target temperature, fan speed, and a boost preset. This fork adds proper ESPHome entities for AC features that exist in the protocol but weren't wired up:

- **Sleep** — `switch` entity
- **Display / light** — `switch` entity (the AC's own front-panel indicator display)
- **Turbo** — `switch` entity (a standalone toggle, independent of the climate preset)
- **Louver / swing position** — `select` entity with the 10 positions the unit actually supports: `Off`, `Full Swing`, `Top`, `Upper`, `Middle`, `Lower`, `Bottom`, `Lower Swing`, `Middle Swing`, `Upper Swing`

## Quick start

Point `external_components` at this repository — ESPHome pulls the component straight from GitHub, no manual download or `git clone` needed. Add this block (and the `climate:` section below it) to any working ESP32/ESP8266 YAML that already has `wifi:`, `api:`, `ota:`, and a `uart:` wired to the AC's control board:

```yaml
external_components:
  - source: github://iqdba/esphome_gree_hvac-extended
    components: [ gree ]
    refresh: 0s

uart:
  tx_pin: 1
  rx_pin: 3
  baud_rate: 4800
  data_bits: 8
  parity: EVEN
  stop_bits: 1

climate:
  - platform: gree
    name: "AC"
    sleep:
      name: "AC Sleep"
    display:
      name: "AC Display"
    turbo:
      name: "AC Turbo"
    louver:
      name: "AC Louver"
```

Run `esphome run <your-config>.yaml` — ESPHome fetches this component automatically on the first compile, and every entity above (climate + 3 switches + 1 select) shows up ready to use, no extra wiring in the YAML required.

For a complete, ready-to-flash device config, see:

- [`examples/d1-mini.yaml`](examples/d1-mini.yaml) — a cheap, common Wemos D1 Mini (ESP8266) wired directly to the AC's UART, with logging of the AC's own state. No extra hardware package needed.
- [`examples/iot-uni-dongle.yaml`](examples/iot-uni-dongle.yaml) — for the [dudanov IOT-UNI dongle](https://github.com/dudanov/esphome-packages) board.

Both need a `secrets.yaml` next to them — copy [`examples/secrets.yaml.example`](examples/secrets.yaml.example) to `examples/secrets.yaml` and fill in your own WiFi/OTA/AP values (never commit `secrets.yaml` — it's already in `.gitignore`).

## Why this fork exists

Built while bridging a set of Gree units into Apple HomeKit. The upstream component didn't expose display/sleep/turbo/louver control, so those were added here to reach full parity with the physical remote.

## Upstream

See the [original repository](https://github.com/bekmansurov/esphome_gree_hvac) for the base protocol implementation and additional supported unit reports.
