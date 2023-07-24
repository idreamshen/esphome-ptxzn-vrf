#include "esphome.h"

#define TAG "climate.vrf.ptxzn"

#define CMD_IDX_IDENTITY 1
#define CMD_IDX_ON_OFF 2
#define CMD_IDX_CLIMATE_MODE 3
#define CMD_IDX_TARGET_TEMP 4
#define CMD_IDX_FAN_MODE 5
#define CMD_IDX_CURRENT_TEMP 6

#define CMD_ON 0x01
#define CMD_OFF 0x00
#define CMD_CLIMATE_MODE_COOL 0x02
#define CMD_CLIMATE_MODE_HEAT 0x01
#define CMD_CLIMATE_MODE_FAN_ONLY 0x04
#define CMD_CLIMATE_MODE_DRY 0x08
#define CMD_FAN_MODE_AUTO 0x00
#define CMD_FAN_MODE_LOW 0x01
#define CMD_FAN_MODE_MEDIUM 0x02
#define CMD_FAN_MODE_HIGH 0x03

class PtxznVrfClimateComponent : public Component, public Climate {
  private:
  UARTComponent *uartComponent;
  uint8_t idx;

  public:
  PtxznVrfClimateComponent(UARTComponent *uartComponent, uint8_t idx) : Component(), Climate() {
    this->uartComponent = uartComponent;
    this->idx = idx;
  }

  void setup() override {
    // This will be called by App.setup()
  }

  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      this->mode = *call.get_mode();
    }
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
    }

    if (call.get_fan_mode().has_value()) {
      this->fan_mode = *call.get_fan_mode();
    }

    byte data[10] = {  0x01, idx, 0xFF, 0x04, 0x1D, 0x03, 0xFF, 0xFF, 0xFF, 0x00 };
    if (this->mode == ClimateMode::CLIMATE_MODE_OFF) {
      data[CMD_IDX_ON_OFF] = CMD_OFF;

      byte offData[10] = {  0x01, idx, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFA };
      offData[9] = offData[9] + 0x01 + idx;
      this->uartComponent->write_array(offData, sizeof(offData));
      ESP_LOGD(TAG, "receive control cmd, mode=off, send %x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        offData[0], offData[1], offData[2], offData[3], offData[4],
        offData[5], offData[6], offData[7], offData[8], offData[9]);

      this->publish_state();
      return;

    } else {
      data[CMD_IDX_ON_OFF] = CMD_ON;
      if (this -> mode == ClimateMode::CLIMATE_MODE_COOL) {
        data[CMD_IDX_CLIMATE_MODE] = CMD_CLIMATE_MODE_COOL;
      } else if (this -> mode == ClimateMode::CLIMATE_MODE_HEAT) {
        data[CMD_IDX_CLIMATE_MODE] = CMD_CLIMATE_MODE_HEAT;
      } else if (this -> mode == ClimateMode::CLIMATE_MODE_FAN_ONLY) {
        data[CMD_IDX_CLIMATE_MODE] = CMD_CLIMATE_MODE_FAN_ONLY;
      } else if (this -> mode == ClimateMode::CLIMATE_MODE_DRY) {
        data[CMD_IDX_CLIMATE_MODE] = CMD_CLIMATE_MODE_DRY;
      }
    }

    if (this->fan_mode == climate::CLIMATE_FAN_HIGH) {
      data[CMD_IDX_FAN_MODE] = CMD_FAN_MODE_HIGH;
    } else if (this->fan_mode == climate::CLIMATE_FAN_MEDIUM) {
      data[CMD_IDX_FAN_MODE] = CMD_FAN_MODE_MEDIUM;
    } else if (this->fan_mode == climate::CLIMATE_FAN_LOW) {
      data[CMD_IDX_FAN_MODE] = CMD_FAN_MODE_LOW;
    }

    data[CMD_IDX_TARGET_TEMP] = this->target_temperature;

    data[9] = data[0] + data[1] + data[2] + data[3] + data[4] + data[5] - 0x03;

    this->uartComponent->write_array(data, sizeof(data));
    ESP_LOGD(TAG, "receive control cmd, send %x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
      data[0], data[1], data[2], data[3], data[4],
      data[5], data[6], data[7], data[8], data[9]);

    this->publish_state();
  }

  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_visual_target_temperature_step(1);
    traits.set_visual_temperature_step(1);
    traits.set_visual_min_temperature(16);
    traits.set_visual_max_temperature(30);
    traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH
    });
    traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_FAN_ONLY,
      climate::CLIMATE_MODE_DRY
    });
    return traits;
  }
};

class PtxznVrfComponent : public PollingComponent, public UARTDevice {
  private:
  uint8_t num;
  uint8_t updatePos;

  public:
  PtxznVrfClimateComponent *ptxznVrfClimateComponents[32]; // max is 32
  PtxznVrfComponent(UARTComponent *uartComponent, uint8_t num) : PollingComponent(2000), UARTDevice(uartComponent) {
    this->num = num;
    for (uint8_t i = 0; i < num; i++) {
      this->ptxznVrfClimateComponents[i] = new PtxznVrfClimateComponent(uartComponent, i);
    }
  }

  void setup() override {
  }

  void update() override {
    if (updatePos >= this->num) {
      updatePos = 0;
    }
    byte data[10] = {  0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFA };
    data[1] = data[1] + updatePos;
    data[9] = data[9] + updatePos;
    write_array(data, sizeof(data));
    ESP_LOGD(TAG, "update loop, send %x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
      data[0], data[1], data[2], data[3], data[4],
      data[5], data[6], data[7], data[8], data[9]);

    updatePos++;
  }

  void loop() override {
    while(available() >= 10) {
      uint8_t data[10];
      for (int i = 0; i <= 9; i++) {
        uint8_t c = read();
        data[i] = c;
      }

      ESP_LOGD(TAG, "uart bus receive %x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
      data[0], data[1], data[2], data[3], data[4],
      data[5], data[6], data[7], data[8], data[9]);

      uint8_t sum = 0;
      for (int i = 0; i <= 8; i++) {
        sum += data[i];
      }

      if (sum != data[9]) {
        ESP_LOGD(TAG, "checksum fail, sum=%d, data[9]=%d", sum, data[9]);
        read(); // drop one

        continue;
      } else {
        ESP_LOGD(TAG, "checksum succ, sum=%d", sum);
      }

      uint8_t identity = data[CMD_IDX_IDENTITY];
      auto ptxznVrfClimateComponent = this->ptxznVrfClimateComponents[identity];

      ESP_LOGD(TAG, "identity=%d receive data", identity);

      if (data[CMD_IDX_ON_OFF] == CMD_OFF) {
        ESP_LOGD(TAG, "identity=%d is off", identity);
        ptxznVrfClimateComponent->mode = ClimateMode::CLIMATE_MODE_OFF;
      } else if (data[CMD_IDX_ON_OFF] == CMD_ON) {
        ESP_LOGD(TAG, "identity=%d is on", identity);
        if (data[CMD_IDX_CLIMATE_MODE] == CMD_CLIMATE_MODE_COOL) {
          ESP_LOGD(TAG, "identity=%d climate_mode is cool", identity);
          ptxznVrfClimateComponent->mode = ClimateMode::CLIMATE_MODE_COOL;
        } else if (data[CMD_IDX_CLIMATE_MODE] == CMD_CLIMATE_MODE_HEAT) {
          ESP_LOGD(TAG, "identity=%d climate_mode is heat", identity);
          ptxznVrfClimateComponent->mode = ClimateMode::CLIMATE_MODE_HEAT;
        } else if (data[CMD_IDX_CLIMATE_MODE] == CMD_CLIMATE_MODE_FAN_ONLY) {
          ESP_LOGD(TAG, "identity=%d climate_mode is fan_only", identity);
          ptxznVrfClimateComponent->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
        } else if (data[CMD_IDX_CLIMATE_MODE] == CMD_CLIMATE_MODE_DRY) {
          ESP_LOGD(TAG, "identity=%d climate_mode is dry", identity);
          ptxznVrfClimateComponent->mode = ClimateMode::CLIMATE_MODE_DRY;
        }
      }

      if (data[CMD_IDX_FAN_MODE] == CMD_FAN_MODE_AUTO) {
        ESP_LOGD(TAG, "identity=%d fan_mode is auto", identity);
        ptxznVrfClimateComponent->fan_mode = climate::CLIMATE_FAN_AUTO;
      } else if (data[CMD_IDX_FAN_MODE] == CMD_FAN_MODE_LOW) {
        ESP_LOGD(TAG, "identity=%d fan_mode is low", identity);
        ptxznVrfClimateComponent->fan_mode = climate::CLIMATE_FAN_LOW;
      } else if (data[CMD_IDX_FAN_MODE] == CMD_FAN_MODE_MEDIUM) {
        ESP_LOGD(TAG, "identity=%d fan_mode is medium", identity);
        ptxznVrfClimateComponent->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      } else if (data[CMD_IDX_FAN_MODE] == CMD_FAN_MODE_HIGH) {
        ESP_LOGD(TAG, "identity=%d fan_mode is high", identity);
        ptxznVrfClimateComponent->fan_mode = climate::CLIMATE_FAN_HIGH;
      }

      ptxznVrfClimateComponent->target_temperature = data[CMD_IDX_TARGET_TEMP];
      ptxznVrfClimateComponent->current_temperature = data[CMD_IDX_CURRENT_TEMP];

      ptxznVrfClimateComponent->publish_state();
    }
  }
};
