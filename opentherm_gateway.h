#ifndef OPENTHERM_GATEWAY_H
#define OPENTHERM_GATEWAY_H

#include "esphome.h"
#include <string>
#include <bitset>

class OpenthermGateway : public Component, public UARTDevice {

    bool _thermostat_heating_requested = false;
    bool _secondary_heating_requested = false;
    esphome::optional<float> _primary_heating_requested;

    bool _primary_heating_override = false;

    bool _flame = false;
    float _water_temperature = 0;
    float _water_temperature_change = 0;
    enum class HeatingCircuit {
        PRIMARY,
        SECONDARY,
        TAP_WATER,
        NONE
    };
    HeatingCircuit _active_heating_circuit = HeatingCircuit::NONE;
    HeatingCircuit _requested_heating_circuit = HeatingCircuit::NONE;

public:
    TextSensor *s_heater_state = new TextSensor();
    TextSensor *s_last_command = new TextSensor();

    // Master state
    BinarySensor *s_master_central_heating_1 = new BinarySensor();
    BinarySensor *s_master_water_heating = new BinarySensor();
    BinarySensor *s_master_cooling = new BinarySensor();
    BinarySensor *s_master_outside_temperature_compensation = new BinarySensor();
    BinarySensor *s_master_central_heating_2 = new BinarySensor();

    // Slave state
    BinarySensor *s_slave_fault = new BinarySensor();
    BinarySensor *s_slave_central_heating_1 = new BinarySensor();
    BinarySensor *s_slave_water_heating = new BinarySensor();
    BinarySensor *s_slave_flame = new BinarySensor();
    BinarySensor *s_slave_cooling = new BinarySensor();
    BinarySensor *s_slave_central_heating_2 = new BinarySensor();
    BinarySensor *s_slave_diagnostic_event = new BinarySensor();

    // Faults
    BinarySensor *s_service_required = new BinarySensor();
    BinarySensor *s_lockout_reset = new BinarySensor();
    BinarySensor *s_low_water_pressure = new BinarySensor();
    BinarySensor *s_gas_flame_fault = new BinarySensor();
    BinarySensor *s_air_pressure_fault = new BinarySensor();
    BinarySensor *s_water_overtemperature = new BinarySensor();

    // Setpoints
    Sensor *s_max_central_heating_setpoint = new Sensor();
    Sensor *s_central_heating_setpoint_1 = new Sensor();
    Sensor *s_central_heating_setpoint_2 = new Sensor();
    Sensor *s_hot_water_setpoint = new Sensor();
    Sensor *s_remote_override_room_setpoint = new Sensor();
    Sensor *s_room_setpoint_1 = new Sensor();
    Sensor *s_room_setpoint_2 = new Sensor();

    // Temperatures
    Sensor *s_central_heating_temperature = new Sensor();
    Sensor *s_hot_water_1_temperature = new Sensor();
    Sensor *s_hot_water_2_temperature = new Sensor();
    Sensor *s_room_temperature = new Sensor();
    Sensor *s_outside_temperature = new Sensor();
    Sensor *s_return_water_temperature = new Sensor();
    Sensor *s_solar_storage_temperature = new Sensor();
    Sensor *s_solar_collector_temperature = new Sensor();
    Sensor *s_central_heating_2_flow_water_temperature = new Sensor();
    Sensor *s_exhaust_temperature = new Sensor();

    // Modulation
    Sensor *s_max_relative_modulation_level = new Sensor();
    Sensor *s_max_boiler_capacity = new Sensor();
    Sensor *s_min_modulation_level = new Sensor();
    Sensor *s_relative_modulation_level = new Sensor();

    // Water
    Sensor *s_central_heating_water_pressure = new Sensor();
    Sensor *s_hot_water_flow_rate = new Sensor();

    // Starts
    Sensor *s_central_heating_burner_starts = new Sensor();
    Sensor *s_central_heating_pump_starts = new Sensor();
    Sensor *s_hot_water_pump_starts = new Sensor();
    Sensor *s_hot_water_burner_starts = new Sensor();

    // Operation hours
    Sensor *s_central_heating_burner_operation_hours = new Sensor();
    Sensor *s_central_heating_pump_operation_hours = new Sensor();
    Sensor *s_hot_water_pump_operation_hours = new Sensor();
    Sensor *s_hot_Water_burner_operation_hours = new Sensor();

    // Thermostat
    Sensor *s_average_temperature = new Sensor();
    Sensor *s_average_temperature_change = new Sensor();
    Sensor *s_predicted_temperature = new Sensor();

private:
    std::string const &readline() {
        static uint32_t buffer_size = 128;
        static std::string buffer;
        buffer.clear();
        buffer.reserve(buffer_size);

        while (true) {
            char c = read();
            if (c <= 0 || c == '\r') // ignore
                continue;

            if (c == '\n') { // stop
                return buffer;
            }

            if (buffer.size() < buffer_size)
                buffer += c;
        }
    }

    float parse_float(uint16_t data) {
        return ((data & 0x8000) ? -(0x10000L - data) : data) / 256.0f;
    }

    int16_t parse_int16(uint16_t data) {
        return *reinterpret_cast<int16_t*>(&data);
    }

    int8_t parse_int8(uint8_t data) {
        return *reinterpret_cast<int8_t*>(&data);
    }

    bool is_busy(char first, char second) {
        if (first == 'O' && second == 'E')
            ESP_LOGD("otgw", "The processor was busy and failed to process all received characters");
        else
            return false;

        return true;
    }

    bool is_error(char first, char second) {
        if (first == 'N' && second == 'G')
            ESP_LOGD("otgw", "The command code is unknown.");
        else if (first == 'S' && second == 'E')
            ESP_LOGD("otgw", "The command contained an unexpected character or was incomplete.");
        else if (first == 'B' && second == 'V')
            ESP_LOGD("otgw", "The command contained a data value that is not allowed.");
        else if (first == 'O' && second == 'R')
            ESP_LOGD("otgw", "A number was specified outside of the allowed range.");
        else if (first == 'N' && second == 'S')
            ESP_LOGD("otgw", "The alternative Data-ID could not be added because the table is full.");
        else if (first == 'N' && second == 'F')
            ESP_LOGD("otgw", "The specified alternative Data-ID could not be removed because it does not exist in the table.");
        else
            return false;

        return true;
    }

    bool send_command(char const *command, char const *parameter) {
        uint32_t retries = 0;

        do {
            write_str(command);
            write_str("=");
            write_str(parameter);
            write_str("\r\n");
            flush();

            do {
                std::string const &line = readline();
                s_last_command->publish_state(std::string(command) + "=" + parameter + " | " + line);
                ESP_LOGD("otgw", "%s", line.c_str());

                if (is_busy(line[0], line[1]))
                    break;

                if (is_busy(line[4], line[5]))
                    break;

                if (is_error(line[0], line[1]))
                    return false;

                if (is_error(line[4], line[5]))
                    return false;

                if (line[0] == command[0] && line[1] == command[1]) {
                    return true;
                }
            } while(available());

        } while(retries++ < 5);

        return false;
    }

public:
    OpenthermGateway(UARTComponent *parent) : UARTDevice(parent) {}

    void setup() override {
        // Reset the PIC, useful when it is confused due to serial weirdness during startup
        esphome::esp8266::ESP8266GPIOPin pic_reset;
        pic_reset.set_pin(D5);
        pic_reset.set_inverted(false);
        pic_reset.set_flags(gpio::Flags::FLAG_OUTPUT);

        pic_reset.setup();
        pic_reset.digital_write(false);
        write_str("GW=R\r"); // prime for immediate sending
        delay(100);
        pic_reset.digital_write(true);
        pic_reset.pin_mode(gpio::Flags::FLAG_INPUT);
    }

    void loop() override {
        while (available()) {
            std::string const &line = readline();

            if (line.size() != 9) {
                ESP_LOGD("otgw", "Received line (%s) is not 9 characters", line.data());
                continue;
            }

            switch (line[0]) {
                case 'B':
                case 'T':
                case 'R':
                case 'A':
                    break;
                default:
                    continue;
            }

            unsigned long message = strtoul(line.substr(1,8).c_str(), nullptr, 16);
            uint8_t message_type = (message >> 28) & 0b0111;
            uint8_t data_type = (message >> 16) & 0xFF;
            uint16_t data = message & 0xFFFF;
            uint8_t high_data = (message >> 8) & 0xFF;
            uint8_t low_data = message & 0xFF;

            switch (message_type) {
                // Master -> Slave (thermostat -> boiler)
                case 0b0000: // READ
                    if (data_type == 0) // The 0 message contains interesting info in the READ (master bits)
                        break;
                    continue;
                case 0b0001: // WRITE
                    break;
                case 0b0010: // INVALID-DATA
                    ESP_LOGD("otgw", "MSG %d: invalid data", data_type);
                    continue;
                case 0b0011: // reserved
                    continue;

                // Slave -> Master (boiler -> thermostat)
                case 0b0100: // READ-ACK
                    break;
                case 0b0101: // WRITE-ACK
                    continue;
                case 0b0110: // DATA-INVALID
                    ESP_LOGD("otgw", "MSG %d: data invalid", data_type);
                    continue;
                case 0b0111: // UNKNOWN-DATAID
                    ESP_LOGD("otgw", "MSG %d: unknown dataid", data_type);
                    continue;
                default:
                    continue; // Does not contain interesting data
            }

            switch (data_type) {
                case 0: {
                    std::bitset<8> master_bits(high_data);
                    bool master_central_heating = master_bits[0];

                    if (line[0] == 'A' || line[0] == 'R') // Fake response/request
                        break;
                    if (line[0] == 'T') { // Command from the thermostat
                        _thermostat_heating_requested = master_central_heating;
                        break;
                    }

                    // Master state
                    s_master_central_heating_1->publish_state(master_central_heating);
                    s_master_water_heating->publish_state(master_bits[1]);
                    s_master_cooling->publish_state(master_bits[2]);
                    s_master_outside_temperature_compensation->publish_state(master_bits[3]);
                    s_master_central_heating_2->publish_state(master_bits[4]);

                    // Slave state
                    std::bitset<8> slave_bits(low_data);
                    bool slave_central_heating = slave_bits[1];
                    bool slave_water_heating = slave_bits[2];
                    s_slave_fault->publish_state(slave_bits[0]);
                    s_slave_central_heating_1->publish_state(slave_central_heating);
                    s_slave_water_heating->publish_state(slave_water_heating);
                    _flame = slave_bits[3];
                    s_slave_flame->publish_state(_flame);
                    s_slave_cooling->publish_state(slave_bits[4]);
                    s_slave_central_heating_2->publish_state(slave_bits[5]);
                    s_slave_diagnostic_event->publish_state(slave_bits[6]);

                    if (slave_water_heating) {
                        _active_heating_circuit = HeatingCircuit::TAP_WATER;
                        s_heater_state->publish_state("tap water");
                    } else if (slave_central_heating) {
                        if (_requested_heating_circuit == HeatingCircuit::SECONDARY) {
                            _active_heating_circuit = HeatingCircuit::SECONDARY;
                            s_heater_state->publish_state("secondary central heating");
                        } else {
                            _active_heating_circuit = HeatingCircuit::PRIMARY;
                            s_heater_state->publish_state("primary central heating");
                        }
                    } else {
                        _active_heating_circuit = HeatingCircuit::NONE;
                        s_heater_state->publish_state("idle");
                    }

                    break;
                }
                // Faults
                case 5: {
                    std::bitset<8> bits(high_data);
                    s_service_required->publish_state(bits[0]);
                    s_lockout_reset->publish_state(bits[1]);
                    s_low_water_pressure->publish_state(bits[2]);
                    s_gas_flame_fault->publish_state(bits[3]);
                    s_air_pressure_fault->publish_state(bits[4]);
                    s_water_overtemperature->publish_state(bits[5]);
                    break;
                }
                // Setpoints
                case 57:
                    s_max_central_heating_setpoint->publish_state(parse_float(data));
                    break;
                case 1: {
                    float requested_temperature = parse_float(data);
                    if (line[0] == 'T' && (_primary_heating_override || _active_heating_circuit == HeatingCircuit::SECONDARY))
                        break;

                    s_central_heating_setpoint_1->publish_state(requested_temperature);
                    break;
                }
                case 8:
                    s_central_heating_setpoint_2->publish_state(parse_float(data));
                    break;
                case 56:
                    s_hot_water_setpoint->publish_state(parse_float(data));
                    break;
                case 9:
                    s_remote_override_room_setpoint->publish_state(parse_float(data));
                    break;
                case 16:
                    s_room_setpoint_1->publish_state(parse_float(data));
                    break;
                case 23:
                    s_room_setpoint_2->publish_state(parse_float(data));
                    break;

                // Temperatures
                case 25: {
                    float new_water_temperature = parse_float(data);
                    _water_temperature_change = new_water_temperature - _water_temperature;
                    _water_temperature = new_water_temperature;

                    s_central_heating_temperature->publish_state(new_water_temperature);
                    break;
                }
                case 26:
                    s_hot_water_1_temperature->publish_state(parse_float(data));
                    break;
                case 32:
                    s_hot_water_2_temperature->publish_state(parse_float(data));
                    break;
                case 24:
                    s_room_temperature->publish_state(parse_float(data));
                    break;
                case 27:
                    s_outside_temperature->publish_state(parse_float(data));
                    break;
                case 28:
                    s_return_water_temperature->publish_state(parse_float(data));
                    break;
                case 29:
                    s_solar_storage_temperature->publish_state(parse_float(data));
                    break;
                case 30:
                    s_solar_collector_temperature->publish_state(parse_int16(data));
                    break;
                case 31:
                    s_central_heating_2_flow_water_temperature->publish_state(parse_float(data));
                    break;
                case 33:
                    s_exhaust_temperature->publish_state(parse_int16(data));
                    break;

                // Modulation
                case 14:
                    s_max_relative_modulation_level->publish_state(parse_float(data));
                    break;
                case 15:
                    s_max_boiler_capacity->publish_state(high_data);
                    s_min_modulation_level->publish_state(low_data);
                    break;
                case 17:
                    s_relative_modulation_level->publish_state(parse_float(data));
                    break;

                // Water
                case 18:
                    s_central_heating_water_pressure->publish_state(parse_float(data));
                    break;
                case 19:
                    s_hot_water_flow_rate->publish_state(parse_float(data));
                    break;

                // Starts
                case 116:
                    s_central_heating_burner_starts->publish_state(data);
                    break;
                case 117:
                    s_central_heating_pump_starts->publish_state(data);
                    break;
                case 118:
                    s_hot_water_pump_starts->publish_state(data);
                    break;
                case 119:
                    s_hot_water_burner_starts->publish_state(data);
                    break;

                // Operation hours
                case 120:
                    s_central_heating_burner_operation_hours->publish_state(data);
                    break;
                case 121:
                    s_central_heating_pump_operation_hours->publish_state(data);
                    break;
                case 122:
                    s_hot_water_pump_operation_hours->publish_state(data);
                    break;
                case 123:
                    s_hot_Water_burner_operation_hours->publish_state(data);
                    break;
                default:
                    ESP_LOGD("otgw", "Unknown data id: %d (%s)", data_type, line.c_str());
                    break;
            }
        }
    }

    void update_heating() {
        bool primary_heating_requested = (_primary_heating_override && _primary_heating_requested) ||
                                         (!_primary_heating_override && _thermostat_heating_requested);
        if (_secondary_heating_requested && !primary_heating_requested) {
            _requested_heating_circuit = HeatingCircuit::SECONDARY;
            send_command("CS", "55.00") && send_command("CH", "1"); // SECONDARY
        } else if (!_primary_heating_override) {
            _requested_heating_circuit = HeatingCircuit::PRIMARY;
            send_command("CS", "0"); // PRIMARY
        } else if (_primary_heating_requested) {
            _requested_heating_circuit = HeatingCircuit::PRIMARY;
            char parameter[6];
            sprintf(parameter, "%2.2f", *_primary_heating_requested);
            send_command("CS", parameter) && send_command("CH", "1"); // PRIMARY (OVERRIDE)
        } else {
            _requested_heating_circuit = HeatingCircuit::NONE;
            send_command("CS", "5.00") && send_command("CH", "0"); // NONE
        }
    }

    bool set_room_setpoint(float temperature) {
        char parameter[6];
        sprintf(parameter, "%2.2f", temperature);

        return send_command("TC", parameter);
    }

    void set_outside_temperature(float temperature) {
        s_outside_temperature->publish_state(temperature);
    }

    void set_secondary_heating(bool heat) {
        _secondary_heating_requested = heat;

        update_heating();
    }

    void set_primary_heating_override(esphome::optional<float> temperature) {
        _primary_heating_requested = temperature;
        _primary_heating_override = true;

        update_heating();
    }

    void disable_primary_heating_override() {
        _primary_heating_requested.reset();
        _primary_heating_override = false;

        update_heating();
    }
};

#endif
