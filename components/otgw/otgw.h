#pragma once

#include "climate.h"
#include "button.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/time/real_time_clock.h"

#include <string>
#include <bitset>
#include <vector>

namespace esphome {
namespace otgw {

template<typename ComponentType> class OptionalComponent {
 protected:
  ComponentType *component{nullptr};

 public:
  void set(ComponentType *comp) { component = comp; }

  void publish_state(decltype(component->state) state) {
    if (component != nullptr) {
      component->publish_state(state);
    }
  }
};

class OpenthermGateway : public Component, public uart::UARTDevice {
 protected:

  struct Transaction {
    enum Step : uint8_t {
      TH_REQUEST = 0,
      GA_REQUEST = 1,
      CH_RESPONSE = 2,
      GA_RESPONSE = 3,
    };
    uint8_t data_type = 0;
    std::array<optional<uint16_t>, 4> data;
  };
  optional<Transaction> _current_transaction;
  Transaction::Step _last_transaction_step;

  // Heater circuit 1 and 2 are very similar, so we reuse their behaviour
  // using this struct
  struct HeatingCircuit {
    uint64_t _time_of_last_command;
    OpenthermGatewayClimate *_component;
    const char *_temp_command;
    const char *_enable_command;

    void set_target(OpenthermGateway &gateway);
    void set_mode(OpenthermGateway &gateway);
    void set_callbacks(OpenthermGateway &gateway);
    void refresh(OpenthermGateway &gateway);
  };

  ///// Components /////
  OpenthermGatewayClimate *_room_thermostat{nullptr};
  OpenthermGatewayClimate *_hot_water{nullptr};
  OpenthermGatewayButton *_reset_service_request{nullptr};
  OpenthermGatewayButton *_hot_water_push{nullptr};
  optional<HeatingCircuit> _heating_circuit_1;
  optional<HeatingCircuit> _heating_circuit_2;

  sensor::Sensor *_outside_temperature_override{nullptr};
  time::RealTimeClock *_time_source{nullptr};

  bool _hot_water_temperature_reported{false};

 public:
  OptionalComponent<text_sensor::TextSensor> slave_opentherm_version;
  OptionalComponent<text_sensor::TextSensor> master_opentherm_version;
  OptionalComponent<text_sensor::TextSensor> opentherm_gateway_version;
  OptionalComponent<text_sensor::TextSensor> opentherm_gateway_build_date;
  OptionalComponent<text_sensor::TextSensor> last_reset_cause;

  // Master state
  OptionalComponent<binary_sensor::BinarySensor> master_central_heating_1;
  OptionalComponent<binary_sensor::BinarySensor> master_central_heating_2;
  OptionalComponent<binary_sensor::BinarySensor> master_water_heating;
  OptionalComponent<binary_sensor::BinarySensor> master_cooling;
  OptionalComponent<binary_sensor::BinarySensor> master_water_heating_blocking;
  OptionalComponent<binary_sensor::BinarySensor> master_summer_mode;
  OptionalComponent<binary_sensor::BinarySensor> master_outside_temperature_compensation;

  // Slave state
  OptionalComponent<binary_sensor::BinarySensor> slave_central_heating_1;
  OptionalComponent<binary_sensor::BinarySensor> slave_central_heating_2;
  OptionalComponent<binary_sensor::BinarySensor> slave_fault;
  OptionalComponent<binary_sensor::BinarySensor> slave_water_heating;
  OptionalComponent<binary_sensor::BinarySensor> slave_flame;
  OptionalComponent<binary_sensor::BinarySensor> slave_cooling;
  OptionalComponent<binary_sensor::BinarySensor> slave_diagnostic_event;

  // Faults
  OptionalComponent<binary_sensor::BinarySensor> service_required;
  OptionalComponent<binary_sensor::BinarySensor> lockout_reset;
  OptionalComponent<binary_sensor::BinarySensor> low_water_pressure;
  OptionalComponent<binary_sensor::BinarySensor> gas_flame_fault;
  OptionalComponent<binary_sensor::BinarySensor> air_pressure_fault;
  OptionalComponent<binary_sensor::BinarySensor> water_overtemperature;

  // Setpoints
  OptionalComponent<sensor::Sensor> max_central_heating_setpoint;
  OptionalComponent<sensor::Sensor> hot_water_setpoint;
  OptionalComponent<sensor::Sensor> remote_override_room_setpoint;
  OptionalComponent<sensor::Sensor> room_setpoint_1;
  OptionalComponent<sensor::Sensor> room_setpoint_2;
  OptionalComponent<sensor::Sensor> central_heating_setpoint_1;
  OptionalComponent<sensor::Sensor> central_heating_setpoint_2;

  // Temperatures
  OptionalComponent<sensor::Sensor> room_temperature;
  OptionalComponent<sensor::Sensor> hot_water_temperature_1;
  OptionalComponent<sensor::Sensor> hot_water_temperature_2;
  OptionalComponent<sensor::Sensor> central_heating_temperature_1;
  OptionalComponent<sensor::Sensor> central_heating_temperature_2;
  OptionalComponent<sensor::Sensor> outside_temperature;
  OptionalComponent<sensor::Sensor> return_water_temperature;
  OptionalComponent<sensor::Sensor> solar_storage_temperature;
  OptionalComponent<sensor::Sensor> solar_collector_temperature;
  OptionalComponent<sensor::Sensor> exhaust_temperature;

  // Modulation
  OptionalComponent<sensor::Sensor> max_relative_modulation_level;
  OptionalComponent<sensor::Sensor> max_boiler_capacity;
  OptionalComponent<sensor::Sensor> min_modulation_level;
  OptionalComponent<sensor::Sensor> relative_modulation_level;

  // Water
  OptionalComponent<sensor::Sensor> central_heating_water_pressure;
  OptionalComponent<sensor::Sensor> hot_water_flow_rate;

  // Starts
  OptionalComponent<sensor::Sensor> central_heating_burner_starts;
  OptionalComponent<sensor::Sensor> central_heating_pump_starts;
  OptionalComponent<sensor::Sensor> hot_water_pump_starts;
  OptionalComponent<sensor::Sensor> hot_water_burner_starts;

  // Operation hous
  OptionalComponent<sensor::Sensor> central_heating_burner_operation_time;
  OptionalComponent<sensor::Sensor> central_heating_pump_operation_time;
  OptionalComponent<sensor::Sensor> hot_water_pump_operation_time;
  OptionalComponent<sensor::Sensor> hot_water_burner_operation_time;

  // Thermostat
  OptionalComponent<sensor::Sensor> average_temperature;
  OptionalComponent<sensor::Sensor> average_temperature_change;
  OptionalComponent<sensor::Sensor> predicted_temperature;

  void set_room_thermostat(OpenthermGatewayClimate *clim);
  void set_hot_water(OpenthermGatewayClimate *clim);
  void set_heating_circuit_1(OpenthermGatewayClimate *clim);
  void set_heating_circuit_2(OpenthermGatewayClimate *clim);
  void set_outside_temperature_override(sensor::Sensor *sens);
  void set_time_source(time::RealTimeClock *time);
  void set_reset_service_request_button(OpenthermGatewayButton *butt);
  void set_hot_water_push_button(OpenthermGatewayButton *butt);

 protected:
  static constexpr uint16_t MAX_BUFFER_SIZE = 128;
  std::string _receive_buffer;

  static constexpr uint16_t MAX_COMMAND_QUEUE_LENGTH = 20;
  std::vector<std::string> _command_queue;
  optional<std::string> _send_command;
  uint16_t _lines_since_command = 0;

  void read_available();
  float parse_float(uint16_t data);
  int16_t parse_int16(uint16_t data);
  int8_t parse_int8(uint8_t data);
  bool is_error(std::string const &command_code);
  bool queue_command(char const *command, char const *parameter);
  void parse_command_response(std::string const &line);
  void handle_transaction();
  void parse_line(std::string const &line);

  climate::ClimateAction calculate_climate_action(bool flame, bool heating);
  bool set_room_setpoint(float temperature);
  bool set_heater_climate_target_temperature(optional<HeatingCircuit> &heating_circuit, float temperature);
  bool set_heater_climate_action(optional<HeatingCircuit> &heating_circuit, bool flame, bool heating);

 public:
  OpenthermGateway(uart::UARTComponent *parent) : uart::UARTDevice(parent) {
    _receive_buffer.reserve(MAX_BUFFER_SIZE);
    _command_queue.reserve(MAX_COMMAND_QUEUE_LENGTH);
  }

  void setup() override;
  void loop() override;
};

inline void OpenthermGateway::HeatingCircuit::set_target(OpenthermGateway &gateway) {
  char parameter[6];

  switch (_component->mode) {
    case climate::ClimateMode::CLIMATE_MODE_HEAT:
      // Do not go below 5 to avoid control being given back to the thermostat
      sprintf(parameter, "%2.2f", max(_component->target_temperature, 5.0f));
      gateway.queue_command(_temp_command, parameter);
      break;
    case climate::ClimateMode::CLIMATE_MODE_AUTO:
      // AUTO means the thermostat is in control so do nothing
      break;
    case climate::ClimateMode::CLIMATE_MODE_OFF:
      // In this case we don't actually set the temperature as that would trigger the otgw firmware to start heating
      break;
    default:
      ESP_LOGE("otgw", "Invalid climate mode for heating circuit %s", _enable_command);
      break;
  }
}

inline void OpenthermGateway::HeatingCircuit::set_mode(OpenthermGateway &gateway) {
  switch (_component->mode) {
    case climate::ClimateMode::CLIMATE_MODE_HEAT:
      // In this case, the temperature was set to 0 or 5 before, so here we restore it
      if (!std::isnan(_component->target_temperature)) {
        char parameter[6];
        sprintf(parameter, "%2.2f", max(_component->target_temperature, 5.0f));
        gateway.queue_command(_temp_command, parameter);
      }
      gateway.queue_command(_enable_command, "1");
      break;
    case climate::ClimateMode::CLIMATE_MODE_AUTO:
      gateway.queue_command(_temp_command, "0");
      break;
    case climate::ClimateMode::CLIMATE_MODE_OFF:
      gateway.queue_command(_temp_command, "5.00");
      gateway.queue_command(_enable_command, "0");
      break;
    default:
      ESP_LOGE("otgw", "Invalid climate mode for heating circuit %s", _enable_command);
      break;
  }
}

inline void OpenthermGateway::HeatingCircuit::set_callbacks(OpenthermGateway &gateway) {
  _component->set_callbacks([&]() {
    set_target(gateway);
  }, [&]() {
    set_mode(gateway);
  });
}

inline void OpenthermGateway::HeatingCircuit::refresh(OpenthermGateway &gateway) {
  uint64_t now = millis();
  if (
    // This means the clock has overrun
    now < _time_of_last_command ||
    // Refresh is needed at least every minute
    now - _time_of_last_command > 50'000
  ) {
    set_mode(gateway);
  }
}

}  // namespace otgw
}  // namespace esphome
