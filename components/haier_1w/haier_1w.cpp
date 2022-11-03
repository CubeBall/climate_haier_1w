#include "haier_1w.h"
#include "esphome/core/log.h"

namespace esphome {
namespace haier_1w {

static const char *const TAG = "Haier_1w_Climate";

#define R_HEADER 0x5B


#define R_CRC   18


#define S_HEADER 0x5A
#define S_CRC   11









const uint16_t SEND_BITS = 12 * 8; //!!! Depends on s_data_ size
const uint16_t RECEIVE_BITS = 19 * 8; //!!! Depends on r_data_ size


//******************************
const uint32_t COMMAND_ON = 0x00000;
const uint32_t COMMAND_ON_AI = 0x03000;
const uint32_t COMMAND_COOL = 0x08000;
const uint32_t COMMAND_HEAT = 0x0C000;
const uint32_t COMMAND_OFF = 0xC0000;
const uint32_t COMMAND_SWING = 0x10000;
// On, 25C, Mode: Auto, Fan: Auto, Zone Follow: Off, Sensor Temp: Ignore.
const uint32_t COMMAND_AUTO = 0x0B000;
const uint32_t COMMAND_DRY_FAN = 0x09000;

const uint32_t COMMAND_MASK = 0xFF000;

const uint32_t FAN_MASK = 0xF0;
const uint32_t FAN_AUTO = 0x50;
const uint32_t FAN_MIN = 0x00;
const uint32_t FAN_MED = 0x20;
const uint32_t FAN_MAX = 0x40;

// Temperature
const uint8_t TEMP_RANGE = TEMP_MAX - TEMP_MIN + 1;
const uint32_t TEMP_MASK = 0XF00;
const uint32_t TEMP_SHIFT = 8;


//******************************


// Climate Setup
climate::ClimateTraits Haier1w::traits() {
  auto traits = climate::ClimateTraits();
  // traits.set_supports_current_temperature(this->sensor_ != nullptr);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT_COOL});
  if (this->supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  if (this->supports_dry_)
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (this->supports_fan_only_)
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(this->minimum_temperature_);
  traits.set_visual_max_temperature(this->maximum_temperature_);
  traits.set_visual_temperature_step(this->temperature_step_);
  traits.set_supported_fan_modes(this->fan_modes_);
  traits.set_supported_swing_modes(this->swing_modes_);
  // traits.set_supported_presets(false);
  // traits.set_supported_presets(this->presets_);
  return traits;
}

// POLLING COMPONENT
void Haier1w::setup() {

}

void Haier1w::update() {

}

void Haier1w::loop() {

}

void Haier1w::dump_config() {
  LOG_CLIMATE("", "Haier 1w Climate", this);
  ESP_LOGCONFIG(TAG, "  Min. Temperature: %.1f°C", this->minimum_temperature_);
  ESP_LOGCONFIG(TAG, "  Max. Temperature: %.1f°C", this->maximum_temperature_);
  ESP_LOGCONFIG(TAG, "  Supports HEAT: %s", YESNO(this->supports_heat_));
  ESP_LOGCONFIG(TAG, "  Supports COOL: %s", YESNO(this->supports_cool_));
}

// CLIMATE PROTOCOL
// Translate Control command to transmit command
void Haier1w::control(const climate::ClimateCall &call) {
  send_swing_cmd_ = call.get_swing_mode().has_value();
  // swing resets after unit powered off
  if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_OFF)
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  if (call.get_mode().has_value())
    this->mode = *call.get_mode();
  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();
  if (call.get_swing_mode().has_value())
    this->swing_mode = *call.get_swing_mode();
  if (call.get_preset().has_value())
    this->preset = *call.get_preset();
  this->transmit_state();
  // this->publish_state(); //*** Publish only on AC Status receiving
}


// Transmit command
void Haier1w::transmit_state() {
  uint32_t remote_state = 0x8800000;
  //Clear Transmit buffer
  this->clear_data_(this->s_data_);
  // ESP_LOGD(TAG, "haier_1w mode_before_ code: 0x%02X", modeBefore_);
  // if (send_swing_cmd_) {
  //   send_swing_cmd_ = false;
  //   remote_state |= COMMAND_SWING;
  // } else {
  //   if (mode_before_ == climate::CLIMATE_MODE_OFF && this->mode == climate::CLIMATE_MODE_HEAT_COOL) {
  //     remote_state |= COMMAND_ON_AI;
  //   } else if (mode_before_ == climate::CLIMATE_MODE_OFF && this->mode != climate::CLIMATE_MODE_OFF) {
  //     remote_state |= COMMAND_ON;
  //     this->mode = climate::CLIMATE_MODE_COOL;
  //   } else {
      switch (this->mode) {
        case climate::CLIMATE_MODE_COOL:
          remote_state |= COMMAND_COOL;
          break;
        case climate::CLIMATE_MODE_HEAT:
          remote_state |= COMMAND_HEAT;
          break;
        case climate::CLIMATE_MODE_HEAT_COOL:
          remote_state |= COMMAND_AUTO;
          break;
        case climate::CLIMATE_MODE_DRY:
          remote_state |= COMMAND_DRY_FAN;
          break;
        case climate::CLIMATE_MODE_OFF:
        default:
          remote_state |= COMMAND_OFF;
          break;
      }
    // }
    // mode_before_ = this->mode;

    ESP_LOGD(TAG, "haier_1w mode code: 0x%02X", this->mode);

    if (this->mode == climate::CLIMATE_MODE_OFF) {
      remote_state |= FAN_AUTO;
    } else if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_DRY ||
               this->mode == climate::CLIMATE_MODE_HEAT) {
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_HIGH:
          remote_state |= FAN_MAX;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          remote_state |= FAN_MED;
          break;
        case climate::CLIMATE_FAN_LOW:
          remote_state |= FAN_MIN;
          break;
        case climate::CLIMATE_FAN_AUTO:
        default:
          remote_state |= FAN_AUTO;
          break;
      }
    }

    if (this->mode == climate::CLIMATE_MODE_HEAT_COOL) {
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      // remote_state |= FAN_MODE_AUTO_DRY;
    }
    if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT) {
      auto temp = (uint8_t) roundf(clamp<float>(this->target_temperature, TEMP_MIN, TEMP_MAX));
      remote_state |= ((temp - 15) << TEMP_SHIFT);
    }
  // }
  this->transmit_(this->s_data_); //Prepare and transmin
  // this->transmit_(remote_state); //Prepare and transmin
  // this->publish_state(); //*** Publish only on AC Status receiving
}


// Receiving data from AC
bool Haier1w::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t nbytes = 0;
  uint8_t nbits = 0;
  uint8_t allnbits = 0;
  uint32_t remote_state = 0;
  // If header correct
  if (!data.expect_item(this->receive_header_high_, this->receive_header_low_))
    return false;
  // Clear buffer
  this->clear_data_(this->r_data_);
  // Read bites to bytes of buffer
  for (nbytes=0; nbytes < sizeof(this->r_data_); nbytes++) {
    for (nbits = 0; nbits < 8; nbits++) {
      if (data.expect_item(this->bit_high_, this->bit_one_low_)) {
        this->r_data_[nbytes]= (this->r_data_[nbytes] << 1) | 1;
        allnbits+=1;
        // remote_state = (remote_state << 1) | 1;
      } else if (data.expect_item(this->bit_high_, this->bit_zero_low_)) {
        this->r_data_[nbytes]= (this->r_data_[nbytes] << 1) | 0;
        allnbits+=1;
        // remote_state = (remote_state << 1) | 0;
      } else if (allnbits == RECEIVE_BITS) {
        break;
      } else {
        return false;
      }
    }
  }
  ESP_LOGD(TAG, "Received message: %s", this->getHex_(this->r_data_));
  //Check CRC
  uint8_t crc=this->calc_checksum_(this->r_data_);
  if (crc!= this->r_data_[R_CRC]) {
    ESP_LOGW(TAG, "Invalid checksum received");
    return false;
  }
  //Check Header
  if (this->r_data_[0] != R_HEADER) {
    ESP_LOGW(TAG, "Invalid header received");
    return false;
  }
  //***********************
  if ((remote_state & COMMAND_MASK) == COMMAND_ON) {
    this->mode = climate::CLIMATE_MODE_COOL;
  } else if ((remote_state & COMMAND_MASK) == COMMAND_ON_AI) {
    this->mode = climate::CLIMATE_MODE_HEAT_COOL;
  }

  if ((remote_state & COMMAND_MASK) == COMMAND_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
  } else if ((remote_state & COMMAND_MASK) == COMMAND_SWING) {
    this->swing_mode =
        this->swing_mode == climate::CLIMATE_SWING_OFF ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;
  } else {
    if ((remote_state & COMMAND_MASK) == COMMAND_AUTO) {
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
    } else if ((remote_state & COMMAND_MASK) == COMMAND_DRY_FAN) {
      this->mode = climate::CLIMATE_MODE_DRY;
    } else if ((remote_state & COMMAND_MASK) == COMMAND_HEAT) {
      this->mode = climate::CLIMATE_MODE_HEAT;
    } else {
      this->mode = climate::CLIMATE_MODE_COOL;
    }

    // Temperature
    if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT)
      this->target_temperature = ((remote_state & TEMP_MASK) >> TEMP_SHIFT) + 15;

    // Fan Speed
    if (this->mode == climate::CLIMATE_MODE_HEAT_COOL) {
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
    } else if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT ||
               this->mode == climate::CLIMATE_MODE_DRY) {
      if ((remote_state & FAN_MASK) == FAN_AUTO) {
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
      } else if ((remote_state & FAN_MASK) == FAN_MIN) {
        this->fan_mode = climate::CLIMATE_FAN_LOW;
      } else if ((remote_state & FAN_MASK) == FAN_MED) {
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      } else if ((remote_state & FAN_MASK) == FAN_MAX) {
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
      }
    }
  }
  this->publish_state(); // Publish AC Status received

  return true;
}

// Prepare and transmit to AC
void Haier1w::transmit_(uint8_t * value) {
  uint8_t nbytes = 0;
  uint8_t nbits = 0;
  uint8_t allnbits = 0;
  // Calc CRC
  uint8_t crc_=calc_checksum_(value);
  value[S_CRC]=crc_;
  // Log
  ESP_LOGD(TAG, "Sending message: %s", this->getHex_(value));
  // Prepare transmit
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(0); // No Carrier
  data->reserve(2 + SEND_BITS * 2u);
  // Add Header
  data->item(this->send_header_high_, this->send_header_low_);
  // Add bits from Bytes
  for (nbytes=0; nbytes < sizeof(value); nbytes++){
    for (nbits = 0; nbits < 8; nbits++) {
      if (value[nbytes] & 1 << nbits) {
        data->item(this->bit_high_, this->bit_one_low_);
      } else {
        data->item(this->bit_high_, this->bit_zero_low_);
      }
    }
  }
  // Add mark bit
  data->mark(this->bit_high_);
  // Transmit
  transmit.perform();
}

// Clear buffer
void Haier1w::clear_data_(uint8_t *message) {
  uint8_t nbytes = 0;
  for(nbytes=0;nbytes < sizeof(message); nbytes++) {
    message[nbytes]=0x00;
  }
}

//CheckSum calc
uint8_t Haier1w::calc_checksum_(uint8_t * message) {
  uint8_t size = sizeof(message);
  uint8_t position = size - 1;
  uint8_t crc = 0;
  for (uint8_t i = 0; i < position; i++) {
      crc += message[i];
  }
  return crc;
}

// Message to HEX string
String Haier1w::getHex_(uint8_t * message) {
  String raw;
  uint8_t count=sizeof(message);
  for (uint8_t i=0; i < count; i++){
      sprintf(&(raw[i * 3]), "%02X", message[i]);
      if (i < count - 1) {
        sprintf(&(raw[i * 3 + 2]), ":");
      }
  }
  raw.toUpperCase();
  return raw;
}

}  // namespace haier_1w
}  // namespace esphome
