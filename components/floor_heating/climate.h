#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/climate/climate.h"

#include <array>
#include <algorithm>

namespace esphome {
namespace floor_heating {

class FloorHeatingClimate : public Component, public climate::Climate {
    // History of room temperatures, used to calculate a smoothed derivative
    std::array<float, 10> _temperature_history;
    esphome::optional<float> _average_temperature;
    float _average_temperature_change = 0;
    float _predicted_temperature = 100; // To stop the system from starting heating immediately
    esphome::optional<uint16_t> _oldest_temperature_index;
    unsigned long _time_of_last_temperature = 0;

    unsigned long _time_of_start_heating = 0;
    bool _previously_heating = false;

    float const _temperature_change_per_degree_per_lookahead = 2;
    unsigned long const _lookahead_time_ms = 5400.0 * 1000.0;

    sensor::Sensor *_temperature{nullptr};
    sensor::Sensor *_heater_temperature{nullptr};
    sensor::Sensor *_outside_temperature{nullptr};
    sensor::Sensor *_heater_return_temperature{nullptr};

    binary_sensor::BinarySensor *_heater_active{nullptr};

    Trigger<> *_idle_trigger{nullptr};
    Trigger<> *_heat_trigger{nullptr};

    float _default_target_temperature;
    float _max_heater_temperature_setpoint = 55;
    float _min_heater_temperature_setpoint = 25;

public:
    void set_temperature(sensor::Sensor* sens) { _temperature = sens; }
    void set_outside_temperature(sensor::Sensor* sens) { _outside_temperature = sens; }
    void set_heater_return_temperature(sensor::Sensor* sens) { _heater_return_temperature = sens; }

    void set_heater_active(binary_sensor::BinarySensor* sens) { _heater_active = sens; }
    void set_heater_temperature(sensor::Sensor* sens) { _heater_temperature = sens; }

    Trigger<> *get_idle_trigger() const { return _idle_trigger; }
    Trigger<> *get_heat_trigger() const { return _heat_trigger; }

    void set_default_target_temperature(float value) { _default_target_temperature = value; }
    void set_max_heater_temperature_setpoint(float value) { _max_heater_temperature_setpoint = value; }
    void set_min_heater_temperature_setpoint(float value) { _min_heater_temperature_setpoint = value; }

    float heater_temperature_setpoint() const { return _max_heater_temperature_setpoint; }

public:
    FloorHeatingClimate() :
        _idle_trigger(new Trigger<>()),
        _heat_trigger(new Trigger<>()) {}

    void setup() override;

    void calculate_average_temperature_change();

    void calculate_central_heating_setpoint();

    void control(climate::ClimateCall const &call) override;

    climate::ClimateTraits traits() override;
};

}
}
