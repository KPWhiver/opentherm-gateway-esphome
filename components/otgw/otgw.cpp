#include "otgw.h"

#include "climate.h"
#include "esphome/components/esp8266/gpio.h"

namespace esphome {
namespace otgw {

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
    ESP_LOGE("otgw", "The specified alternative Data-ID could not be removed because it does not exist in the table.");
  else if (command_code == "OE")
    ESP_LOGE("otgw", "The processor was busy and failed to process all received characters.");
  else
    return false;

  return true;
}

bool OpenthermGateway::queue_command(char const *command, char const *parameter) {
  if (_command_queue.size() == MAX_COMMAND_QUEUE_LENGTH) {
    ESP_LOGE("otgw", "Failed to send %s=%s because the queue is full", command, parameter);
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
  queue_command("PM", "125");
  queue_command("PR", "A");
  queue_command("PR", "B");
  queue_command("PR", "Q");
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
    ESP_LOGD("otgw", "%s", line.substr(3).c_str());
  } else {
    // Otherwise we received a confirmation that the command was succesfull.
  }
  _send_command.reset();
}

void OpenthermGateway::parse_line(std::string const &line) {
  ESP_LOGD("otgw", "< %s", line.c_str());
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

  switch (line[0]) {
    case 'B':
    case 'T':
    case 'R':
    case 'A':
      break;
    default:
      return;
  }

  unsigned long message = strtoul(line.substr(1, 8).c_str(), nullptr, 16);
  uint8_t message_type = (message >> 28) & 0b0111;
  uint8_t data_type = (message >> 16) & 0xFF;
  uint16_t data = message & 0xFFFF;
  uint8_t high_data = (message >> 8) & 0xFF;
  uint8_t low_data = message & 0xFF;

  switch (message_type) {
    // Master -> Slave (thermostat -> boiler)
    case 0b0000:           // READ
      if (data_type == 0)  // The 0 message contains interesting info in the READ (master bits)
        break;
      return;
    case 0b0001:  // WRITE
      break;
    case 0b0010:  // INVALID-DATA
      ESP_LOGE("otgw", "MSG %d: invalid data", data_type);
      return;
    case 0b0011:  // reserved
      return;

    // Slave -> Master (boiler -> thermostat)
    case 0b0100:  // READ-ACK
      break;
    case 0b0101:  // WRITE-ACK
      return;
    case 0b0110:  // DATA-INVALID
      ESP_LOGE("otgw", "MSG %d: data invalid", data_type);
      return;
    case 0b0111:  // UNKNOWN-DATAID
      ESP_LOGE("otgw", "MSG %d: unknown dataid", data_type);
      return;
    default:
      return;  // Does not contain interesting data
  }

  switch (data_type) {
    case 0: {
      if (line[0] == 'A' || line[0] == 'R')  // Fake response/request
        break;
      if (line[0] == 'T') {  // Command from the thermostat
        break;
      }

      // Master state
      std::bitset<8> master_bits(high_data);

      bool master_central_heating_1 = master_bits[0];
      if (_room_thermostat != nullptr) {
        _room_thermostat->set_action(master_central_heating_1 ? climate::CLIMATE_ACTION_HEATING
                                                              : climate::CLIMATE_ACTION_IDLE);
      }

      bool master_water_heating = master_bits[1];
      if (_hot_water != nullptr) {
        _hot_water->set_mode(master_water_heating ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF);
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
      set_heating_circuit_action(_heating_circuit_1, slave_flame, central_heating_1);

      bool central_heating_2 = slave_bits[5];
      set_heating_circuit_action(_heating_circuit_2, slave_flame, central_heating_2);

      bool water_heating = slave_bits[2];
      if (_hot_water != nullptr) {
        _hot_water->set_action(calculate_climate_action(slave_flame, water_heating));
      }

      this->slave_fault.publish_state(slave_bits[0]);
      this->slave_central_heating_1.publish_state(central_heating_1);
      this->slave_water_heating.publish_state(water_heating);
      this->slave_flame.publish_state(slave_flame);
      this->slave_cooling.publish_state(slave_bits[4]);
      this->slave_central_heating_2.publish_state(central_heating_2);
      this->slave_diagnostic_event.publish_state(slave_bits[6]);
      break;
    }
    case 3: {
      std::bitset<8> slave_configuration(high_data);
      if (_room_thermostat != nullptr) {
        _room_thermostat->set_cooling_supported(slave_configuration[2]);
      }

      if (_hot_water != nullptr) {
        _hot_water->set_internal(!slave_configuration[0]);
      }

      bool modulation_supported = !slave_configuration[1];
      bool hot_water_tank = slave_configuration[3];
      if (_heating_circuit_2) {
        _heating_circuit_2->heating_circuit->set_internal(!slave_configuration[5]);
      }
    }
    // Faults
    case 5: {
      std::bitset<8> bits(high_data);
      this->service_required.publish_state(bits[0]);
      this->lockout_reset.publish_state(bits[1]);
      this->low_water_pressure.publish_state(bits[2]);
      this->gas_flame_fault.publish_state(bits[3]);
      this->air_pressure_fault.publish_state(bits[4]);
      this->water_overtemperature.publish_state(bits[5]);
      break;
    }
    // Setpoints
    case 57: {
      double temperature = parse_float(data);
      this->max_central_heating_setpoint.publish_state(temperature);
      if (_heating_circuit_1) {
        _heating_circuit_1->heating_circuit->set_max_temperature(temperature);
      }
      if (_heating_circuit_2) {
        _heating_circuit_2->heating_circuit->set_max_temperature(temperature);
      }
      break;
    }
    case 1: {
      float temperature = parse_float(data);
      this->central_heating_setpoint_1.publish_state(temperature);

      if (line[0] != 'T') {
        set_heating_circuit_target_temperature(_heating_circuit_1, temperature);
      }
      break;
    }
    case 8: {
      float temperature = parse_float(data);
      this->central_heating_setpoint_2.publish_state(temperature);

      if (line[0] != 'T') {
        set_heating_circuit_target_temperature(_heating_circuit_2, temperature);
      }
      break;
    }
    case 56: {
      float temperature = parse_float(data);
      this->hot_water_setpoint.publish_state(temperature);

      if (_hot_water != nullptr) {
        _hot_water->set_target_temperature(temperature);
      }
      break;
    }
    case 9:
      if (line[0] != 'T') {
        this->remote_override_room_setpoint.publish_state(parse_float(data));
      }
      break;
    case 16: {
      float temperature = parse_float(data);
      this->room_setpoint_1.publish_state(temperature);

      if (_room_thermostat != nullptr) {
        _room_thermostat->set_target_temperature(temperature);
      }
      break;
    }
    case 23:
      this->room_setpoint_2.publish_state(parse_float(data));
      break;

    // Temperatures
    case 25: {
      float temperature = parse_float(data);
      this->central_heating_temperature_1.publish_state(temperature);

      if (_heating_circuit_1) {
        _heating_circuit_1->heating_circuit->set_current_temperature(temperature);
      }

      // Some boilers do not report the hot water temperature. In that case we can take the overall temperature.
      if (_hot_water && !_hot_water_temperature_reported) {
        _hot_water->set_current_temperature(temperature);
      }
      break;
    }
    case 31: {
      float temperature = parse_float(data);
      this->central_heating_temperature_2.publish_state(temperature);

      if (_heating_circuit_2) {
        _heating_circuit_2->heating_circuit->set_current_temperature(temperature);
      }
      break;
    }
    case 26: {
      float temperature = parse_float(data);
      this->hot_water_temperature_1.publish_state(temperature);

      _hot_water_temperature_reported = true;

      if (_hot_water != nullptr) {
        _hot_water->set_current_temperature(temperature);
      }
      break;
    }
    case 32:
      this->hot_water_temperature_2.publish_state(parse_float(data));
      break;
    case 24: {
      float temperature = parse_float(data);
      this->room_temperature.publish_state(temperature);

      if (_room_thermostat != nullptr) {
        _room_thermostat->set_current_temperature(temperature);
      }
      break;
    }
    case 27:
      this->outside_temperature.publish_state(parse_float(data));
      break;
    case 28:
      this->return_water_temperature.publish_state(parse_float(data));
      break;
    case 29:
      this->solar_storage_temperature.publish_state(parse_float(data));
      break;
    case 30:
      this->solar_collector_temperature.publish_state(parse_int16(data));
      break;
    case 33:
      this->exhaust_temperature.publish_state(parse_int16(data));
      break;

    // Modulation
    case 14:
      this->max_relative_modulation_level.publish_state(parse_float(data));
      break;
    case 15:
      this->max_boiler_capacity.publish_state(high_data);
      this->min_modulation_level.publish_state(low_data);
      break;
    case 17:
      this->relative_modulation_level.publish_state(parse_float(data));
      break;

    // Water
    case 18:
      this->central_heating_water_pressure.publish_state(parse_float(data));
      break;
    case 19:
      this->hot_water_flow_rate.publish_state(parse_float(data));
      break;

    // Starts
    case 116:
      this->central_heating_burner_starts.publish_state(data);
      break;
    case 117:
      this->central_heating_pump_starts.publish_state(data);
      break;
    case 118:
      this->hot_water_pump_starts.publish_state(data);
      break;
    case 119:
      this->hot_water_burner_starts.publish_state(data);
      break;

    // Operation hours
    case 120:
      this->central_heating_burner_operation_time.publish_state(data);
      break;
    case 121:
      this->central_heating_pump_operation_time.publish_state(data);
      break;
    case 122:
      this->hot_water_pump_operation_time.publish_state(data);
      break;
    case 123:
      this->hot_water_burner_operation_time.publish_state(data);
      break;

    // Config
    case 124:
      this->master_opentherm_version.publish_state(std::to_string(parse_float(data)));
      break;
    case 125:
      this->slave_opentherm_version.publish_state(std::to_string(parse_float(data)));
      break;

    default:
      ESP_LOGW("otgw", "Unsupported data id: %d (%s)", data_type, line.c_str());
      break;
  }
}

void OpenthermGateway::loop() {
  if (!_send_command && !_command_queue.empty()) {
    _send_command = _command_queue[0];
    _command_queue.erase(_command_queue.begin());
    ESP_LOGD("otgw", "> %s", _send_command->c_str());
    write_str(_send_command->c_str());
    flush();
  }

  read_available();

  // Send heater commands
  refresh_heating_circuit_setpoint(_heating_circuit_1);
  refresh_heating_circuit_setpoint(_heating_circuit_2);
}

bool OpenthermGateway::refresh_heating_circuit_setpoint(optional<HeatingCircuit> &heating_circuit) {
  uint64_t now = millis();
  if (heating_circuit &&
      (now < heating_circuit->time_of_last_command || now - heating_circuit->time_of_last_command > 50'000)) {
    optional<float> target_temperature;
    if (heating_circuit->heating_circuit->mode == climate::CLIMATE_MODE_HEAT) {
      target_temperature = heating_circuit->heating_circuit->target_temperature;
    }
    return set_heating_circuit_setpoint(*heating_circuit, target_temperature);
  }
  return false;
}

void OpenthermGateway::set_room_thermostat(OpenthermGatewayClimate *clim) {
  _room_thermostat = clim;
  _room_thermostat->set_callback([=](optional<float> state) {
    if (state) {
      char parameter[6];
      sprintf(parameter, "%2.2f", *state);

      return queue_command("TC", parameter);
    }

    return false;
  });
}

void OpenthermGateway::set_hot_water(OpenthermGatewayClimate *clim) {
  _hot_water = clim;
  _hot_water->set_callback([=](optional<float> state) {
    if (state) {
      char parameter[6];
      sprintf(parameter, "%2.2f", *state);

      return queue_command("HW", "1") && queue_command("SW", parameter);
    } else {
      return queue_command("HW", "0");
    }
  });
}

void OpenthermGateway::set_heating_circuit_1(OpenthermGatewayClimate *clim) {
  _heating_circuit_1 = HeatingCircuit{0, clim, "CS", "CH"};

  _heating_circuit_1->heating_circuit->set_callback(
      [=](optional<float> state) { set_heating_circuit_setpoint(*_heating_circuit_1, state); });
}

void OpenthermGateway::set_heating_circuit_2(OpenthermGatewayClimate *clim) {
  _heating_circuit_2 = HeatingCircuit{0, clim, "C2", "H2"};

  _heating_circuit_2->heating_circuit->set_callback(
      [=](optional<float> state) { set_heating_circuit_setpoint(*_heating_circuit_2, state); });
}

void OpenthermGateway::set_override_thermostat(bool value) { _override_thermostat = value; }

void OpenthermGateway::set_outside_temperature_override(sensor::Sensor *sens) {
  _outside_temperature_override = sens;
  _outside_temperature_override->add_on_state_callback([=](float temperature) {
    char parameter[6];
    sprintf(parameter, "%2.2f", temperature);

    queue_command("OT", parameter);
  });
}

void OpenthermGateway::set_time_source(time::RealTimeClock *time) {
  _time_source = time;
  _time_source->add_on_time_sync_callback([=]() {
    auto now = _time_source->now();

    char parameter[12];
    sprintf(parameter, "%02d:%02d/%d", now.hour, now.minute, now.day_of_week);

    queue_command("SC", parameter);
  });
}

void OpenthermGateway::set_reset_service_request_button(OpenthermGatewayButton *butt) {
  _reset_service_request = butt;
  _reset_service_request->set_callback([=]() {
    queue_command("RR", "10");
  });
}

void OpenthermGateway::set_hot_water_push_button(OpenthermGatewayButton *butt) {
  _hot_water_push = butt;
  _hot_water_push->set_callback([=]() {
    queue_command("HW", "P");
  });
}

bool OpenthermGateway::set_heating_circuit_setpoint(HeatingCircuit &heating_circuit, optional<float> temperature) {
  heating_circuit.time_of_last_command = millis();

  if (!temperature) {
    if (_override_thermostat) {
      return queue_command(heating_circuit.temp_command, "5.00") &&
             queue_command(heating_circuit.enable_command, "0");  // By setting to 5 the thermostat is ignored
    }

    return queue_command(heating_circuit.temp_command, "0");
  } else {
    char parameter[6];
    sprintf(parameter, "%2.2f", *temperature);
    return queue_command(heating_circuit.temp_command, parameter) && queue_command(heating_circuit.enable_command, "1");
  }
}

bool OpenthermGateway::set_heating_circuit_target_temperature(optional<HeatingCircuit> &heating_circuit,
                                                              float temperature) {
  if (heating_circuit) {
    heating_circuit->heating_circuit->set_target_temperature(temperature);
    return true;
  }
  return false;
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

bool OpenthermGateway::set_heating_circuit_action(optional<HeatingCircuit> &heating_circuit, bool flame, bool heating) {
  if (heating_circuit) {
    auto action = calculate_climate_action(flame, heating);
    heating_circuit->heating_circuit->set_action(action);
    return true;
  }
  return false;
}

}  // namespace otgw
}  // namespace esphome
