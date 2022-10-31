/**
* Create by Miguel Ángel López on 20/07/19
**/

#ifndef HAIER_ESP_HAIER_H
#define HAIER_ESP_HAIER_H

#include "esphome.h"

using namespace esphome;
using namespace esphome::climate;

//CONTROL and STATUS
#define C_HEADER        0
#define S_HEADER        0
//STATE and SET_TEMP
#define C_POWER          1
#define S_POWER          1
#define C_TARG_TEMP       1
#define S_TARG_TEMP       1
#define BIT_ONOFF           0
#define TARG_TEMP_MASK       0xf0
#define TARG_TEMP_SHIFT      4
#define TARG_TEMP_SUB        16
//MODE
#define C_MODE          6
#define S_MODE          6 
#define MODE_MASK       0xf0
#define MODE_SHIFT      4
#define MODE_COOL       0x02
#define MODE_HEAT       0x04
#define MODE_FAN_ONLY   0x06
#define MODE_DRY
#define MODE_AUTO
//CURRENT TEMP
#define S_CUR_TEMP      9
#define CUR_TEMP_MASK   0x1F


#define C_CRC           11
#define S_CRC           18
#define C_LEN           12
#define S_LEN           19

// temperatures supported by AC system
#define MIN_SET_TEMPERATURE 16
#define MAX_SET_TEMPERATURE 31
#define STEP_TEMPERATURE 1

#define MIN_VALID_INTERNAL_TEMP 0
#define MAX_VALID_INTERNAL_TEMP 50

#define C_POLLING_INTERVAL   5000

#define H_TAG           "Haier_1w"

//*******************
//OLD
// #define TEMPERATURE     13
// #define COMMAND         17

// #define MODE            23
// #define MODE_SMART      0
// #define MODE_COOL       1
// #define MODE_HEAT       2
// #define MODE_FAN_ONLY   3
// #define MODE_DRY        4

// #define FAN_SPEED       25
// #define FAN_HIGH        0
// #define FAN_MIDDLE      1
// #define FAN_MEDIUM      2
// #define FAN_AUTO        3

// #define SWING           27
// #define SWING_OFF       0
// #define SWING_BOTH      1

// #define LOCK            28
// #define LOCK_ON         80
// #define LOCK_OFF        00

// #define POWER           29
// // bit position  >>
// #define POWER_BIT_ON             0
// #define HEALTH_MODE_BIT_ON       3
// #define COMPRESSOR_MODE_BIT_ON   4

// #define SWING_POS               31
// // bit position >>
// #define SWING_UNDEFINED_BIT     0
// #define TURBO_MODE_BIT_ON       1
// #define SILENT_MODE_BIT_ON      2
// #define SWING_HORIZONTAL_BIT    3
// #define SWING_VERTICAL_BIT      4
// #define LED_BIT_OFF             5

// #define SET_TEMPERATURE 35



// // temperatures supported by AC system
// #define MIN_SET_TEMPERATURE 16
// #define MAX_SET_TEMPERATURE 30
// #define STEP_TEMPERATURE 1
// #define TEMPERATURE_FOR_FAN_ONLY    8

// //if internal temperature is outside of those boundaries, message will be discarded
// #define MIN_VALID_INTERNAL_TEMP 10
// #define MAX_VALID_INTERNAL_TEMP 50

class Haier_1w : public Climate, public PollingComponent {

  private:

    // byte lastCRC;
    byte status_data[S_LEN];
    byte data[C_LEN];
    byte _data[C_LEN];
    bool _data_ready=false;

    // byte poll[13] = {255,255,10,0,0,0,0,0,1,1,77,1,90};
    // byte on[13] = {255,255,10,0,0,0,0,0,1,1,77,2,91};

  public:
  Haier_1w() : PollingComponent(C_POLLING_INTERVAL) {
      // lastCRC = 0;
  }

  //*** SETUP ***
  void setup() override {
      
      Serial.begin(9600);
    
      
  }
  //*** MAIN LOOP ***
  //CONTROL DATA RECEIVED FROM AC
  void loop() override  {
    if (Serial.available() >0) {
      if (Serial.read() != 0x5B) return;
      
      data[0] = 0x5B;
      Serial.readBytes(data+1, sizeof(data)-1);

      readData();

    }
  }
  //*** UPDATE (REQUEST) DATA FROM AC ***
  void update() override {    
    Serial.write(poll, sizeof(poll));
    auto raw = getHex(poll, sizeof(poll));
    ESP_LOGD(H_TAG, "POLL: %s ", raw.c_str());
  }

  //*** ESPHOME CLIMATE TRAITS ***
  protected:
  ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
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
    traits.set_visual_min_temperature(MIN_SET_TEMPERATURE);
    traits.set_visual_max_temperature(MAX_SET_TEMPERATURE);
    traits.set_visual_temperature_step(STEP_TEMPERATURE);
    traits.set_supports_current_temperature(true);
    traits.set_supports_two_point_target_temperature(false);

    return traits;
  }
  //*** WORK FUNCTIONS ***
  public:
  void readData() {
    auto raw = getHex(status_data, sizeof(status_data));
    ESP_LOGD(H_TAG, "Readed message: %s ", raw.c_str());

    byte check = getChecksum(status_data, sizeof(data)-1);

    if (check != status_data[S_CRC]) {
      ESP_LOGW(H_TAG, "Invalid checksum received");
      return;
    }

    current_temperature = status_data[S_CUR_TEMP] & CUR_TEMP_MASK;
    target_temperature = ((status_data[S_TARG_TEMP] & TARG_TEMP_MASK) >> TARG_TEMP_SHIFT) + TARG_TEMP_SUB;

    if(current_temperature < MIN_VALID_INTERNAL_TEMP || current_temperature > MAX_VALID_INTERNAL_TEMP 
      || target_temperature < MIN_SET_TEMPERATURE || target_temperature > MAX_SET_TEMPERATURE){
      ESP_LOGW(H_TAG, "Invalid temperatures received");
      return;
    }


    if (status_data[S_POWER] & ( 1 << BIT_ONOFF )) {

      switch ((status_data[S_MODE] & MODE_MASK) >> MODE_SHIFT) {
        case MODE_COOL:
          mode = CLIMATE_MODE_COOL;
          break;
        case MODE_HEAT:
          mode = CLIMATE_MODE_HEAT;
          break;
        case MODE_DRY:
          mode = CLIMATE_MODE_DRY;
          break;
        case MODE_FAN_ONLY:
          mode = CLIMATE_MODE_FAN_ONLY;
          break;
        default:
          mode = CLIMATE_MODE_HEAT_COOL;
        }

        if ( status_data[SWING_POS] & ( 1 << SILENT_MODE_BIT_ON )) {
          fan_mode = CLIMATE_FAN_LOW;
        } else {
          switch (status_data[FAN_SPEED]) {
            case FAN_AUTO:
              fan_mode = CLIMATE_FAN_AUTO;
              break;
            case FAN_MEDIUM:
              fan_mode = CLIMATE_FAN_MEDIUM;
              break;
            case FAN_MIDDLE:
              fan_mode = CLIMATE_FAN_MIDDLE;
              break;
            case FAN_HIGH:
              fan_mode = CLIMATE_FAN_HIGH;
              break;
            default:
              fan_mode = CLIMATE_FAN_AUTO;
          }
        }

        switch (status_data[SWING]) {
          case SWING_OFF: 
            if ( status_data[SWING_POS] & ( 1 << SWING_VERTICAL_BIT )) {
              swing_mode = CLIMATE_SWING_VERTICAL;
            } else if ( status_data[SWING_POS] & ( 1 << SWING_HORIZONTAL_BIT )) {
              swing_mode = CLIMATE_SWING_HORIZONTAL;
            } else {
              swing_mode = CLIMATE_SWING_OFF;
            }
            break;
          case SWING_BOTH:
            swing_mode = CLIMATE_SWING_BOTH;
            break;

        } 
    } else {
      mode = CLIMATE_MODE_OFF;
      //fan_mode = CLIMATE_FAN_OFF;
      //swing_mode = CLIMATE_SWING_OFF;
    }
    
    copy_from_status_data();
    this->publish_state();

  }

  void copy_from_status_data() {
    _data[1]=status_data[1];
    //_data[2]=status_data[2];
    //_data[3]=status_data[3];
    //_data[4]=status_data[4];
    //_data[5]=status_data[5];
    _data[6]=status_data[6];
    //_data[7]=status_data[7];
    //_data[8]=0x00;
    //_data[9]=status_data[8];
    //_data[10]=0x01;
    
    _data_ready=true;
   }
  // void copy_to_data() {
  //   for (int i=0; i < sizeof(data); i++){
  //       data[i]=_data[i];
  //   }
  // }  

  // Climate control from head
  void control(const ClimateCall &call) override {
    if _data_ready {
      data=_data;
      if (call.get_mode().has_value()){
        switch (call.get_mode().value()) {
          case CLIMATE_MODE_OFF:
            data[C_POWER] &= ~(1 << BIT_ONOFF);
            break;
          case CLIMATE_MODE_COOL:
            data[C_POWER] |= (1 << BIT_ONOFF);
            data[C_MODE] = MODE_COOL;
            break;
          case CLIMATE_MODE_HEAT:
            data[C_POWER] |= (1 << BIT_ONOFF);
            data[C_MODE] = MODE_HEAT;
            break;
          case CLIMATE_MODE_DRY:
            data[C_POWER] |= (1 << BIT_ONOFF);
            data[C_MODE] = MODE_DRY;
            break;
          case CLIMATE_MODE_FAN_ONLY:
            if (data[C_POWER] & ( 1 << BIT_ONOFF )) {
                //ESP_LOGW("Haier", "Cold start");
            } else {
                data[SET_TEMPERATURE] = TEMPERATURE_FOR_FAN_ONLY;
            }
            data[C_POWER] |= (1 << BIT_ONOFF);
            data[FAN_SPEED] = FAN_MIDDLE; // set fan speed for change work mode
            data[C_MODE] = MODE_FAN_ONLY;
            break;
          case CLIMATE_MODE_HEAT_COOL:
            data[C_POWER] |= (1 << BIT_ONOFF);
            data[C_MODE] = MODE_AUTO;
            break;
        }
      }
      //Set fan speed
      if (call.get_fan_mode().has_value()) {
        switch(call.get_fan_mode().value()) {
          case CLIMATE_FAN_LOW:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[SWING_POS] |= (1 << SILENT_MODE_BIT_ON);
            data[SWING_POS] &= ~(1 << TURBO_MODE_BIT_ON);
            break;
          case CLIMATE_FAN_MIDDLE:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[FAN_SPEED] = FAN_MIDDLE;
            data[SWING_POS] &= ~(1 << SILENT_MODE_BIT_ON);
            data[SWING_POS] &= ~(1 << TURBO_MODE_BIT_ON);
            break;
          case CLIMATE_FAN_MEDIUM:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[FAN_SPEED] = FAN_MEDIUM;
            data[SWING_POS] &= ~(1 << SILENT_MODE_BIT_ON);
            data[SWING_POS] &= ~(1 << TURBO_MODE_BIT_ON);
            break;
          case CLIMATE_FAN_HIGH:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[FAN_SPEED] = FAN_HIGH;
            data[SWING_POS] &= ~(1 << SILENT_MODE_BIT_ON);
            data[SWING_POS] &= ~(1 << TURBO_MODE_BIT_ON);
            break;
          case CLIMATE_FAN_AUTO:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[FAN_SPEED] = FAN_AUTO;
            data[SWING_POS] &= ~(1 << SILENT_MODE_BIT_ON);
            data[SWING_POS] &= ~(1 << TURBO_MODE_BIT_ON);
            break;
        }
      }

      //Set swing mode
      if (call.get_swing_mode().has_value()) {
        switch(call.get_swing_mode().value()) {
          case CLIMATE_SWING_OFF:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[SWING] = SWING_OFF;
            data[SWING_POS] &= ~(1 << SWING_UNDEFINED_BIT);
            break;
          case CLIMATE_SWING_VERTICAL:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[SWING] = SWING_OFF;
            data[SWING_POS] |= (1 << SWING_VERTICAL_BIT);
            data[SWING_POS] &= ~(1 << SWING_HORIZONTAL_BIT);
            break;
          case CLIMATE_SWING_HORIZONTAL:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[SWING] = SWING_OFF;
            data[SWING_POS] |= (1 << SWING_HORIZONTAL_BIT);
            data[SWING_POS] &= ~(1 << SWING_VERTICAL_BIT);
            break;
          case CLIMATE_SWING_BOTH:
            data[POWER] |= (1 << POWER_BIT_ON);
            data[SWING] = SWING_BOTH;
            data[SWING_POS] &= ~(1 << SWING_UNDEFINED_BIT);
            data[SWING_POS] &= ~(1 << SWING_VERTICAL_BIT);
            data[SWING_POS] &= ~(1 << SWING_HORIZONTAL_BIT);
            break;
        }
      }
      
      if (call.get_target_temperature().has_value()) {
        byte ttemp=((call.get_target_temperature().value() - TARG_TEMP_SUB) << TARG_TEMP_SHIFT) & TARG_TEMP_MASK;
        data[C_TARG_TEMP] = ttemp & (data[S_TARG_TEMP] & 0x0f);
      }

      //Values for "send"
      data[C_HEADER] = 0x5A;
      //Unknown data
      data[2] = 0x00;
      data[3] = 0x00;
      data[4] = 0x60;
      data[5] = 0x00;
      data[7] = 0x00;
      data[8] = 0x00;
      data[9] = 0x10;
      data[10] = 0x01;
      sendData(data, sizeof(data));
    }
  }

  // SEND DATA TO AC
  void sendData(byte * message, byte size) {

    byte crc = getChecksum(message, size-1);
    message[C_CRC]=crc;
    Serial.write(message, size);
    // Serial.write(crc);

    auto raw = getHex(message, size);
    ESP_LOGD(H_TAG, "Sended message: %s ", raw.c_str());

  }

  //--- HEX MASSIVE TO STRING ROWS  
  String getHex(byte * message, byte size) {
    String raw;
    for (int i=0; i < size; i++){
        raw += "\n" + String(i) + "-" + String(message[i]);
    }
    raw.toUpperCase();
    return raw;
  }

  //--- CHECKSUM for MESSAGE (SIZE without CRC)
  byte getChecksum(const byte * message, size_t size) {
    byte end_position = size - 1;
    byte crc = 0;
    for (int i = 0; i < position; i++)
        crc += message[i];
    return crc;
  }

};


#endif //HAIER_ESP_HAIER_H
