#include "haier_1w.h"
#include "esphome/core/log.h"

namespace esphome {
namespace haier_1w {

static const char *const TAG = "Haier_1w_Climate";


#define _HEADER    0
#define _TEMP_POWER  1
#define TARGET_TEMP_MASK 0xF0
#define TARGET_TEMP_SHIFT 4
#define TARGET_TEMP_ADD 16
#define BIT_POWER_ON 0






#define R_HEADER  0x5B

#define CURRENT_TEMP 9
#define CURRENT_TEMP_MASK 0x01F

#define R_CRC   18


#define S_HEADER 0x5A
#define S_CRC   11




// CONTROL MODE for component_status_
#define READY 1
#define SENDING 2
#define WAITING 3
#define RECIIVING 4




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
  traits.set_supports_current_temperature(true);
  // // traits.set_supports_current_temperature(this->sensor_ != nullptr);
  // traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT_COOL});
  // if (this->supports_cool_)
  //   traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  // if (this->supports_heat_)
  //   traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  // if (this->supports_dry_)
  //   traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  // if (this->supports_fan_only_)
  //   traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  //***
  traits.set_supported_modes(
{
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_HEAT_COOL, //
    climate::CLIMATE_MODE_COOL,
    climate::CLIMATE_MODE_HEAT,
    climate::CLIMATE_MODE_FAN_ONLY,
    climate::CLIMATE_MODE_DRY
});
  
  traits.set_supported_fan_modes(
  {
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_MIDDLE,
  });
  traits.set_supported_swing_modes(
  {
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_BOTH,
      climate::CLIMATE_SWING_VERTICAL,
      climate::CLIMATE_SWING_HORIZONTAL
  });

  //***
  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(TEMP_MIN);
  // traits.set_visual_min_temperature(this->minimum_temperature_);
  traits.set_visual_max_temperature(TEMP_MAX);
  // traits.set_visual_max_temperature(this->maximum_temperature_);
  traits.set_visual_temperature_step(1.0f);
  // traits.set_visual_temperature_step(this->temperature_step_);
  // traits.set_supported_fan_modes(this->fan_modes_);
  // traits.set_supported_swing_modes(this->swing_modes_);
  // traits.set_supported_presets(false);
  // traits.set_supported_presets(this->presets_);
  return traits;
}

// Config dump
void Haier1w::dump_config() {
  LOG_CLIMATE("", "Haier 1w Climate", this);
  ESP_LOGCONFIG(TAG, "  Min. Temperature: %.1f°C", this->minimum_temperature_);
  ESP_LOGCONFIG(TAG, "  Max. Temperature: %.1f°C", this->maximum_temperature_);
  ESP_LOGCONFIG(TAG, "  Supports HEAT: %s", YESNO(this->supports_heat_));
  ESP_LOGCONFIG(TAG, "  Supports COOL: %s", YESNO(this->supports_cool_));
}

//*** COMPONENT
void Haier1w::setup() {
  // Ability to send commands - only after first receiving
  this->ready_for_command_=false;
  // restore set points
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    // restore from defaults
    this->mode = climate::CLIMATE_MODE_OFF;
    // initialize target temperature to some value so that it's not NAN
    this->target_temperature =
        roundf(clamp(this->current_temperature, this->minimum_temperature_, this->maximum_temperature_));
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->swing_mode = climate::CLIMATE_SWING_OFF;
    this->preset = climate::CLIMATE_PRESET_NONE;

  }
  // Never send nan to HA
  if (std::isnan(this->target_temperature))
    this->target_temperature = 24;
  // Common statuses
  this->component_status_= READY;
  this->need_update_=false;
}

// Periodical send request (by transmitting current state) or request sending
void Haier1w::update() {
  if (this->component_status_== READY) {
    this->need_update_=false;
    this->transmit_state();
  } else {
    this->need_update_=true;
  }
}

// Check for pendidng command sending
void Haier1w::loop() {
  if (this->need_update_ && this->component_status_== READY) {
    this->need_update_=false;
    this->transmit_state();
  }
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
  // if (call.get_preset().has_value())
  //   this->preset = *call.get_preset();
  
  // Sending or requestimg sending
  if (this->component_status_== READY) {
    this->transmit_state();
    this->need_update_=false;
  } else {
    this->need_update_=true;
  }
  // this->publish_state(); //*** Publish only on AC Status receiving
}

// Transmit full state command
void Haier1w::transmit_state() {
  uint32_t remote_state = 0x8800000;
  this->component_status_= SENDING;
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

  //*************************
  // TEST
  //Send packet constants
  s_data_[_HEADER]=S_HEADER;

  s_data_[1]=0x01; // Temp and State
  s_data_[2]=0x02;  // allways
  s_data_[3]=0x03;  // allways
  s_data_[4]=0x04;  //0x40 - some command
  s_data_[5]=0x05;  // allways
  s_data_[6]=0x06;  //Mode
  s_data_[7]=0x07;  // allways
  s_data_[8]=0x08;  // allways
  s_data_[9]=0x09;  //0x00 some command
  s_data_[10]=0x0A; //some data
  //s_data_[11]=0x00; //CRC
  //*******************************

  // //Send packet constants
  // s_data_[_HEADER]=S_HEADER;

  // // s_data_[1]=0x00; // Temp and State
  // s_data_[2]=0x00;  // allways
  // s_data_[3]=0x00;  // allways
  // s_data_[4]=0x60;  //0x40 - some command
  // s_data_[5]=0x00;  // allways
  // //s_data_[6]=0x00;  //Mode
  // s_data_[7]=0x00;  // allways
  // s_data_[8]=0x00;  // allways
  // s_data_[9]=0x10;  //0x00 some command
  // s_data_[10]=0x01; //some data
  // //s_data_[11]=0x00; //CRC

  this->transmit_(this->s_data_); //Prepare and transmin
  // this->transmit_(remote_state); //Prepare and transmin
  // this->publish_state(); //*** Publish only on AC Status receiving
  this->component_status_= WAITING;
}


// Receiving data from AC
bool Haier1w::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t nbytes = 0;
  uint8_t nbits = 0;
  uint8_t allnbits = 0;
  uint32_t remote_state = 0;
  this->component_status_= RECIIVING;
  // If header received
  if (!data.expect_mark(this->receive_header_high_))
  // if (!data.expect_item(this->receive_header_high_, this->receive_header_low_))
    return false;
  // Clear buffer
  //this->clear_data_(this->r_data_);
  // Read bites to bytes of buffer
  for (nbytes=0; nbytes < sizeof(this->r_data_); nbytes++) {
    for (nbits = 0; nbits < 8; nbits++) {
      //Check for Space received
      if(data.expect_space(this->bit_space_)) { 
        //Check for Bit recived
        if (data.expect_mark(this->bit_one_)) {
        // if (data.expect_item(this->bit_high_, this->bit_one_low_)) {
          this->r_data_[nbytes]= (this->r_data_[nbytes] << 1) | 1;
          allnbits+=1;
          // remote_state = (remote_state << 1) | 1;
        } else if (data.expect_mark(this->bit_zero_)) {
        // } else if (data.expect_item(this->bit_high_, this->bit_zero_low_)) {
          this->r_data_[nbytes]= (this->r_data_[nbytes] << 1) | 0;
          allnbits+=1;
          // remote_state = (remote_state << 1) | 0;
        } else if (allnbits == RECEIVE_BITS) {
          break;
        } else {
          return false;
        }
      } else {
        return false;
      }
    }
  }
  // Inverting Receive buffer
  invert_r_data_();

  ESP_LOGD(TAG, "Received message: %s", this->getHex_(this->r_data_));
  //Check CRC
  uint8_t crc=this->calc_checksum_(this->r_data_);
  if (crc!= this->r_data_[R_CRC]) {
    ESP_LOGW(TAG, "Invalid checksum received");
    return false;
  }
  //Check Header
  if (this->r_data_[_HEADER] != R_HEADER) {
    ESP_LOGW(TAG, "Invalid header received");
    return false;
  }
  
  // Copy received state to command state
  //s_data_[_TEMP_MODE]=r_data_[_TEMP_MODE]; //Mode and temp

  // Ready for command sending
  this->ready_for_command_=true;
  //***********************
  
  
  
  
  // Target Temperature
  this->target_temperature=((this->r_data_[_TEMP_POWER] & TARGET_TEMP_MASK) >> TARGET_TEMP_SHIFT) + TARGET_TEMP_ADD;

  // Current temperature
  this->current_temperature=this->r_data_[CURRENT_TEMP] & CURRENT_TEMP_MASK;

  



  //*** OLD
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
  this->component_status_= READY;
  return true;
}

//*********************************
// Prepare and transmit to AC
void Haier1w::transmit_(uint8_t * value) {
  uint8_t nbytes = 0;
  uint8_t nbits = 0;
  uint8_t allnbits = 0;
  
  // Calc CRC and put to last byte in value
  uint8_t crc_=calc_checksum_(value);
  value[sizeof(value)-1]=crc_;
  // Prepare Send buffer
  //prepare_c_data_();
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
  for (nbytes=0; nbytes < sizeof(this->c_data_); nbytes++){
    for (nbits = 0; nbits < 8; nbits++) {
      // Prepare items !!!! with inverting
      if (this->c_data_[nbytes] & 1 << nbits) {
        data->item(this->bit_space_, this->bit_one_);
      } else {
        data->item(this->bit_space_, this->bit_zero_);
      }
      // if (this->c_data_[nbytes] & 1 << nbits) {
      //   data->item(this->bit_high_, this->bit_one_low_);
      // } else {
      //   data->item(this->bit_high_, this->bit_zero_low_);
      // }
    }
  }
  // Add mark bit
  data->item(this->bit_space_, this->bit_one_);
  // data->mark(this->bit_space_);
  // Transmit
  transmit.perform();
}

// Invert send buffer
void Haier1w::prepare_c_data_() {
  uint8_t nbytes = 0;
  for(nbytes=0;nbytes < sizeof(this->s_data_); nbytes++) {
    this->c_data_[nbytes]=this->s_data_[nbytes] ^ 0xFF;
  }
}

// Invert receive buffer
void Haier1w::invert_r_data_() {
  uint8_t nbytes = 0;
  for(nbytes=0;nbytes < sizeof(this->r_data_); nbytes++) {
    this->r_data_[nbytes]^=0xFF;
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
