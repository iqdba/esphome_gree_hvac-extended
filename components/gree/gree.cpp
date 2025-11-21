#include "gree.h"
#include "esphome/core/macros.h"

namespace esphome {
namespace gree {

static const char *const TAG = "gree";

// Bit masks for mode and fan extraction
static const uint8_t MODE_MASK = 0b11110000;
static const uint8_t FAN_MASK = 0b00001111;

// component settings
static const uint8_t MIN_VALID_TEMPERATURE = 16;
static const uint8_t MAX_VALID_TEMPERATURE = 30;
static const uint8_t TEMPERATURE_STEP = 1;

// prints user configuration
void GreeClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "Gree:");
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  ESP_LOGCONFIG(TAG, "  Packet timeout: %u ms", GREE_PACKET_TIMEOUT_MS);
  this->dump_traits_(TAG);
  this->check_uart_settings(4800, 1, uart::UART_CONFIG_PARITY_EVEN, 8);
}

void GreeClimate::loop() {
  gree_raw_packet_t *raw_packet = (gree_raw_packet_t *)this->data_read_;
  uint32_t now = millis();

  // Check for packet reception timeout
  if (receiving_packet_ && (now - last_packet_byte_time_ > GREE_PACKET_TIMEOUT_MS)) {
    ESP_LOGW(TAG, "Packet reception timeout, resetting state");
    timeout_errors_++;
    receiving_packet_ = false;
    memset(this->data_read_, 0, GREE_RX_BUFFER_SIZE);
  }

  while (!receiving_packet_ && this->available() >= sizeof(gree_header_t)) {
    if (this->peek() != GREE_START_BYTE) {
      this->read(); // discard invalid byte
      continue;
    }

    this->read_array(this->data_read_, sizeof(gree_start_bytes_t));
    receiving_packet_ = (raw_packet->header.start_bytes.u8x2[1] == GREE_START_BYTE);
    last_packet_byte_time_ = now;
    
    if (receiving_packet_) {
      this->read_byte(&raw_packet->header.data_length);

      if (raw_packet->header.data_length == 0) {
        ESP_LOGW(TAG, "Invalid packet: data_length is 0");
        receiving_packet_ = false;
        memset(this->data_read_, 0, GREE_RX_BUFFER_SIZE);
        continue;
      }

      if (raw_packet->header.data_length + sizeof(gree_header_t) > GREE_RX_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Incoming packet is too big! header.data_length = %u, maximum is %u", 
                 raw_packet->header.data_length, GREE_RX_BUFFER_SIZE - sizeof(gree_header_t));
        receiving_packet_ = false;
        memset(this->data_read_, 0, GREE_RX_BUFFER_SIZE);
      }
    }
  }

  if (receiving_packet_ && this->available() >= raw_packet->header.data_length) {
    this->read_array(raw_packet->data, raw_packet->header.data_length);
    last_packet_byte_time_ = millis();

    uint8_t total_size = raw_packet->header.data_length + sizeof(gree_header_t);
    dump_message_("Read array", this->data_read_, total_size);
    read_state_(this->data_read_, total_size);
    
    packets_received_++;
    receiving_packet_ = false;
    memset(this->data_read_, 0, GREE_RX_BUFFER_SIZE);
  }
}


void GreeClimate::update() {
  data_write_[gree_packet::POS_CRC_WRITE] = get_checksum_(data_write_, sizeof(data_write_));
  send_data_(data_write_, sizeof(data_write_));
}

climate::ClimateTraits GreeClimate::traits() {
  auto traits = climate::ClimateTraits();

  traits.set_visual_min_temperature(MIN_VALID_TEMPERATURE);
  traits.set_visual_max_temperature(MAX_VALID_TEMPERATURE);
  traits.set_visual_temperature_step(TEMPERATURE_STEP);

  traits.set_supported_modes({
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_AUTO,
    climate::CLIMATE_MODE_COOL,
    climate::CLIMATE_MODE_DRY,
    climate::CLIMATE_MODE_FAN_ONLY,
    climate::CLIMATE_MODE_HEAT
  });

  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH
  });

  traits.set_supported_swing_modes(this->supported_swing_modes_);
  traits.set_supports_current_temperature(true);
  traits.set_supports_two_point_target_temperature(false);

  traits.set_supported_presets(this->supported_presets_);

  traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  traits.add_supported_preset(climate::CLIMATE_PRESET_BOOST);
  // traits.add_supported_preset(climate::CLIMATE_PRESET_SLEEP);

  return traits;
}

void GreeClimate::read_state_(const uint8_t *data, uint8_t size) {
  // Validate minimum packet size
  constexpr uint8_t MIN_PACKET_SIZE = sizeof(gree_header_t) + 1; // header + at least 1 data byte + CRC
  if (size < MIN_PACKET_SIZE + 1) {
    ESP_LOGW(TAG, "Packet too small: size = %u, minimum = %u", size, MIN_PACKET_SIZE + 1);
    return;
  }

  // Get checksum byte from received data (using the last byte)
  uint8_t data_crc = data[size - 1];
  // Get checksum byte based on received data (calculating)
  uint8_t calculated_crc = get_checksum_(data, size);

  if (data_crc != calculated_crc) {
    checksum_errors_++;
    ESP_LOGW(TAG, "Invalid checksum. Received: 0x%02X, Calculated: 0x%02X (errors: %u)", 
             data_crc, calculated_crc, checksum_errors_);
    return;
  }

  // Validate packet type (0x31 as first data byte)
  if (size <= gree_packet::POS_PACKET_TYPE || data[gree_packet::POS_PACKET_TYPE] != gree_packet::PACKET_TYPE_STATE) {
    invalid_packet_errors_++;
    ESP_LOGW(TAG, "Invalid packet type. Expected: 0x%02X, Got: 0x%02X (errors: %u)", 
             gree_packet::PACKET_TYPE_STATE, 
             size > gree_packet::POS_PACKET_TYPE ? data[gree_packet::POS_PACKET_TYPE] : 0,
             invalid_packet_errors_);
    return;
  }

  // Validate array bounds before accessing temperature fields
  if (size <= gree_packet::POS_TEMPERATURE) {
    ESP_LOGW(TAG, "Packet too small to contain temperature data");
    return;
  }

  // Extract and validate target temperature
  uint8_t temp_raw = data[gree_packet::POS_TEMPERATURE];
  float target_temp = (temp_raw / 16.0f) + MIN_VALID_TEMPERATURE;
  if (target_temp >= MIN_VALID_TEMPERATURE && target_temp <= MAX_VALID_TEMPERATURE) {
    this->target_temperature = target_temp;
  } else {
    ESP_LOGW(TAG, "Invalid target temperature: %.1f (raw: 0x%02X)", target_temp, temp_raw);
  }

  // Extract and validate current temperature
  if (size > gree_packet::POS_INDOOR_TEMPERATURE) {
    int8_t current_temp_raw = static_cast<int8_t>(data[gree_packet::POS_INDOOR_TEMPERATURE]);
    float current_temp = current_temp_raw - 40.0f;
    // Reasonable temperature range: -10 to 50°C
    if (current_temp >= -10.0f && current_temp <= 50.0f) {
      this->current_temperature = current_temp;
    } else {
      ESP_LOGW(TAG, "Invalid current temperature: %.1f (raw: 0x%02X)", current_temp, 
               static_cast<uint8_t>(current_temp_raw));
    }
  }

  // Validate array bounds before accessing mode field
  if (size <= gree_packet::POS_MODE) {
    ESP_LOGW(TAG, "Packet too small to contain mode data");
    return;
  }

  // Partially saving current state to previous request
  data_write_[gree_packet::POS_MODE] = data[gree_packet::POS_MODE];
  data_write_[gree_packet::POS_TEMPERATURE] = data[gree_packet::POS_TEMPERATURE];

  // Update CLIMATE state according AC response
  uint8_t mode_byte = data[gree_packet::POS_MODE];
  switch (mode_byte & MODE_MASK) {
    case AC_MODE_OFF:
      this->mode = climate::CLIMATE_MODE_OFF;
      break;
    case AC_MODE_AUTO:
      this->mode = climate::CLIMATE_MODE_AUTO;
      break;
    case AC_MODE_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      break;
    case AC_MODE_DRY:
      this->mode = climate::CLIMATE_MODE_DRY;
      break;
    case AC_MODE_FANONLY:
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
      break;
    case AC_MODE_HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;
    default:
      ESP_LOGW(TAG, "Unknown AC MODE: 0x%02X", mode_byte & MODE_MASK);
  }

  // Get current AC FAN SPEED from its response
  switch (mode_byte & FAN_MASK) {
    case AC_FAN_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    case AC_FAN_LOW:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case AC_FAN_MEDIUM:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case AC_FAN_HIGH:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    default:
      ESP_LOGW(TAG, "Unknown AC FAN: 0x%02X", mode_byte & FAN_MASK);
  }

  // Parse preset (boost mode)
  if (size > gree_packet::POS_PRESET) {
    uint8_t preset_byte = data[gree_packet::POS_PRESET];
    switch (preset_byte) {
      case gree_packet::PRESET_COOL_BOOST:
      case gree_packet::PRESET_HEAT_BOOST:
        this->preset = climate::CLIMATE_PRESET_BOOST;
        break;
      case gree_packet::PRESET_COOL_NORMAL:
      case gree_packet::PRESET_HEAT_NORMAL:
      default:
        this->preset = climate::CLIMATE_PRESET_NONE;
        break;
    }
  } else {
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  // Parse swing mode
  if (size > gree_packet::POS_SWING) {
    uint8_t swing_byte = data[gree_packet::POS_SWING];
    switch (swing_byte) {
      case AC_SWING_OFF:
        this->swing_mode = climate::CLIMATE_SWING_OFF;
        break;
      case AC_SWING_VERTICAL:
        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
        break;
      case AC_SWING_HORIZONTAL:
        this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
        break;
      case AC_SWING_BOTH:
        this->swing_mode = climate::CLIMATE_SWING_BOTH;
        break;
      default:
        ESP_LOGW(TAG, "Unknown swing mode: 0x%02X", swing_byte);
        // Keep current swing mode if unknown value received
        break;
    }
    // Save swing mode to write buffer for next command
    data_write_[gree_packet::POS_SWING] = swing_byte;
  }

  this->publish_state();
}

void GreeClimate::control(const climate::ClimateCall &call) {
  data_write_[gree_packet::POS_FORCE_UPDATE] = gree_packet::FORCE_UPDATE_VALUE;
  // Show current temperature on display every time when sending new command
  data_write_[gree_packet::POS_DISPLAY] = gree_packet::DISPLAY_SHOW_TEMP;

  // Saving mode&fan values from previous state
  uint8_t new_mode = data_write_[gree_packet::POS_MODE] & MODE_MASK;
  uint8_t new_fan_speed = data_write_[gree_packet::POS_MODE] & FAN_MASK;

  if (call.get_mode().has_value()) {
    switch (call.get_mode().value()) {
      case climate::CLIMATE_MODE_OFF:
        new_mode = AC_MODE_OFF;
        break;
      case climate::CLIMATE_MODE_AUTO:
        new_mode = AC_MODE_AUTO;
        break;
      case climate::CLIMATE_MODE_COOL:
        new_mode = AC_MODE_COOL;
        break;
      case climate::CLIMATE_MODE_DRY:
        new_mode = AC_MODE_DRY;
        new_fan_speed = AC_FAN_LOW;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        new_mode = AC_MODE_FANONLY;
        break;
      case climate::CLIMATE_MODE_HEAT:
        new_mode = AC_MODE_HEAT;
        break;
      default:
        ESP_LOGW(TAG, "Setting of unsupported MODE: %d", static_cast<int>(call.get_mode().value()));
        break;
    }
  }

  // set fan speed only if MODE != DRY (only LOW available)
  if (call.get_fan_mode().has_value()) {
    switch (call.get_fan_mode().value()) {
      case climate::CLIMATE_FAN_AUTO:
        new_fan_speed = AC_FAN_AUTO;
        break;
      case climate::CLIMATE_FAN_LOW:
        new_fan_speed = AC_FAN_LOW;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        new_fan_speed = AC_FAN_MEDIUM;
        break;
      case climate::CLIMATE_FAN_HIGH:
        new_fan_speed = AC_FAN_HIGH;
        break;
      default:
        ESP_LOGW(TAG, "Setting of unsupported FANSPEED: %d", static_cast<int>(call.get_fan_mode().value()));
        break;
    }
  }
  
  // Set low speed when DRY mode because other speeds are not available
  if (new_mode == AC_MODE_DRY && new_fan_speed != AC_FAN_LOW) {
    new_fan_speed = AC_FAN_LOW;
  }

  if (call.get_preset().has_value()) {
    switch (call.get_preset().value()) {
      case climate::CLIMATE_PRESET_NONE:
        if (new_mode == AC_MODE_COOL) {
          data_write_[gree_packet::POS_PRESET] = gree_packet::PRESET_COOL_NORMAL;
        } else if (new_mode == AC_MODE_HEAT) {
          data_write_[gree_packet::POS_PRESET] = gree_packet::PRESET_HEAT_NORMAL;
        }
        break;
      case climate::CLIMATE_PRESET_BOOST:
        if (new_mode == AC_MODE_COOL) {
          data_write_[gree_packet::POS_PRESET] = gree_packet::PRESET_COOL_BOOST;
        } else if (new_mode == AC_MODE_HEAT) {
          data_write_[gree_packet::POS_PRESET] = gree_packet::PRESET_HEAT_BOOST;
        }
        // Skip preset when not COOL or HEAT mode
        break;
      default:
        break;
    }
  }

  if (call.get_target_temperature().has_value()) {
    float target_temp = call.get_target_temperature().value();
    // Check if temperature set in valid limits
    if (target_temp >= MIN_VALID_TEMPERATURE && target_temp <= MAX_VALID_TEMPERATURE) {
      data_write_[gree_packet::POS_TEMPERATURE] = 
        static_cast<uint8_t>((target_temp - MIN_VALID_TEMPERATURE) * 16);
    } else {
      ESP_LOGW(TAG, "Target temperature out of range: %.1f (valid: %u-%u)", 
               target_temp, MIN_VALID_TEMPERATURE, MAX_VALID_TEMPERATURE);
    }
  }

  // Handle swing mode
  if (call.get_swing_mode().has_value()) {
    switch (call.get_swing_mode().value()) {
      case climate::CLIMATE_SWING_OFF:
        data_write_[gree_packet::POS_SWING] = AC_SWING_OFF;
        break;
      case climate::CLIMATE_SWING_VERTICAL:
        data_write_[gree_packet::POS_SWING] = AC_SWING_VERTICAL;
        break;
      case climate::CLIMATE_SWING_HORIZONTAL:
        data_write_[gree_packet::POS_SWING] = AC_SWING_HORIZONTAL;
        break;
      case climate::CLIMATE_SWING_BOTH:
        data_write_[gree_packet::POS_SWING] = AC_SWING_BOTH;
        break;
      default:
        ESP_LOGW(TAG, "Setting of unsupported SWING mode: %d", 
                 static_cast<int>(call.get_swing_mode().value()));
        break;
    }
  }

  data_write_[gree_packet::POS_MODE] = new_mode | new_fan_speed;

  // Compute checksum & send data
  data_write_[gree_packet::POS_CRC_WRITE] = get_checksum_(data_write_, sizeof(data_write_));
  send_data_(data_write_, sizeof(data_write_));

  // Change force_update byte to "passive" state
  data_write_[gree_packet::POS_FORCE_UPDATE] = 0;
}

void GreeClimate::send_data_(const uint8_t *message, uint8_t size) {
  this->write_array(message, size);
  packets_sent_++;
  dump_message_("Sent message", message, size);
}

void GreeClimate::dump_message_(const char *title, const uint8_t *message, uint8_t size) {
  ESP_LOGV(TAG, "%s:", title);
  constexpr size_t MAX_STR_SIZE = 250;
  char str[MAX_STR_SIZE] = {0};
  
  // Each byte needs 3 chars (2 hex digits + space), plus null terminator
  if (size * 3 > MAX_STR_SIZE - 1) {
    ESP_LOGE(TAG, "Message too long to dump: %u bytes (max: %u)", size, (MAX_STR_SIZE - 1) / 3);
    return;
  }
  
  char *pstr = str;
  for (uint8_t i = 0; i < size; i++) {
    pstr += sprintf(pstr, "%02X ", message[i]);
  }
  ESP_LOGV(TAG, "%s", str);
}

uint8_t GreeClimate::get_checksum_(const uint8_t *message, size_t size) {
  // position of crc in packet
  uint8_t position = size - 1;
  uint8_t sum = 0;
  // ignore first 2 bytes & last one
  for (int i = 2; i < position; i++)
    sum += message[i];
  uint8_t crc = sum % 256;
  return crc;
}

}  // namespace gree
}  // namespace esphome
