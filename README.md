# esphome_gree_hvac

ESPHome component for GREE-based HVAC (air conditioners made by Zhuhai Gree Group Co., Ltd.)

This component provides integration for GREE-based air conditioners via UART protocol, allowing full control and monitoring through ESPHome and Home Assistant.

## Features

- ✅ Full climate control (mode, temperature, fan speed)
- ✅ Swing mode support (OFF, VERTICAL, HORIZONTAL, BOTH)
- ✅ Boost mode (Turbo) support for COOL and HEAT modes
- ✅ Current and target temperature monitoring
- ✅ Automatic state synchronization
- ✅ Robust error handling and packet validation
- ✅ Timeout protection for incomplete packets

## Compatible Devices

The component has been tested on the following units:

- **Kentatsu Turin** (KSGU26HZAN1/KSRU26HZAN1, KSGU35HZAN1/KSRU35HZAN1)
- **Lessar Enigma** (LS-HE12KDE2/LU-HE12KDE2)

> **Note:** This component may work with other GREE-based HVAC units. If you test it on a different model, please report your results.

## Installation

### Method 1: External Component (Recommended)

Add this to your `esphome.yaml`:

```yaml
external_components:
  - source: github://bekmansurov/esphome_gree_hvac
    components: [ gree ]
    refresh: 0s
```

### Method 2: Local Installation

1. Clone this repository:
```bash
git clone https://github.com/bekmansurov/esphome_gree_hvac.git
```

2. Copy the `components/gree` directory to your ESPHome `custom_components` folder.

## Configuration

### Basic Configuration

```yaml
uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 4800
  data_bits: 8
  parity: EVEN
  stop_bits: 1

climate:
  - platform: gree
    name: "Living Room AC"
```

### Advanced Configuration

```yaml
climate:
  - platform: gree
    name: "Kitchen AC"
    supported_presets:
      - NONE
      - BOOST
    update_interval: 10s
```

### Configuration Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Name of the climate device |
| `supported_presets` | list | No | `[NONE, BOOST]` | List of supported presets |
| `supported_swing_modes` | list | No | `[]` (all if not specified) | List of supported swing modes: `OFF`, `VERTICAL`, `HORIZONTAL`, `BOTH` |
| `update_interval` | time | No | `10s` | How often to poll the AC for state |

## Hardware Setup

### UART Connection

Connect the ESP device to your GREE HVAC unit:

- **TX Pin** → Connect to AC's RX line
- **RX Pin** → Connect to AC's TX line
- **GND** → Common ground
- **VCC** → Check your AC's requirements (usually 3.3V or 5V)

> **Warning:** Ensure proper voltage levels. Some AC units may require level shifters.

### Supported Platforms

- ESP32
- ESP8266
- ESP32-S2/S3
- Any ESPHome-compatible platform with UART support

## Examples

See the `examples/` directory for complete configuration examples:

- `iot-uni-dongle.yaml` - Example for IoT Uni Dongle board

## Troubleshooting

### No Response from AC

1. **Check UART settings:**
   - Verify baud rate is 4800
   - Verify parity is EVEN
   - Verify data bits is 8
   - Verify stop bits is 1

2. **Check wiring:**
   - Verify TX/RX are not swapped
   - Verify ground connection
   - Check for loose connections

3. **Enable verbose logging:**
```yaml
logger:
  level: VERBOSE
```

### Invalid Checksum Errors

- Check for electrical interference
- Verify UART connection quality
- Try reducing UART baud rate (if your AC supports it)
- Check for proper grounding

### Temperature Reading Issues

- Verify the AC is sending valid temperature data
- Check logs for temperature validation warnings
- Some AC models may report temperatures differently

## Development

### Building from Source

1. Ensure you have ESPHome installed
2. Clone this repository
3. Use as external component or copy to custom_components

### Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Protocol Information

This component communicates with GREE HVAC units using a proprietary UART protocol:

- **Baud Rate:** 4800
- **Data Format:** 8E1 (8 data bits, even parity, 1 stop bit)
- **Packet Format:** Start bytes (0x7E 0x7E) + Length + Data + CRC
- **Update Rate:** Configurable (default: 10 seconds)

## License

[Add your license here]

## Credits

- Original implementation by [bekmansurov](https://github.com/bekmansurov)
- Based on reverse engineering of GREE HVAC protocol

## Support

For issues, questions, or contributions, please use the [GitHub Issues](https://github.com/bekmansurov/esphome_gree_hvac/issues) page.