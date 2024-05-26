#pragma once

#include "climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/time/real_time_clock.h"

#include <string>
#include <bitset>
#include <vector>

namespace esphome {
namespace otgw {

template<typename ComponentType>
class OptionalComponent {
    ComponentType* component{nullptr};

public:
    void set(ComponentType* comp) {
        component = comp;
    }

    void publish_state(decltype(component->state) state) {
        if (component != nullptr) {
            component->publish_state(state);
        }
    }
};

class OpenthermGateway : public Component, public uart::UARTDevice {

    struct HeatingCircuit {
        uint64_t time_of_last_command;
        OpenthermGatewayClimate* heating_circuit;
        const char* temp_command;
        const char* enable_command;
    };

    ///// Components /////
    OpenthermGatewayClimate* _room_thermostat{nullptr};
    OpenthermGatewayClimate* _hot_water{nullptr};
    optional<HeatingCircuit> _heating_circuit_1;
    optional<HeatingCircuit> _heating_circuit_2;

    sensor::Sensor* _outside_temperature_override{nullptr};
    time::RealTimeClock* _time_source{nullptr};

    bool _override_thermostat{false};

public:
    OptionalComponent<text_sensor::TextSensor> slave_opentherm_version;
    OptionalComponent<text_sensor::TextSensor> master_opentherm_version;

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

    void set_room_thermostat(OpenthermGatewayClimate* clim);
    void set_hot_water(OpenthermGatewayClimate* clim);
    void set_heating_circuit_1(OpenthermGatewayClimate* clim);
    void set_heating_circuit_2(OpenthermGatewayClimate* clim);
    void set_override_thermostat(bool value);
    void set_outside_temperature_override(sensor::Sensor* sens);
    void set_time_source(time::RealTimeClock* time);

private:
    std::string const &readline();
    float parse_float(uint16_t data);
    int16_t parse_int16(uint16_t data);
    int8_t parse_int8(uint8_t data);
    bool is_busy(char first, char second);
    bool is_error(char first, char second);
    bool send_command(char const *command, char const *parameter);

    climate::ClimateAction calculate_climate_action(bool flame, bool heating);
    bool set_room_setpoint(float temperature);
    bool set_heating_circuit_setpoint(HeatingCircuit& heating_circuit, optional<float> temperature);
    bool refresh_heating_circuit_setpoint(optional<HeatingCircuit>& heating_circuit);
    bool set_heating_circuit_target_temperature(optional<HeatingCircuit>& heating_circuit, float temperature);
    bool set_heating_circuit_action(optional<HeatingCircuit>& heating_circuit, bool flame, bool heating);

public:
    OpenthermGateway(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

    void setup() override;
    void loop() override;

};

}
}
