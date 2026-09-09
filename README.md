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

## Example

```yaml
climate:
  - platform: gree
    name: None
    sleep:
      name: "AC Sleep"
    display:
      name: "AC Display"
    turbo:
      name: "AC Turbo"
    louver:
      name: "AC Louver"
```

See `examples/iot-uni-dongle.yaml` for a full device config.

## Why this fork exists

Built while bridging a set of Gree units into Apple HomeKit. The upstream component didn't expose display/sleep/turbo/louver control, so those were added here to reach full parity with the physical remote.

## Upstream

See the [original repository](https://github.com/bekmansurov/esphome_gree_hvac) for the base protocol implementation and additional supported unit reports.
