#include "otgw.h"

#include "climate.h"
#include "esphome/components/esp8266/gpio.h"

namespace esphome {
namespace otgw {

using namespace data_types;

float OpenthermGateway::parse_float(uint16_t data) { return ((data & 0x8000) ? -(0x10000L - data) : data) / 256.0f; }

int16_t OpenthermGateway::parse_int16(uint16_t data) { return *reinterpret_cast<int16_t *>(&data); }

int8_t OpenthermGateway::parse_int8(uint8_t data) { return *reinterpret_cast<int8_t *>(&data); }

bool OpenthermGateway::is_error(std::string const &command_code) {
  if (command_code == "NG")
    ESP_LOGE("otgw", "The command code is unknown.");
  else if (command_code == "SE")
    ESP_LOGE("otgw", "The command contained an unexpected character or was incomplete.");
  else if (command_code == "BV")
    ESP_LOGE("otgw", "The command contained a data value that is not allowed.");
  else if (command_code == "OR")
    ESP_LOGE("otgw", "A number was specified outside of the allowed range.");
  else if (command_code == "NS")
    ESP_LOGE("otgw", "The alternative Data-ID could not be added because the table is full.");
  else if (command_code == "NF")
    ESP_LOGI("otgw", "The specified alternative Data-ID could not be removed because it does not exist in the table.");
  else if (command_code == "OE")
    ESP_LOGE("otgw", "The processor was busy and failed to process all received characters.");
  else
    return false;

  return true;
}

bool OpenthermGateway::queue_command(char const *command, std::string const &parameter) {
  if (_command_queue.size() == MAX_COMMAND_QUEUE_LENGTH) {
    ESP_LOGE("otgw", "Failed to send %s=%s because the queue is full", command, parameter.c_str());
    return false;
  }

  _command_queue.push_back(std::string(command) + "=" + parameter + "\r\n");
  return true;
}

void OpenthermGateway::setup() {
  // Reset the PIC, useful when it is confused due to serial weirdness during startup
  esp8266::ESP8266GPIOPin pic_reset;
  pic_reset.set_pin(D5);
  pic_reset.set_inverted(false);
  pic_reset.set_flags(gpio::Flags::FLAG_OUTPUT);

  pic_reset.setup();
  pic_reset.digital_write(false);
  write_str("GW=R\r"); // prime for immediate sending
  delay(100);
  pic_reset.digital_write(true);
  pic_reset.pin_mode(gpio::Flags::FLAG_INPUT);

  // Get gateway info
  queue_command("PR", "A");
  queue_command("PR", "B");
  queue_command("PR", "Q");

  // Trigger slave opentherm version requests. We do this to find out when the initialization of
  // the gateway is done
  queue_command("KI", SlaveOpenThermVersion::as_str());
  queue_command("AA", SlaveOpenThermVersion::as_str());
}

void OpenthermGateway::read_available() {
  while (available()) {
    char c = read();
    if (c == -1) {  // Nothing received
      return;
    }

    if (c == '\r') {  // Can be ignored
      continue;
    }

    if (c == '\n') {  // End of the line
      parse_line(_receive_buffer);
      _receive_buffer.clear();
      continue;
    }

    if (_receive_buffer.size() == MAX_BUFFER_SIZE) {  // Buffer full
      _receive_buffer.clear();
    }

    _receive_buffer += c;
  }
}

void OpenthermGateway::parse_command_response(std::string const &line) {
  if (!_send_command) {
    ESP_LOGE("otgw", "Received unexpected reply (%s).", line.c_str());
    return;
  }

  std::string command_code = line.substr(0, 2);

  if (command_code == "OE") {
    if (_command_queue.size() < MAX_COMMAND_QUEUE_LENGTH)
      _command_queue.push_back(*_send_command);
    _send_command.reset();
    return;
  }

  if (is_error(command_code)) {
    _send_command.reset();
    return;
  }

  if (command_code != _send_command->substr(0, 2)) {
    ESP_LOGE("otgw", "Received reply (%s) that does not match command (%s).", line.c_str(), _send_command->c_str());
    return;
  }

  _lines_since_command = 0;

  if (command_code == "PR") {
    char print_report_code = line.at(4);
    if (_send_command->at(3) != print_report_code) {
      ESP_LOGE("otgw", "Received reply (%s) that does not match command (%s).", line.c_str(), _send_command->c_str());
      return;
    }

    switch (print_report_code) {
      case 'A':
        this->opentherm_gateway_version.publish_state(line.substr(6));
        break;
      case 'B':
        this->opentherm_gateway_build_date.publish_state(line.substr(6));
        break;
      case 'Q': {
        char last_reset_code = line.at(6);
        switch (last_reset_code) {
          case 'B':
            this->last_reset_cause.publish_state("Brown out");
            break;
          case 'C':
            this->last_reset_cause.publish_state("GW=R command");
            break;
          case 'E':
            this->last_reset_cause.publish_state("Reset signal");
            break;
          case 'L':
            this->last_reset_cause.publish_state("Boot loop");
            break;
          case 'O':
            this->last_reset_cause.publish_state("Stack overflow");
            break;
          case 'P':
            this->last_reset_cause.publish_state("Power on");
            break;
          case 'S':
            this->last_reset_cause.publish_state("BREAK condition on serial interface");
            break;
          case 'U':
            this->last_reset_cause.publish_state("Stack underflow");
            break;
          case 'W':
            this->last_reset_cause.publish_state("Watch dog timer");
            break;
          default:
            ESP_LOGW("otgw", "Received unknown last reset code: %c", last_reset_code);
            break;
        }
        break;
      }
      default:
        ESP_LOGE("otgw", "No code written to process response: %s", line.c_str());
        break;
    }
  } else if (command_code == "CS") {
    if (_heating_circuit_1) {
      // The CS command needs to be given once per minute
      _heating_circuit_1->_time_of_last_command = millis_64();
    }
  } else if (command_code == "C2") {
    if (_heating_circuit_2) {
      // The C2 command needs to be given once per minute
      _heating_circuit_2->_time_of_last_command = millis_64();
    }
  }
  _send_command.reset();
}

bool OpenthermGateway::handle_slave_response(uint8_t data_type, uint16_t data) {
  uint8_t high_data = (data >> 8) & 0xFF;
  uint8_t low_data = data & 0xFF;

  switch (data_type) {
    case Status::ID: {
      // Master state
      std::bitset<8> master_bits(high_data);

      bool master_central_heating_1 = master_bits[0];
      if (_room_thermostat != nullptr) {
        _room_thermostat->set_action(master_central_heating_1 ? climate::CLIMATE_ACTION_HEATING
                                                              : climate::CLIMATE_ACTION_IDLE);
      }

      bool master_water_heating = master_bits[1];
      bool master_block_hot_water = master_bits[6];
      if (_hot_water != nullptr) {
        climate::ClimateMode mode = climate::CLIMATE_MODE_OFF;
        if (!master_block_hot_water) {
          mode = master_water_heating ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_AUTO;
        }
        _hot_water->set_mode(mode);
      }

      this->master_central_heating_1.publish_state(master_central_heating_1);
      this->master_water_heating.publish_state(master_water_heating);
      this->master_cooling.publish_state(master_bits[2]);
      this->master_outside_temperature_compensation.publish_state(master_bits[3]);
      this->master_central_heating_2.publish_state(master_bits[4]);
      this->master_summer_mode.publish_state(master_bits[5]);
      this->master_water_heating_blocking.publish_state(master_bits[6]);

      // Slave state
      std::bitset<8> slave_bits(low_data);
      bool slave_flame = slave_bits[3];

      bool central_heating_1 = slave_bits[1];
      set_heater_climate_action(_heating_circuit_1, slave_flame, central_heating_1);

      bool central_heating_2 = slave_bits[5];
      set_heater_climate_action(_heating_circuit_2, slave_flame, central_heating_2);

      bool water_heating = slave_bits[2];
      if (_hot_water != nullptr) {
        _hot_water->set_action(calculate_climate_action(slave_flame, water_heating));
      }

      bool fault = slave_bits[0];
      // Some heaters will not send FaultFlags if there is no fault. So we set those here or trigger
      // new faultflags messages by marking them as "known"
      if (!fault) {
        this->service_required.publish_state(false);
        this->lockout_reset.publish_state(false);
        this->low_water_pressure.publish_state(false);
        this->gas_flame_fault.publish_state(false);
        this->air_pressure_fault.publish_state(false);
        this->water_overtemperature.publish_state(false);
      } else {
        auto &fault_flags = _data_types[FaultFlags::ID];
        if (fault_flags.interest && !fault_flags.supported) {
          queue_command("KI", FaultFlags::as_str());
        }
      }
      this->slave_fault.publish_state(fault);
      this->slave_central_heating_1.publish_state(central_heating_1);
      this->slave_water_heating.publish_state(water_heating);
      this->slave_flame.publish_state(slave_flame);
      this->slave_cooling.publish_state(slave_bits[4]);
      this->slave_central_heating_2.publish_state(central_heating_2);
      this->slave_diagnostic_event.publish_state(slave_bits[6]);
      break;
    }
    case ControlSetpoint::ID: {
      float temperature = parse_float(data);
      this->central_heating_setpoint_1.publish_state(temperature);
      if (_heating_circuit_1) {
        _heating_circuit_1->_component->set_target_temperature(temperature);
      }
      break;
    }
    case SlaveConfiguration::ID: {
      std::bitset<8> slave_configuration(high_data);
      if (_room_thermostat != nullptr) {
        _room_thermostat->set_cooling_supported(slave_configuration[2]);
      }
    }
    case FaultFlags::ID: {
      std::bitset<8> bits(high_data);
      this->service_required.publish_state(bits[0]);
      this->lockout_reset.publish_state(bits[1]);
      this->low_water_pressure.publish_state(bits[2]);
      this->gas_flame_fault.publish_state(bits[3]);
      this->air_pressure_fault.publish_state(bits[4]);
      this->water_overtemperature.publish_state(bits[5]);
      break;
    }
    case ControlSetpoint2::ID: {
      float temperature = parse_float(data);
      this->central_heating_setpoint_2.publish_state(temperature);
      if (_heating_circuit_2) {
        _heating_circuit_2->_component->set_target_temperature(temperature);
      }
      break;
    }
    case RemoteOverrideRoomSetpoint::ID:
      this->remote_override_room_setpoint_1.publish_state(parse_float(data));
      break;
    case MaxRelativeModulationLevel::ID:
      this->max_relative_modulation_level.publish_state(parse_float(data));
      break;
    case MaxBoilerCapMinModulationLevel::ID:
      this->max_boiler_capacity.publish_state(high_data);
      this->min_modulation_level.publish_state(low_data);
      break;
    case RelativeModulationLevel::ID:
      this->relative_modulation_level.publish_state(parse_float(data));
      break;
    case CHWaterPressure::ID:
      this->central_heating_water_pressure.publish_state(parse_float(data));
      break;
    case DHWFlowRate::ID:
      this->hot_water_flow_rate.publish_state(parse_float(data));
      break;
    case BoilerFlowWaterTemperature::ID: {
      float temperature = parse_float(data);
      this->central_heating_temperature_1.publish_state(temperature);

      if (_heating_circuit_1) {
        _heating_circuit_1->_component->set_current_temperature(temperature);
      }
      break;
    }
    case DHWTemperature::ID: {
      float temperature = parse_float(data);
      this->hot_water_temperature_1.publish_state(temperature);

      if (_hot_water != nullptr) {
        _hot_water->set_current_temperature(temperature);
      }
      break;
    }
    case OutsideTemperature::ID:
      this->outside_temperature.publish_state(parse_float(data));
      break;
    case ReturnWaterTemperature::ID:
      this->return_water_temperature.publish_state(parse_float(data));
      break;
    case SolarStorageTemperature::ID:
      this->solar_storage_temperature.publish_state(parse_float(data));
      break;
    case SolarCollectorTemperature::ID:
      this->solar_collector_temperature.publish_state(parse_int16(data));
      break;
    case FlowTemperatureCH2::ID: {
      float temperature = parse_float(data);
      this->central_heating_temperature_2.publish_state(temperature);

      if (_heating_circuit_2) {
        _heating_circuit_2->_component->set_current_temperature(temperature);
      }
      break;
    }
    case DHWTemperature2::ID:
      this->hot_water_temperature_2.publish_state(parse_float(data));
      break;
    case ExhaustTemperature::ID:
      this->exhaust_temperature.publish_state(parse_int16(data));
      break;
    case DHWSetpointBounds::ID:
      if (_hot_water != nullptr) {
        _hot_water->set_max_temperature(high_data);
        _hot_water->set_min_temperature(low_data);
      }
      break;
    case DHWSetpoint::ID: {
      float temperature = parse_float(data);
      this->hot_water_setpoint.publish_state(temperature);

      if (_hot_water != nullptr) {
        _hot_water->set_target_temperature(temperature);
      }
      break;
    }
    case MaxCHWaterSetpoint::ID: {
      double temperature = parse_float(data);
      this->max_central_heating_setpoint.publish_state(temperature);
      if (_heating_circuit_1) {
        _heating_circuit_1->_component->set_max_temperature(temperature);
      }
      if (_heating_circuit_2) {
        _heating_circuit_2->_component->set_max_temperature(temperature);
      }
      break;
    }
    case SuccessfulBurnerStarts::ID:
      this->central_heating_burner_starts.publish_state(data);
      break;
    case CHPumpStarts::ID:
      this->central_heating_pump_starts.publish_state(data);
      break;
    case DHWPumpStarts::ID:
      this->hot_water_pump_starts.publish_state(data);
      break;
    case DHWBurnerStarts::ID:
      this->hot_water_burner_starts.publish_state(data);
      break;
    case BurnerOperationHours::ID:
      this->central_heating_burner_operation_time.publish_state(data);
      break;
    case CHPumpOperationHours::ID:
      this->central_heating_pump_operation_time.publish_state(data);
      break;
    case DHWPumpOperationHours::ID:
      this->hot_water_pump_operation_time.publish_state(data);
      break;
    case DHWBurnerOperationHours::ID:
      this->hot_water_burner_operation_time.publish_state(data);
      break;
    case SlaveOpenThermVersion::ID:
      this->slave_opentherm_version.publish_state(std::to_string(parse_float(data)));
      break;
    default:
      return false;
  }
  return true;
}

bool OpenthermGateway::handle_master_request(uint8_t data_type, uint16_t data) {
  switch (data_type) {
    case RoomSetpoint::ID: {
      float temperature = parse_float(data);
      this->room_setpoint_1.publish_state(temperature);

      if (_room_thermostat != nullptr) {
        _room_thermostat->set_target_temperature(temperature);
      }
      break;
    }
    case RoomSetpoint2::ID:
      this->room_setpoint_2.publish_state(parse_float(data));
      break;
    case RoomTemperature::ID: {
      float temperature = parse_float(data);
      this->room_temperature_1.publish_state(temperature);

      if (_room_thermostat != nullptr) {
        _room_thermostat->set_current_temperature(temperature);
      }
      break;
    }
    case RoomTemperatureCH2::ID:
      break;
    case OutsideTemperature::ID:
      this->outside_temperature.publish_state(parse_float(data));
      break;
    case MasterOpenThermVersion::ID:
      this->master_opentherm_version.publish_state(std::to_string(parse_float(data)));
      break;
    default:
      return false;
  }
  return true;
}

bool OpenthermGateway::handle_gateway_response(uint8_t data_type, uint16_t data) {
  switch (data_type) {
    case RemoteOverrideRoomSetpoint::ID: {
      if (_room_thermostat != nullptr) {
        if (parse_float(data) == 0) {
          _room_thermostat->set_mode(climate::ClimateMode::CLIMATE_MODE_AUTO);
        } else {
          _room_thermostat->set_mode(climate::ClimateMode::CLIMATE_MODE_HEAT);
        }
      }
    }
    default:
      return false;
  }
  return true;
}

void OpenthermGateway::handle_transaction(OpenthermGateway::Transaction const &transaction) {
  ESP_LOGD("otgw", "Received %d,%d transaction:", transaction.master_data_type, transaction.slave_data_type);
  for (uint8_t step = 0; step != 4; ++step) {
    auto message = transaction.data[step];
    if (message) {
      ESP_LOGI("otgw", "  %c, %s: %d", Transaction::STEP[step], Transaction::MESSAGE_TYPE[message->message_type], message->data);
    }
  }

  bool reusable_master_slot = false;
  if (
    _reuse_master_slots && transaction.data[Transaction::TH_REQUEST] &&
    transaction.data[Transaction::TH_REQUEST]->message_type == Transaction::WRITE_DATA && (
      transaction.master_data_type == RoomSetpoint::ID ||
      transaction.master_data_type == RoomSetpoint2::ID ||
      transaction.master_data_type == RoomTemperature::ID ||
      transaction.master_data_type == RoomTemperatureCH2::ID ||
      transaction.master_data_type == MasterOpenThermVersion::ID ||
      transaction.master_data_type == MasterProductVersion::ID
    )
  ) {
    reusable_master_slot = true;
  }

  if (transaction.master_data_type != transaction.slave_data_type) {
    // In this case we are really dealing with two transactions so we split them up
    if (transaction.slave_data_type == SlaveOpenThermVersion::ID) {
      _ready_for_requests = true;
    }

    // We don't support the use of alternatives as it disrupts the algorithm determining when
    // to send priority messages
    queue_command("DA", std::to_string(transaction.slave_data_type));

    if (!reusable_master_slot) {
      auto const &data_type_info = _data_types[transaction.master_data_type];
      if (data_type_info.interest && data_type_info.supported) {
        // We are interested but apparently it is marked as unknown
        queue_command("KI", std::to_string(transaction.master_data_type));
      }
    }

    auto data = transaction.data;
    data[Transaction::GA_REQUEST].reset();
    data[Transaction::CH_RESPONSE].reset();
    handle_transaction_messages(transaction.master_data_type, data);

    data = transaction.data;
    data[Transaction::TH_REQUEST].reset();
    data[Transaction::GA_RESPONSE].reset();
    handle_transaction_messages(transaction.slave_data_type, data);
  } else {
    uint8_t data_type = transaction.master_data_type;

    if (reusable_master_slot) {
      queue_command("UI", std::to_string(data_type));
    }

    handle_transaction_messages(data_type, transaction.data);
  }
}

void OpenthermGateway::handle_transaction_messages(uint8_t data_type, OpenthermGateway::Transaction::Messages data) {
  std::optional<bool> read_transaction;
  bool supported = true;
  for (uint8_t step = 0; step != 4; ++step) {
    auto &message = data[step];
    if (!message) {
      continue;
    }

    bool step_request = step <= Transaction::GA_REQUEST;
    bool message_type_request = message->message_type <= Transaction::RESERVED;
    if (step_request != message_type_request) {
      ESP_LOGE("otgw", "Received line message type (%d) and transaction step (%d) does not match", message->message_type, step);
      return;
    }

    std::optional<bool> read_message;
    switch (message->message_type) {
      case Transaction::READ_DATA:
        read_message = true;
        break;
      case Transaction::WRITE_DATA:
        read_message = false;
        break;
      case Transaction::INVALID_DATA:
        // This means that this message is required to be sent but not valid
        // in this particular situation
        ESP_LOGW("otgw", "MSG %d: invalid data", data_type);
        return;
      case Transaction::RESERVED:
        return;

      case Transaction::READ_ACK:
        read_message = true;
        break;
      case Transaction::WRITE_ACK:
        read_message = false;
        break;
      case Transaction::DATA_INVALID:
        ESP_LOGW("otgw", "MSG %d: data invalid", data_type);
        supported = false;
        break;
      case Transaction::UNKNOWN_DATAID:
        if (step != Transaction::GA_RESPONSE) {
          // This just means the gateway has overriden the request, not really an error
          ESP_LOGW("otgw", "MSG %d: unknown dataid", data_type);
          supported = false;
        }
        break;
      default:
        ESP_LOGE("otgw", "Received unknown message type %d", message->message_type);
        return;
    }

    if (read_message) {
      if (!read_transaction) {
        read_transaction = read_message;
      } else if (*read_message != *read_transaction) {
        ESP_LOGE("otgw",
          "Received %s transaction contains conflicting message type %d",
          *read_transaction ? "READ" : "WRITE", message->message_type
        );
      }
    }
  }

  if (!read_transaction) {
    ESP_LOGE("otgw", "Couldn't deduce message READ or write from transaction %d", data_type);
    return;
  }

  // Count the number of consecutive failures, this will then be used to determine if it should be
  // reported as unknown
  DataTypeInfo& info = _data_types[data_type];
  if (!supported) {
    info.consecutive_failures = min(info.consecutive_failures + 1, 10);
  } else {
    info.consecutive_failures = 0;
    info.supported = true;
  }

  if (info.readable_info) {
    info.readable_info->time_last_received = seconds();
  }
  if (_data_type_request && _data_type_request->data_type == data_type) {
    _data_type_request.reset();
  }

  // Some data types are commands, the heater will send the value back to the thermostat,
  // so we can get the relevant data from the heater response
  bool treat_as_read_transaction = (
    *read_transaction ||
    data_type == ControlSetpoint::ID ||
    data_type == ControlSetpoint2::ID ||
    data_type == MaxRelativeModulationLevel::ID ||
    data_type == DHWSetpoint::ID ||
    data_type == MaxCHWaterSetpoint::ID
  );

  if (data[Transaction::CH_RESPONSE]) {
    if ((
      !info.interest &&
      // Without these the heating might not function
      data_type != Status::ID &&
      data_type != SlaveConfiguration::ID &&
      data_type != ControlSetpoint::ID &&
      data_type != ControlSetpoint2::ID &&
      data_type != BoilerFlowWaterTemperature::ID &&
      data_type != FlowTemperatureCH2::ID &&
      data_type != MaxRelativeModulationLevel::ID &&
      data_type != RelativeModulationLevel::ID &&
      data_type != RemoteRequest::ID &&
      data_type != CoolingControl::ID &&
      (data_type != RemoteOverrideRoomSetpoint::ID || _ignore_heater_overrides) &&
      (data_type != RemoteOverrideRoomSetpoint2::ID || _ignore_heater_overrides)
    ) || info.consecutive_failures >= 3) {
      // Tell the gateway that we are not interested in this data type
      std::string data_type_as_str = std::to_string(data_type);
      queue_command("UI", data_type_as_str);
      queue_command("DA", data_type_as_str);
      info.supported = false;
    }
  }

  if (treat_as_read_transaction) {
    if (!supported) {
      return;
    }

    auto data_value = data[Transaction::CH_RESPONSE];
    if (data_value) {
      handle_slave_response(data_type, data_value->data);
    }
    data_value = data[Transaction::GA_RESPONSE];
    if (data_value) {
      handle_gateway_response(data_type, data_value->data);
    }
  } else {
    auto data_value = data[Transaction::TH_REQUEST];
    if (data_value) {
      handle_master_request(data_type, data_value->data);
    }
  }
}

void OpenthermGateway::parse_line(std::string const &line) {
  ESP_LOGD("otgw", "Received line %s", line.c_str());
  if (line.size() >= 3 && line[2] == ':') {
    parse_command_response(line);
    return;
  }

  if (_send_command) {
    if (_lines_since_command > 3) {
      ESP_LOGE("otgw", "Did not receive a reply to command (%s).", _send_command->c_str());
      if (_command_queue.size() < MAX_COMMAND_QUEUE_LENGTH)
        _command_queue.push_back(*_send_command);
      _send_command.reset();
    } else {
      _lines_since_command++;
    }
  }

  if (line.size() != 9) {
    ESP_LOGE("otgw", "Received line (%s) is not 9 characters", line.c_str());
    return;
  }

  unsigned long message = strtoul(line.substr(1, 8).c_str(), nullptr, 16);
  uint8_t message_type = (message >> 28) & 0b0111;
  uint8_t data_type = (message >> 16) & 0xFF;
  uint16_t data = message & 0xFFFF;
  ESP_LOGD("otgw", "  Data type %d: %s", data_type, Transaction::MESSAGE_TYPE[message_type]);

  Transaction::Step transaction_step;
  switch (line[0]) {
    case 'T':
      transaction_step = Transaction::TH_REQUEST;
      break;
    case 'R':
      transaction_step = Transaction::GA_REQUEST;
      break;
    case 'B':
      transaction_step = Transaction::CH_RESPONSE;
      break;
    case 'A':
      transaction_step = Transaction::GA_RESPONSE;
      break;
    default:
      ESP_LOGE("otgw", "Received line (%s) does not start with B, T, R, or A", line.c_str());
      return;
  }

  // Check if this is the start of a new transaction
  if (_current_transaction) {
    if (transaction_step <= _last_transaction_step) {
      handle_transaction(*_current_transaction);
      _current_transaction.reset();
    } else if (
      transaction_step == Transaction::CH_RESPONSE &&
      _current_transaction->slave_data_type != data_type
    ) {
      _current_transaction.reset();
      ESP_LOGE("otgw", "Type of line (%s) does not match that of transaction (%d)", line.c_str(), _current_transaction->slave_data_type);
    } else if (
      transaction_step == Transaction::GA_RESPONSE &&
      _current_transaction->master_data_type != data_type
    ) {
      _current_transaction.reset();
      ESP_LOGE("otgw", "Type of line (%s) does not match that of transaction (%d)", line.c_str(), _current_transaction->master_data_type);
    }
  }

  // If this is a new transaction
  if (!_current_transaction && transaction_step <= Transaction::GA_REQUEST) {
    _current_transaction = Transaction{};
    _current_transaction->slave_data_type = data_type;
    _current_transaction->master_data_type = data_type;
  }

  if (_current_transaction) {
    // The gateway can change the request to slot in for an unknown dataid
    if (transaction_step == Transaction::GA_REQUEST) {
      _current_transaction->slave_data_type = data_type;
    }

    _current_transaction->data[transaction_step] = Transaction::Message{Transaction::MessageType{message_type}, data};
  }

  _last_transaction_step = transaction_step;
}

void OpenthermGateway::loop() {
  if (!_send_command && !_command_queue.empty()) {
    _send_command = _command_queue[0];
    _command_queue.erase(_command_queue.begin());
    ESP_LOGD("otgw", "> %s", _send_command->c_str());
    write_str(_send_command->c_str());
    flush();
  } else if (!_send_command) { // Means queue is empty
    uint32_t current_time = seconds();

    // We do this here to avoid spamming the queue
    if (_heating_circuit_1)
      _heating_circuit_1->refresh(*this);
    if (_heating_circuit_2)
      _heating_circuit_2->refresh(*this);

    if (_data_type_request) {
      if (
        _data_type_request->time_of_request + DATA_TYPE_REQUEST_TIMEOUT < current_time ||
        _data_type_request->time_of_request > current_time // clock overflow
      ) {
        ESP_LOGD("otgw", "Did not receive data after PM=%d command", _data_type_request->data_type);
        _data_type_request.reset();
      }
    } else if (_ready_for_requests) {
      std::optional<DataTypeRequest> most_outdated_data_type;
      for (auto &item : _data_types) {
        if (!item.second.interest || !item.second.supported || !item.second.readable_info) {
          continue;
        }

        auto time_last_received = item.second.readable_info->time_last_received;
        if (!time_last_received) {
          most_outdated_data_type = DataTypeRequest{item.first, current_time};
          break;
        }
        
        auto time_of_next_request = *time_last_received + item.second.readable_info->interval * 60;
        if (!most_outdated_data_type || time_of_next_request < most_outdated_data_type->time_of_request) {
          most_outdated_data_type = DataTypeRequest{item.first, time_of_next_request};
        }
      }

      if (most_outdated_data_type) {
        uint8_t data_type = most_outdated_data_type->data_type;

        _data_type_request = DataTypeRequest{data_type, current_time};
        queue_command("PM", std::to_string(data_type));
      }
    }
  }

  read_available();
}

void OpenthermGateway::set_room_thermostat(OpenthermGatewayClimate *clim) {
  _room_thermostat = clim;

  auto callback = [this]() {
    char parameter[6];
    sprintf(parameter, "%2.2f", _room_thermostat->target_temperature);

    switch (_room_thermostat->mode) {
      case climate::ClimateMode::CLIMATE_MODE_HEAT:
        // TC makes the target temperature constant, the thermostat can't change it automatically
        queue_command("TC", parameter);
        break;
      case climate::ClimateMode::CLIMATE_MODE_AUTO:
        // TT makes the target temperature temporary, the thermostat can change it when it wants
        queue_command("TT", parameter);
        break;
      default:
        ESP_LOGE("otgw", "Invalid climate mode for room thermostat");
        break;
    }
  };

  _room_thermostat->set_callbacks(callback, callback);

  set_interest<Status>();
  set_interest<SlaveConfiguration>();
  set_interest<RoomSetpoint>();
  set_interest<RoomTemperature>();
}

void OpenthermGateway::set_hot_water(OpenthermGatewayClimate *clim) {
  _hot_water = clim;
  _hot_water->set_callbacks([this]() {
    // target callback
    char parameter[6];
    sprintf(parameter, "%2.2f", _hot_water->target_temperature);

    queue_command("SW", parameter);
    _data_type_request = DataTypeRequest{DHWSetpoint::ID, seconds()};
  }, [this]() {
    // mode callback
    switch (_hot_water->mode) {
      case climate::ClimateMode::CLIMATE_MODE_HEAT:
        queue_command("BW", "0");
        queue_command("HW", "1");
        break;
      case climate::ClimateMode::CLIMATE_MODE_AUTO:
        queue_command("BW", "0");
        queue_command("HW", "0");
        break;
      case climate::ClimateMode::CLIMATE_MODE_OFF:
        queue_command("HW", "0");
        queue_command("BW", "1");
        break;
      default:
        ESP_LOGE("otgw", "Invalid climate mode for hot water");
        break;
    }
  });

  set_interest<Status>();
  set_interest<SlaveConfiguration>();
  set_interest<BoilerFlowWaterTemperature>();
  set_interest<DHWTemperature>();
  set_interest<DHWSetpointBounds>();
  set_interest<DHWSetpoint>();
  set_interest<RemoteParameters>(); // Without this, OTGW won't send setpoints
}

void OpenthermGateway::set_heating_circuit_1(OpenthermGatewayClimate *clim) {
  _heating_circuit_1 = HeatingCircuit{0, clim, "CS", "CH"};
  _heating_circuit_1->set_callbacks(*this);

  set_interest<Status>();
  set_interest<ControlSetpoint>();
  set_interest<BoilerFlowWaterTemperature>();
  set_interest<MaxCHWaterSetpoint>();
  set_interest<CHSetpointBounds>();
}

void OpenthermGateway::set_heating_circuit_2(OpenthermGatewayClimate *clim) {
  _heating_circuit_2 = HeatingCircuit{0, clim, "C2", "H2"};
  _heating_circuit_2->set_callbacks(*this);

  set_interest<Status>();
  set_interest<SlaveConfiguration>();
  set_interest<ControlSetpoint2>();
  set_interest<FlowTemperatureCH2>();
  set_interest<MaxCHWaterSetpoint>();
  set_interest<CHSetpointBounds>();
}

void OpenthermGateway::reuse_master_slots(bool reuse_slots) {
  _reuse_master_slots = reuse_slots;
}

void OpenthermGateway::ignore_heater_overrides(bool ignore_overrides) {
  _ignore_heater_overrides = ignore_overrides;
}

void OpenthermGateway::set_outside_temperature_override(sensor::Sensor *sens) {
  _outside_temperature_override = sens;
  _outside_temperature_override->add_on_state_callback([this](float temperature) {
    char parameter[6];
    sprintf(parameter, "%2.2f", temperature);

    queue_command("OT", parameter);
  });
}

void OpenthermGateway::set_time_source(time::RealTimeClock *time) {
  _time_source = time;
  _time_source->add_on_time_sync_callback([this]() {
    auto now = _time_source->now();

    char parameter[12];
    sprintf(parameter, "%02d:%02d/%d", now.hour, now.minute, now.day_of_week);

    queue_command("SC", parameter);
  });
}

void OpenthermGateway::set_reset_service_request_button(OpenthermGatewayButton *butt) {
  _reset_service_request = butt;
  _reset_service_request->set_callback([this]() {
    queue_command("RR", "10");
    _data_type_request = DataTypeRequest{RemoteRequest::ID, seconds()};
  });
}

void OpenthermGateway::set_hot_water_push_button(OpenthermGatewayButton *butt) {
  _hot_water_push = butt;
  _hot_water_push->set_callback([this]() {
    queue_command("HW", "P");
  });
}

climate::ClimateAction OpenthermGateway::calculate_climate_action(bool flame, bool heating) {
  climate::ClimateAction action;
  if (!heating) {
    action = climate::CLIMATE_ACTION_OFF;
  } else if (flame) {
    action = climate::CLIMATE_ACTION_HEATING;
  } else {
    action = climate::CLIMATE_ACTION_IDLE;
  }
  return action;
}

bool OpenthermGateway::set_heater_climate_action(std::optional<HeatingCircuit> &heating_circuit, bool flame, bool heating) {
  if (heating_circuit) {
    auto action = calculate_climate_action(flame, heating);
    heating_circuit->_component->set_action(action);
    return true;
  }
  return false;
}

}  // namespace otgw
}  // namespace esphome
