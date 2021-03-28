#include "esphome.h"
#include <string>
#include <bitset>

class OpenthermGateway : public Component, public UARTDevice {
public:
    std::string file_content;
    size_t file_index = 0;


    std::string readline() {
        std::string buffer(9, ' ');
        uint32_t read_length = 0;

        while (true) {
            char c = read();
            if (c <= 0) // ignore
                continue;

            if (c == '\r' || c == '\n') { // stop
                if (read_length != 9)
                    return "";
                return buffer;
            }

            if (read_length < 9)
                buffer[read_length] = c;

            ++read_length;
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

public:
    // Master state
    Sensor *s_master_central_heating_1 = new Sensor();
    Sensor *s_master_water_heating = new Sensor();
    Sensor *s_master_cooling = new Sensor();
    Sensor *s_master_outside_temperature_compensation = new Sensor();
    Sensor *s_master_central_heating_2 = new Sensor();

    // Slave state
    Sensor *s_slave_fault = new Sensor();
    Sensor *s_slave_central_heating_1 = new Sensor();
    Sensor *s_slave_water_heating = new Sensor();
    Sensor *s_slave_flame = new Sensor();
    Sensor *s_slave_cooling = new Sensor();
    Sensor *s_slave_central_heating_2 = new Sensor();
    Sensor *s_slave_diagnostic_event = new Sensor();

    // Faults
    Sensor *s_service_required = new Sensor();
    Sensor *s_lockout_reset = new Sensor();
    Sensor *s_low_water_pressure = new Sensor();
    Sensor *s_gas_flame_fault = new Sensor();
    Sensor *s_air_pressure_fault = new Sensor();
    Sensor *s_water_overtemperature = new Sensor();

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

    OpenthermGateway(UARTComponent *parent) : UARTDevice(parent) {}

    void setup() {
        /*s_service_required->publish_state(0);
        s_lockout_reset->publish_state(0);
        s_low_water_pressure->publish_state(0);
        s_gas_flame_fault->publish_state(0);
        s_air_pressure_fault->publish_state(0);
        s_water_overtemperature->publish_state(0);*/
    }

    void loop() { // override {
        while (available()) {
            std::string line = readline();

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
                case 0b0001: // WRITE
                    break;
                case 0b0100: // READ-ACK
                    break;
                default:
                    continue; // Does not contain interesting data
            }

            switch (data_type) {
                case 0: {
                    // Master state
                    std::bitset<8> master_bits(high_data);
                    s_master_central_heating_1->publish_state(master_bits[0]);
                    s_master_water_heating->publish_state(master_bits[1]);
                    s_master_cooling->publish_state(master_bits[2]);
                    s_master_outside_temperature_compensation->publish_state(master_bits[3]);
                    s_master_central_heating_2->publish_state(master_bits[4]);

                    // Slave state
                    std::bitset<8> slave_bits(low_data);
                    s_slave_fault->publish_state(slave_bits[0]);
                    s_slave_central_heating_1->publish_state(slave_bits[1]);
                    s_slave_water_heating->publish_state(slave_bits[2]);
                    s_slave_flame->publish_state(slave_bits[3]);
                    s_slave_cooling->publish_state(slave_bits[4]);
                    s_slave_central_heating_2->publish_state(slave_bits[5]);
                    s_slave_diagnostic_event->publish_state(slave_bits[6]);
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
                case 1:
                    s_central_heating_setpoint_1->publish_state(parse_float(data));
                    break;
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
                case 25:
                    s_central_heating_temperature->publish_state(parse_float(data));
                    break;
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
            }
        }
    }
};
