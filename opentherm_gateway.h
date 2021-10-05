#ifndef OPENTHERM_GATEWAY_H
#define OPENTHERM_GATEWAY_H

#include "esphome.h"
#include <string>
#include <bitset>

class OpenthermGateway : public Component, public UARTDevice {

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

public:
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

    OpenthermGateway(UARTComponent *parent) : UARTDevice(parent) {}

    void setup() override {
        // Reset the PIC, useful when it is confused due to MCU serial weirdness during startup
        GPIOPin pic_reset(D5, OUTPUT);
        pic_reset.setup();
        pic_reset.digital_write(false);
        write_str("GW=R\r"); // prime for immediate sending
        delay(100);
        pic_reset.digital_write(true);
        pic_reset.pin_mode(INPUT);
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

    bool is_busy(std::string const &line) {
        if (line[0] == 'O' && line[1] == 'E')
            ESP_LOGD("otgw", "The processor was busy and failed to process all received characters");
        else
            return false;

        return true;
    }

    bool is_error(std::string const &line) {
        if (line[0] == 'N' && line[1] == 'G')
            ESP_LOGD("otgw", "The command code is unknown.");
        else if (line[0] == 'S' && line[1] == 'E')
            ESP_LOGD("otgw", "The command contained an unexpected character or was incomplete.");
        else if (line[0] == 'B' && line[1] == 'V')
            ESP_LOGD("otgw", "The command contained a data value that is not allowed.");
        else if (line[0] == 'O' && line[1] == 'R')
            ESP_LOGD("otgw", "A number was specified outside of the allowed range.");
        else if (line[0] == 'N' && line[1] == 'S')
            ESP_LOGD("otgw", "The alternative Data-ID could not be added because the table is full.");
        else if (line[0] == 'N' && line[1] == 'F')
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
                if (line[0] == command[0] && line[1] == command[1])
                    return true;   

                if (is_busy(line))
                    break;

                if (is_error(line))
                    return false;
            } while(available());

        } while(retries++ < 5);

        return false;
    }

    bool set_room_setpoint(float temperature) {
        char parameter[6];
        sprintf(parameter, "%2.2f", temperature);

        return send_command("TC", parameter);
    }
    
    bool set_central_heating_setpoint(float temperature) {
        char parameter[6];
        sprintf(parameter, "%2.2f", temperature);

        return send_command("CS", parameter);
    }

    bool disable_central_heating() {
        return send_command("CH", "0");
    }
};

#endif
