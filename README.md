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

## Quick start — fastest way to flash a unit

1. Download one file: [`examples/d1-mini.yaml`](examples/d1-mini.yaml). It's for a cheap, common ESP8266 board (Wemos D1 Mini) wired directly to the AC's UART — no other hardware package or extra file needed.
2. Open it and edit the `CHANGE_ME_...` values near the top (`substitutions:` block): your WiFi SSID/password, your MQTT broker/username/password, and the OTA/fallback-hotspot passwords. That block is the only thing you need to touch.
3. Set `location:` to whatever you want this unit called (it drives the device name and MQTT topic).
4. Flash it — see **Flashing** below.

ESPHome fetches this component straight from GitHub on the first compile via the `external_components:` block already in the file — no manual download or `git clone` of the component itself. Climate (mode, temperature, fan) plus the sleep/display/turbo switches and the louver select all come up ready to use in Home Assistant (via MQTT discovery) immediately after flashing.

## Flashing

Needs [ESPHome](https://esphome.io/guides/installing_esphome) installed (`pip install esphome`, or use the ESPHome add-on in Home Assistant instead of the commands below).

**First flash — over USB, one time only.** Plug the board into your computer, then:

```sh
esphome run d1-mini.yaml
```

ESPHome will ask you to pick a serial port — choose the one for your board (something like `/dev/cu.usbserial-XXXX` on Mac, `/dev/ttyUSB0` on Linux, `COM3` on Windows).

**Every update after that — over WiFi (OTA), no cable needed:**

```sh
esphome run d1-mini.yaml --device gree_ac_office.local
```

Replace `gree_ac_office` with `gree_ac_<location>`, using whatever you set `location:` to. If you don't pass `--device`, `esphome run` looks for a USB-connected board first and falls back to OTA over the network if it doesn't find one.

**Just want to watch the logs**, without reflashing:

```sh
esphome logs d1-mini.yaml
```

To add this component to your own existing YAML instead, just copy its `external_components:` block:

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

For the [dudanov IOT-UNI dongle](https://github.com/dudanov/esphome-packages) board instead of a plain D1 Mini, see [`examples/iot-uni-dongle.yaml`](examples/iot-uni-dongle.yaml) — that one keeps the original `!secret`-based style, so copy [`examples/secrets.yaml.example`](examples/secrets.yaml.example) to `examples/secrets.yaml` next to it and fill in your values there instead (never commit `secrets.yaml` — it's already in `.gitignore`).

## Why this fork exists

Built while bridging a set of Gree units into Apple HomeKit. The upstream component didn't expose display/sleep/turbo/louver control, so those were added here to reach full parity with the physical remote.

## Upstream

See the [original repository](https://github.com/bekmansurov/esphome_gree_hvac) for the base protocol implementation and additional supported unit reports.
