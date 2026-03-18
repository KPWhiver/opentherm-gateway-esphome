#pragma once

#include "climate.h"
#include "button.h"
#include "data_types.h"
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

template<typename ComponentType>
class OptionalComponent {
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

template<typename ComponentType, typename DataType>
class OptionalOTComponent : public OptionalComponent<ComponentType> {};

class OpenthermGateway : public Component, public uart::UARTDevice {
 protected:

  struct Transaction {
    enum Step : uint8_t {
      TH_REQUEST = 0,
      GA_REQUEST = 1,
      CH_RESPONSE = 2,
      GA_RESPONSE = 3,
    };
    constexpr static std::array<char, 4> STEP{'T', 'R', 'B', 'A'};

    enum MessageType : uint8_t {
      // Request
      READ_DATA = 0b0000,
      WRITE_DATA = 0b0001,
      INVALID_DATA = 0b0010,
      RESERVED = 0b0011,

      // Response
      READ_ACK = 0b0100,
      WRITE_ACK = 0b0101,
      DATA_INVALID = 0b0110,
      UNKNOWN_DATAID = 0b0111,
    };
    constexpr static std::array<char const *, 8> MESSAGE_TYPE{
      "READ_DATA",
      "WRITE_DATA",
      "INVALID_DATA",
      "RESERVED",
      "READ_ACK",
      "WRITE_ACK",
      "DATA_INVALID",
      "UNKNOWN_DATAID",
    };
    uint8_t master_data_type = 0;
    uint8_t slave_data_type = 0;
    struct Message {
      MessageType message_type;
      uint16_t data;
    };
    using Messages = std::array<optional<Message>, 4>;
    Messages data;
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

  // Information to keep track of which data types are available
  struct DataTypeInfo {
    optional<uint8_t> consecutive_failures = 0;
    bool interest = false;
  };
  std::unordered_map<uint8_t, DataTypeInfo> _data_types;

  ///// Components /////
  OpenthermGatewayClimate *_room_thermostat{nullptr};
  OpenthermGatewayClimate *_hot_water{nullptr};
  OpenthermGatewayButton *_reset_service_request{nullptr};
  OpenthermGatewayButton *_hot_water_push{nullptr};
  optional<HeatingCircuit> _heating_circuit_1;
  optional<HeatingCircuit> _heating_circuit_2;

  bool _reuse_master_slots;
  sensor::Sensor *_outside_temperature_override{nullptr};
  time::RealTimeClock *_time_source{nullptr};

  bool _hot_water_temperature_reported{false};

 public:
  template<typename SensorType, typename DataType>
  void set_sensor(OptionalOTComponent<SensorType, DataType> &var, SensorType *sens) {
    _data_types[DataType::ID].interest = true;
    var.set(sens);
  }

  template<typename SensorType>
  void set_sensor(OptionalComponent<SensorType> &var, SensorType *sens) {
    var.set(sens);
  }

  OptionalOTComponent<text_sensor::TextSensor, SlaveOpenThermVersion> slave_opentherm_version;
  OptionalOTComponent<text_sensor::TextSensor, MasterOpenThermVersion> master_opentherm_version;
  OptionalComponent<text_sensor::TextSensor> opentherm_gateway_version;
  OptionalComponent<text_sensor::TextSensor> opentherm_gateway_build_date;
  OptionalComponent<text_sensor::TextSensor> last_reset_cause;

  // Master state
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_central_heating_1;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_central_heating_2;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_water_heating;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_cooling;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_water_heating_blocking;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_summer_mode;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> master_outside_temperature_compensation;

  // Slave state
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_central_heating_1;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_central_heating_2;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_fault;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_water_heating;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_flame;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_cooling;
  OptionalOTComponent<binary_sensor::BinarySensor, Status> slave_diagnostic_event;

  // Faults
  OptionalOTComponent<binary_sensor::BinarySensor, FaultFlags> service_required;
  OptionalOTComponent<binary_sensor::BinarySensor, FaultFlags> lockout_reset;
  OptionalOTComponent<binary_sensor::BinarySensor, FaultFlags> low_water_pressure;
  OptionalOTComponent<binary_sensor::BinarySensor, FaultFlags> gas_flame_fault;
  OptionalOTComponent<binary_sensor::BinarySensor, FaultFlags> air_pressure_fault;
  OptionalOTComponent<binary_sensor::BinarySensor, FaultFlags> water_overtemperature;

  // Setpoints
  OptionalOTComponent<sensor::Sensor, MaxCHWaterSetpoint> max_central_heating_setpoint;
  OptionalOTComponent<sensor::Sensor, DHWSetpoint> hot_water_setpoint;
  OptionalOTComponent<sensor::Sensor, RemoteOverrideRoomSetpoint> remote_override_room_setpoint;
  OptionalOTComponent<sensor::Sensor, RoomSetpoint> room_setpoint_1;
  OptionalOTComponent<sensor::Sensor, RoomSetpoint2> room_setpoint_2;
  OptionalOTComponent<sensor::Sensor, ControlSetpoint> central_heating_setpoint_1;
  OptionalOTComponent<sensor::Sensor, ControlSetpoint2> central_heating_setpoint_2;

  // Temperatures
  OptionalOTComponent<sensor::Sensor, RoomTemperature> room_temperature;
  OptionalOTComponent<sensor::Sensor, DHWTemperature> hot_water_temperature_1;
  OptionalOTComponent<sensor::Sensor, DHWTemperature2> hot_water_temperature_2;
  OptionalOTComponent<sensor::Sensor, BoilerFlowWaterTemperature> central_heating_temperature_1;
  OptionalOTComponent<sensor::Sensor, FlowTemperatureCH2> central_heating_temperature_2;
  OptionalOTComponent<sensor::Sensor, OutsideTemperature> outside_temperature;
  OptionalOTComponent<sensor::Sensor, ReturnWaterTemperature> return_water_temperature;
  OptionalOTComponent<sensor::Sensor, SolarStorageTemperature> solar_storage_temperature;
  OptionalOTComponent<sensor::Sensor, SolarCollectorTemperature> solar_collector_temperature;
  OptionalOTComponent<sensor::Sensor, ExhaustTemperature> exhaust_temperature;

  // Modulation
  OptionalOTComponent<sensor::Sensor, MaxRelativeModulationLevel> max_relative_modulation_level;
  OptionalOTComponent<sensor::Sensor, MaxBoilerCapMinModulationLevel> max_boiler_capacity;
  OptionalOTComponent<sensor::Sensor, MaxBoilerCapMinModulationLevel> min_modulation_level;
  OptionalOTComponent<sensor::Sensor, RelativeModulationLevel> relative_modulation_level;

  // Water
  OptionalOTComponent<sensor::Sensor, CHWaterPressure> central_heating_water_pressure;
  OptionalOTComponent<sensor::Sensor, DHWFlowRate> hot_water_flow_rate;

  // Starts
  OptionalOTComponent<sensor::Sensor, PowerCycles> power_cycles;
  OptionalOTComponent<sensor::Sensor, UnsuccesfulBurnerStarts> unsuccesful_burner_starts;
  OptionalOTComponent<sensor::Sensor, TimesFlameSignalLow> times_flame_signal_low;
  OptionalOTComponent<sensor::Sensor, SuccessfulBurnerStarts> central_heating_burner_starts;
  OptionalOTComponent<sensor::Sensor, CHPumpStarts> central_heating_pump_starts;
  OptionalOTComponent<sensor::Sensor, DHWPumpStarts> hot_water_pump_starts;
  OptionalOTComponent<sensor::Sensor, DHWBurnerStarts> hot_water_burner_starts;

  // Operation hous
  OptionalOTComponent<sensor::Sensor, BurnerOperationHours> central_heating_burner_operation_time;
  OptionalOTComponent<sensor::Sensor, CHPumpOperationHours> central_heating_pump_operation_time;
  OptionalOTComponent<sensor::Sensor, DHWPumpOperationHours> hot_water_pump_operation_time;
  OptionalOTComponent<sensor::Sensor, DHWBurnerOperationHours> hot_water_burner_operation_time;

  void set_room_thermostat(OpenthermGatewayClimate *clim);
  void set_hot_water(OpenthermGatewayClimate *clim);
  void set_heating_circuit_1(OpenthermGatewayClimate *clim);
  void set_heating_circuit_2(OpenthermGatewayClimate *clim);
  void set_outside_temperature_override(sensor::Sensor *sens);
  void set_time_source(time::RealTimeClock *time);
  void set_reset_service_request_button(OpenthermGatewayButton *butt);
  void set_hot_water_push_button(OpenthermGatewayButton *butt);

  void reuse_master_slots(bool reuse_slots);

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
  bool queue_command(char const *command, std::string const &parameter);
  void parse_command_response(std::string const &line);
  void handle_transaction(Transaction const &transaction);
  void handle_transaction_messages(uint8_t data_type, Transaction::Messages data);
  bool handle_slave_response(uint8_t data_type, uint16_t data);
  bool handle_master_request(uint8_t data_type, uint16_t data);
  bool handle_gateway_response(uint8_t data_type, uint16_t data);
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
