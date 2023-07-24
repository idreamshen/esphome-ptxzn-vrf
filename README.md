# 硬件改造
1. 拆除原先 VRF 板载 ESP32 模组
1. 为 TX/RX 焊接接线端子，并将线引出
1. 使用外置电源（确保和 VRF 电源共地）给 ESP8266/ESP32 上电，并将上述的 TX/RX 与 ESP8266/ESP32 的 RX/TX 相连

# Esphome 配置
```yaml
logger:
  baud_rate: 0
  
uart:
  - id: vrf_uart
    tx_pin: 1
    rx_pin: 3
    baud_rate: 9600

climate:
- platform: custom
  lambda: |-
    auto vrfComponent = new PtxznVrfComponent(id(vrf_uart), 4);
    App.register_component(vrfComponent);
    App.register_component(vrfComponent->ptxznVrfClimateComponents[0]);
    App.register_component(vrfComponent->ptxznVrfClimateComponents[1]);
    App.register_component(vrfComponent->ptxznVrfClimateComponents[2]);
    App.register_component(vrfComponent->ptxznVrfClimateComponents[3]);
    return {
      vrfComponent->ptxznVrfClimateComponents[0],
      vrfComponent->ptxznVrfClimateComponents[1],
      vrfComponent->ptxznVrfClimateComponents[2],
      vrfComponent->ptxznVrfClimateComponents[3],
    };

  climates:
    - name: "AirCondition keting"
    - name: "AirCondition zhuwo"
    - name: "AirCondition canting"
    - name: "AirCondition ciwo"
```