```yaml
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