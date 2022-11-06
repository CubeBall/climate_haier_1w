#pragma once

#include <utility>

#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
// #include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace haier_1w {

// Temperature
const uint8_t TEMP_MIN = 16;  // Celsius
const uint8_t TEMP_MAX = 31;  // Celsius

// const uint32_t POLLING_INTERVAL=5000; //ms

class Haier1w : public climate::Climate, public PollingComponent, public remote_base::RemoteReceiverListener {
 public:
  Haier1w(float minimum_temperature, float maximum_temperature, float temperature_step = 1.0f,
            bool supports_dry = false, bool supports_fan_only = false, std::set<climate::ClimateFanMode> fan_modes = {},
            std::set<climate::ClimateSwingMode> swing_modes = {}, std::set<climate::ClimatePreset> presets = {}) {
    // this->minimum_temperature_ = minimum_temperature;
    // this->maximum_temperature_ = maximum_temperature;
    // this->temperature_step_ = temperature_step;
    // this->supports_dry_ = supports_dry;
    // this->supports_fan_only_ = supports_fan_only;
    // this->fan_modes_ = std::move(fan_modes);
    // this->swing_modes_ = std::move(swing_modes);
    // this->presets_ = std::move(presets);
    // this->update_interval_=update_interval;
  }

  //Polling component
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  //Climate part
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
    this->transmitter_ = transmitter;
  }
  void set_supports_cool(bool supports_cool) { this->supports_cool_ = supports_cool; }
  void set_supports_heat(bool supports_heat) { this->supports_heat_ = supports_heat; }
  // void set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
  
  //Protocol definitions
  void set_send_header_high(uint32_t send_header_high) { this->send_header_high_ = send_header_high; }
  void set_receive_header_high(uint32_t receive_header_high) { this->receive_header_high_ = receive_header_high; }
  void set_send_header_low(uint32_t send_header_low) { this->send_header_low_ = send_header_low; }
  void set_receive_header_low(uint32_t receive_header_low) { this->receive_header_low_ = receive_header_low; }
  void set_bit_high(uint32_t bit_high) { this->bit_high_ = bit_high; }
  void set_bit_one_low(uint32_t bit_one_low) { this->bit_one_low_ = bit_one_low; }
  void set_bit_zero_low(uint32_t bit_zero_low) { this->bit_zero_low_ = bit_zero_low; }
  
  protected:
  float minimum_temperature_, maximum_temperature_, temperature_step_;
  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override;
  /// Return the traits of this controller.
  climate::ClimateTraits traits() override;
  /// Transmit via IR the state of this climate controller.
  void transmit_state();

  bool on_receive(remote_base::RemoteReceiveData data) override;

  bool send_swing_cmd_{false};
  bool supports_cool_{true};
  bool supports_heat_{true};
  bool supports_dry_{false};
  bool supports_fan_only_{false};

  std::set<climate::ClimateFanMode> fan_modes_ = {};
  std::set<climate::ClimateSwingMode> swing_modes_ = {};
  std::set<climate::ClimatePreset> presets_ = {};

  remote_transmitter::RemoteTransmitterComponent *transmitter_;
  // sensor::Sensor *sensor_{nullptr};


  uint8_t calc_checksum_(uint8_t * message);
  void transmit_(uint8_t * value);
  void prepare_c_data_();
  void invert_r_data_();
  String getHex_(uint8_t * message);

  uint32_t send_header_high_;
  uint32_t receive_header_high_;
  uint32_t send_header_low_;
  uint32_t receive_header_low_;
  uint32_t bit_high_;
  uint32_t bit_one_low_;
  uint32_t bit_zero_low_;
  uint32_t update_interval_;
  
  //Protocol Data
  bool ready_for_command_;
  bool need_update_;
  uint8_t component_status_;
  uint8_t s_data_[12];
  uint8_t r_data_[19];
  uint8_t c_data_[12];

};

// #endif
}  // namespace haier_1w
}  // namespace esphome


// class Haier1w : public climate_ir::ClimateIR, public PollingComponent  {
// //class Haier1w : public climate_ir::ClimateIR {
//   public:
//   // Haier1w() :
//       climate_ir::ClimateIR(TEMP_MIN, TEMP_MAX, 1.0f, true, false,
//                               {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
//                                 climate::CLIMATE_FAN_HIGH},
//                               {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL})//, 
//       // PollingComponent({update_interval_}) {}
                              

//   /// Override control to change settings of the climate device.
//   // void control(const climate::ClimateCall &call) override {
//   //   send_swing_cmd_ = call.get_swing_mode().has_value();
//   //   // swing resets after unit powered off
//   //   if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_OFF)
//   //     this->swing_mode = climate::CLIMATE_SWING_OFF;
//   //   climate_ir::ClimateIR::control(call);
//   // }

//   //Climate part
  // void control(const climate::ClimateCall &call) override;

  // void set_header_high(uint32_t header_high) { this->header_high_ = header_high; }
  // void set_header_low(uint32_t header_low) { this->header_low_ = header_low; }
  // void set_bit_high(uint32_t bit_high) { this->bit_high_ = bit_high; }
  // void set_bit_one_low(uint32_t bit_one_low) { this->bit_one_low_ = bit_one_low; }
  // void set_bit_zero_low(uint32_t bit_zero_low) { this->bit_zero_low_ = bit_zero_low; }
  // void set_update_interval(uint32_t update_interval) { this->update_interval_ = update_interval; }
  
  //Polling Component parts
  // void setup() override;
  // void update() override;
  // void loop() override;
  // void dump_config() override;

//  protected:
  /// Transmit via IR the state of this climate controller.
  // void transmit_state() override;
  /// Handle received IR Buffer
//   bool on_receive(remote_base::RemoteReceiveData data) override;

//   bool send_swing_cmd_{false};

//   void calc_checksum_(uint32_t &value);
//   void transmit_(uint32_t value);

//   uint32_t header_high_;
//   uint32_t header_low_;
//   uint32_t bit_high_;
//   uint32_t bit_one_low_;
//   uint32_t bit_zero_low_;
//   uint32_t update_interval_;
//   climate::ClimateMode mode_before_{climate::CLIMATE_MODE_OFF};
// };

// #endif
// }  // namespace haier_1w
// }  // namespace esphome
