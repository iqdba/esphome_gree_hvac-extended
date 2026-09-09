# MQTT topics

Matches the `tp` substitution in [`examples/d1-mini.yaml`](examples/d1-mini.yaml):

`home/ac/<location>`

Replace `<location>` with whatever you set `location:` to in the YAML (e.g. `office`, `bedroom`, `living_room`) and `home/ac` with your own `tp` prefix if you changed it.

## Availability

| Topic | Direction | Payload |
|---|---|---|
| `home/ac/<location>/status` | read | `online` or `offline` |

## Climate

| Function | State topic | Command topic | Command payload |
|---|---|---|---|
| Mode | `.../climate/gree_ac/mode/state` | `.../climate/gree_ac/mode/command` | `off`, `auto`, `cool`, `dry`, `fan_only`, `heat` |
| Current temperature | `.../climate/gree_ac/current_temperature/state` | — | number in °C |
| Target temperature | `.../climate/gree_ac/target_temperature/state` | `.../climate/gree_ac/target_temperature/command` | `16` through `30` |
| Fan speed | `.../climate/gree_ac/fan_mode/state` | `.../climate/gree_ac/fan_mode/command` | `auto`, `low`, `medium`, `high` |
| Swing | `.../climate/gree_ac/swing_mode/state` | `.../climate/gree_ac/swing_mode/command` | `off`, `vertical` |
| Preset / turbo | `.../climate/gree_ac/preset/state` | `.../climate/gree_ac/preset/command` | `none`, `boost` |

In this table, `...` means `home/ac/<location>`.

Example — set the office unit to 24 °C:

- topic: `home/ac/office/climate/gree_ac/target_temperature/command`
- payload: `24`

## Feature switches

The entity object ID includes the location.

| Function | State topic | Command topic | Command payload |
|---|---|---|---|
| Sleep | `.../switch/gree_ac_<location>_sleep/state` | `.../switch/gree_ac_<location>_sleep/command` | `ON` or `OFF` |
| Display light | `.../switch/gree_ac_<location>_display/state` | `.../switch/gree_ac_<location>_display/command` | `ON` or `OFF` |
| Turbo | `.../switch/gree_ac_<location>_turbo/state` | `.../switch/gree_ac_<location>_turbo/command` | `ON` or `OFF` |

## Exact louver position

| State topic | Command topic |
|---|---|
| `.../select/gree_ac_<location>_louver/state` | `.../select/gree_ac_<location>_louver/command` |

Allowed command payloads (spelling and capitalization must match):

- `Off`
- `Full Swing`
- `Top`
- `Upper`
- `Middle`
- `Lower`
- `Bottom`
- `Lower Swing`
- `Middle Swing`
- `Upper Swing`

Home Assistant MQTT discovery is also enabled under `homeassistant/...`. Those discovery topics are generated automatically and should not be used for direct control.
