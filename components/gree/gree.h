#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gree {

enum ac_mode: uint8_t {
  AC_MODE_OFF = 0x10,
  // auto 0-1-2-3
  AC_MODE_AUTO = 0x80,
  // cool 0-1-2-3
  AC_MODE_COOL = 0x90,
  // dry 1 (only) but set it to AUTO (0) for setting it to 1 later
  AC_MODE_DRY = 0xA0,
  // fanonly 0-1-2-3
  AC_MODE_FANONLY = 0xB0,
  // heat 0-1-2-3
  AC_MODE_HEAT = 0xC0
};

enum ac_fan: uint8_t {
  AC_FAN_AUTO = 0x00,
  AC_FAN_LOW = 0x01,
  AC_FAN_MEDIUM = 0x02,
  AC_FAN_HIGH = 0x03
};

// Observed on gree_ac_office using the physical remote.
enum ac_louver: uint8_t {
  AC_LOUVERH_OFF = 0x00,
  AC_LOUVERH_SWING_FULL = 0x10,
  AC_LOUVERH_SWING_TOP = 0x20,
  AC_LOUVERH_SWING_ABOVEMIDDLE = 0x30,
  AC_LOUVERH_SWING_MIDDLE = 0x40,
  AC_LOUVERH_SWING_BELOWMIDDLE = 0x50,
  AC_LOUVERH_SWING_BOTTOM = 0x60,
  AC_LOUVERH_SWING_MIDDLE_TO_BOTTOM = 0x70,
  AC_LOUVERH_SWING_ABOVEMIDDLE_TO_BELOWMIDDLE = 0x90,
  AC_LOUVERH_SWING_MIDDLE_TO_TOP = 0xB0
};

#define GREE_START_BYTE 0x7E
#define GREE_RX_BUFFER_SIZE 52

union gree_start_bytes_t {
//     uint16_t u16;
    uint8_t u8x2[2];
};

struct gree_header_t
{
  gree_start_bytes_t start_bytes;
  uint8_t data_length;
};

struct gree_raw_packet_t
{
  gree_header_t header;
  uint8_t data[1]; // first data byte
};


/*
class Constants {
  public:
    // ac update interval in ms
    static const uint32_t AC_STATE_REQUEST_INTERVAL;
};
const uint32_t Constants::AC_STATE_REQUEST_INTERVAL = 300;
*/

class GreeFeatureSwitch;
class GreeLouverSelect;

class GreeClimate : public climate::Climate, public uart::UARTDevice, public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  void control(const climate::ClimateCall &call) override;
  void set_supported_presets(const std::set<climate::ClimatePreset> &presets) { this->supported_presets_ = presets; }

  void set_sleep_switch(GreeFeatureSwitch *sw) { this->sleep_switch_ = sw; }
  void set_display_switch(GreeFeatureSwitch *sw) { this->display_switch_ = sw; }
  void set_turbo_switch(GreeFeatureSwitch *sw) { this->turbo_switch_ = sw; }
  void set_louver_select(GreeLouverSelect *sel) { this->louver_select_ = sel; }

  void set_sleep(bool on);
  void set_display(bool on);
  void set_turbo(bool on);
  void set_louver(const std::string &value);

 protected:
  climate::ClimateTraits traits() override;
  void read_state_(const uint8_t *data, uint8_t size);
  void send_data_(const uint8_t *message, uint8_t size);
  void dump_message_(const char *title, const uint8_t *message, uint8_t size);
  uint8_t get_checksum_(const uint8_t *message, size_t size);
  void send_updated_state_();
  const char *louver_name_(uint8_t raw) const;

 private:
  // uint32_t _update_period = Constants::AC_STATE_REQUEST_INTERVAL;

  // Parts of the message that must have specific values for "send" command.
  // These are not 0x00 and the meaning of those values is unknown at the moment.
  // Others set to 0x00
  // data_write_[41] = 12; // unknown but not 0x00. TODO
  uint8_t data_write_[47] = {0x7E, 0x7E, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t data_read_[GREE_RX_BUFFER_SIZE] = {0};

  bool receiving_packet_ = false;

  std::set<climate::ClimatePreset> supported_presets_{};
  GreeFeatureSwitch *sleep_switch_{nullptr};
  GreeFeatureSwitch *display_switch_{nullptr};
  GreeFeatureSwitch *turbo_switch_{nullptr};
  GreeLouverSelect *louver_select_{nullptr};

  bool sleep_{false};
  bool display_{true};
  bool turbo_{false};
  uint8_t louver_{AC_LOUVERH_OFF};
};

enum GreeFeature : uint8_t { SLEEP, DISPLAY_LIGHT, TURBO };

class GreeFeatureSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(GreeClimate *parent) { this->parent_ = parent; }
  void set_feature(GreeFeature feature) { this->feature_ = feature; }

 protected:
  void write_state(bool state) override;

 private:
  GreeClimate *parent_{nullptr};
  GreeFeature feature_{GreeFeature::SLEEP};
};

class GreeLouverSelect : public select::Select, public Component {
 public:
  void set_parent(GreeClimate *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;

 private:
  GreeClimate *parent_{nullptr};
};

}  // namespace gree
}  // namespace esphome
