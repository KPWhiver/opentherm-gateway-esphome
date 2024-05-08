#include "climate.h"

#include "esphome/core/hal.h"

namespace esphome {
namespace floor_heating {

void FloorHeatingClimate::setup() {
    // Initial state
    this->target_temperature = 0; // to stop the system from starting heating immediately

    this->mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
    this->publish_state();

    // Callbacks
    _temperature->add_on_state_callback([=](float value) {
        this->current_temperature = value;
        this->calculate_average_temperature_change();
        this->calculate_central_heating_setpoint();

        this->publish_state();
    });
    _heater_temperature->add_on_state_callback([=](float) {
        this->calculate_central_heating_setpoint();
    });
    _heater_active->add_on_state_callback([=](bool) {
        this->calculate_central_heating_setpoint();
    });
    if (_outside_temperature != nullptr) {
        _outside_temperature->add_on_state_callback([=](float) {
            this->calculate_central_heating_setpoint();
        });
    }
    if (_heater_return_temperature != nullptr) {
        _heater_return_temperature->add_on_state_callback([=](float) {
            this->calculate_central_heating_setpoint();
        });
    }
}

void FloorHeatingClimate::calculate_average_temperature_change() {
    // Insert temperature into history
    if (!_oldest_temperature_index) {
        _temperature_history.fill(this->current_temperature);
        _oldest_temperature_index = 0;
    } else {
        _temperature_history[*_oldest_temperature_index] = this->current_temperature;
        _oldest_temperature_index = (*_oldest_temperature_index + 1) % _temperature_history.size();
    }

    // Calculate average temperature change
    float temperature_history_sum = 0;
    for (float temperature : _temperature_history)
        temperature_history_sum += temperature;

    float new_average_temperature = temperature_history_sum / _temperature_history.size();
    if (_average_temperature)
        _average_temperature_change = new_average_temperature - *_average_temperature;
    _average_temperature = new_average_temperature;

    // Calculate the time since the last temperature measurement
    unsigned long current_time = millis();
    unsigned long time_since_last_temperature = 60000; // Decent default
    if (_time_of_last_temperature > 0 && current_time > _time_of_last_temperature) {
        time_since_last_temperature = current_time - _time_of_last_temperature;
    }
    _time_of_last_temperature = current_time;

    float time_factor = _lookahead_time_ms / float(time_since_last_temperature);
    _predicted_temperature = *_average_temperature + time_factor * _average_temperature_change;
}

void FloorHeatingClimate::calculate_central_heating_setpoint() {
    bool request_heating = false;
    if (this->mode == climate::ClimateMode::CLIMATE_MODE_OFF) {
        this->action = climate::ClimateAction::CLIMATE_ACTION_OFF;
    } else if (_predicted_temperature < this->target_temperature) {
        this->action = climate::ClimateAction::CLIMATE_ACTION_HEATING;
        request_heating = true;
    } else {
        this->action = climate::ClimateAction::CLIMATE_ACTION_IDLE;
    }

    float central_heating_setpoint = _max_heater_temperature_setpoint;

    // Check if we should stop heating
    unsigned long current_time = millis();
    unsigned long time_since_start_heating = current_time - _time_of_start_heating;
    unsigned long const minute = 60000;

    if (_previously_heating && !request_heating) {
        bool currently_heating = _heater_active->state || _heater_temperature->state > central_heating_setpoint;

        if (time_since_start_heating < 5 * minute) { // Heat for at least 5 minutes
            request_heating = true;
        } else if (currently_heating && time_since_start_heating < 15 * minute) {
            request_heating = true;
        }
    } else if (!_previously_heating && request_heating) {
        _time_of_start_heating = current_time;
    }

    if (!request_heating) {
        _idle_trigger->trigger();
    } else {
        _heat_trigger->trigger();
    }

    _previously_heating = request_heating;

    this->publish_state();
}

void FloorHeatingClimate::control(climate::ClimateCall const &call) {
    if (call.get_mode().has_value()) {
        this->mode = *call.get_mode();
        this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
        this->target_temperature = *call.get_target_temperature();
        this->publish_state();
    }
}

climate::ClimateTraits FloorHeatingClimate::traits() {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
    traits.set_supports_action(true);
    traits.set_visual_min_temperature(0);
    traits.set_visual_max_temperature(30);
    traits.set_visual_temperature_step(0.01);
    return traits;
}

}
}
