# Nodoshop Opentherm Gateway - esphome
This is an external esphome component to control a [Nodoshop Opentherm Gateway](https://www.nodo-shop.nl/nl/48-opentherm-gateway).

Tested with gateway v2.10 and v2.11, but it should also work with older versions.

It won't work with a DIYLESS Master OpenTherm Shield!

Due to a lack of testing hardware, not all functionality offered by the gateway is currently supported. If you need specific functionality and have hardware that supports it, feel free to create a new issue.

## Use cases
There are several ways to use this component. A full example of all the options can be found in `example_otgw.yaml`.

A simple setup would be to just use it to control the temperature setpoint remotely and perhaps update the time and outside temperature on the thermostat. This can be done with something like the following configuration:
```yaml
esp8266:
  board: d1_mini

external_components:
- source: github://KPWhiver/opentherm-gateway-esphome@main

uart:
  id: uart_bus
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  data_bits: 8
  stop_bits: 1
  parity: NONE

sensor:
- platform: homeassistant
  name: "Outside temperature"
  entity_id: sensor.outside_temperature
  id: outside_temperature

time:
- platform: homeassistant
  id: hass_time

otgw:
  # (Optional) This option will set the time using the time from Home Assistant
  time_source: hass_time
  # (Optional) This option will tell the heater the outside temperature which
  # allows it to take it into account when determining the water temperature.
  outside_temperature: outside_temperature

climate:
- platform: otgw
  room_thermostat:
    name: "Room"
```

Another option would be to add additional thermostats based on temperature sensors provided by home assistant. This can be done by adding something like the following to your configuration: (Use at your own risk, misconfiguration can lead to runaway heating and doing this might also have negative effects on your thermostat algorithm.)
```yaml
sensor:
- platform: homeassistant
  name: "Room2 temperature"
  entity_id: sensor.room2_temperature
  id: room2_temperature

climate:
- platform: otgw
  heating_circuit_1:
    id: heater_circuit
    name: "Heating circuit"
    internal: true
- platform: thermostat
  name: "Room2"
  sensor: room2_temperature
  min_heating_off_time: 300s
  min_heating_run_time: 300s
  min_idle_time: 300s
  heat_action:
  - lambda: |-
      auto call = id(heater_circuit).make_call();
      call.set_target_temperature(55);
      // Setting the mode to HEAT takes control away from the thermostat
      call.set_mode(climate::CLIMATE_MODE_HEAT);
      call.perform();
  idle_action:
  - lambda: |-
      auto call = id(heater_circuit).make_call();
      // Setting the mode to AUTO gives control back from the thermostat
      call.set_mode(climate::CLIMATE_MODE_AUTO);
      call.perform();
  default_preset: Home
  preset:
    - name: Home
      default_target_temperature_low: 20 °C
```

## Requesting extra information
The component will try to request information from the heater that the thermostat does not request. It does this by marking all the sensors in the yaml as interesting and querying them manually.

Manually requesting is done by altering messages that the heater does not support. There are two settings that can be added that will increase the number of message types that can be used for this, use with care:
```yaml
otgw:
  # This will make the heater ignore room temperature and setpoint. The component can then instead request other
  # information from the heater. Disable if your heater requires this information.
  reuse_master_slots: true
  # This will stop the heater from sending setpoint overrides to the thermostat. The component can then instead
  # request other information from the heater. Disable if you rely on your heater requesting this
  # information.
  ignore_heater_overrides: true
```

Message types that are requested by the thermostat but not mentioned in your YAML file will also be altered. As such, it is best to only put the sensors/components in the YAML that you actually need.
