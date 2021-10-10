#ifndef OPENTHERM_GATEWAY_THERMOSTAT_H
#define OPENTHERM_GATEWAY_THERMOSTAT_H

#include <array>
#include <algorithm>

#include "esphome.h"
#include "opentherm_gateway.h"

class OpenthermGatewayThermostat : public Component, public Climate {
    OpenthermGateway *_opentherm_gateway;

    // History of room temperatures, used to calculate a smoothed derivative
    std::array<float, 10> _temperature_history;
    float _temperature_average = 0;
    uint16_t _oldest_temperature_index = 0;
    unsigned long _time_of_last_temperature = 0;

    float const _start_central_heating_setpoint = 25;
    float _central_heating_setpoint = _start_central_heating_setpoint;
    float const _max_central_heating_setpoint = 45;
    float const _min_central_heating_setpoint = 20;
    float const _temperature_change_per_degree_per_lookahead = 2;
    unsigned long const _lookahead_time_ms = 2400.0 * 1000.0;

public:
    OpenthermGatewayThermostat(OpenthermGateway *opentherm_gateway) :
        _opentherm_gateway(opentherm_gateway) {

        _temperature_history.fill(0.f);

        this->mode = ClimateMode::CLIMATE_MODE_HEAT;
        this->publish_state();

        _opentherm_gateway->s_room_setpoint_1->add_on_state_callback([=](float room_setpoint) {
            this->target_temperature = room_setpoint;
            this->publish_state();
        });
        _opentherm_gateway->s_room_temperature->add_on_state_callback([=](float room_temperature) {
            if (this->mode == ClimateMode::CLIMATE_MODE_AUTO)
                this->calculate_central_heating_setpoint(this->current_temperature, room_temperature);

            this->current_temperature = room_temperature;
            this->publish_state();
        });

    }

    void setup() override {
    }

    float smoothed_temperature_difference(float new_temperature) {
        _temperature_history[_oldest_temperature_index] = new_temperature;
        _oldest_temperature_index = (_oldest_temperature_index + 1) % _temperature_history.size();
        
        float temperature_history_sum = std::accumulate(_temperature_history.begin(), _temperature_history.end(), 0);

        float new_temperature_average = temperature_history_sum / _temperature_history.size();
        float temperature_difference = new_temperature_average - _temperature_average;
        _temperature_average = new_temperature_average;

        return temperature_difference;
    }

    void calculate_central_heating_setpoint(float previous_temperature, float new_temperature) {
        float temperature_difference = smoothed_temperature_difference(new_temperature);

        // Calculate the time since the last temperature measurement
        unsigned long current_time = millis();
        if (_time_of_last_temperature == 0 || current_time < _time_of_last_temperature) {
            _time_of_last_temperature = current_time;
            return;
        }

        unsigned long time_since_last_temperature = current_time - _time_of_last_temperature;
        
        // Predict the temperature error
        float time_factor = _lookahead_time_ms / float(time_since_last_temperature);
        float predicted_temperature = _temperature_average + time_factor * temperature_difference;
        float predicted_temperature_error = this->target_temperature - predicted_temperature;

        // Calculate the new setpoint
        float central_heating_setpoint_change = predicted_temperature_error * (_temperature_change_per_degree_per_lookahead / time_factor);
        _central_heating_setpoint += central_heating_setpoint_change;

        // Clamp the setpoint
        _central_heating_setpoint = esphome::clamp(_central_heating_setpoint, _min_central_heating_setpoint, _max_central_heating_setpoint);

        // Check if the setpoint will soon drop below the min value, in which case: shut down
        if (central_heating_setpoint_change < 0 && _central_heating_setpoint - _min_central_heating_setpoint < std::abs(central_heating_setpoint_change)) {
            _central_heating_setpoint = _min_central_heating_setpoint;
            bool success = _opentherm_gateway->set_primary_heating_override(esphome::nullopt);
            if (!success) {
                ESP_LOGD("otgw", "Failed to stop heating");
                return;
            }
        }

        ESP_LOGD("otgw", "Setpoint: %f", _central_heating_setpoint);
        bool success = _opentherm_gateway->set_primary_heating_override(_central_heating_setpoint);
        if (!success) {
            ESP_LOGD("otgw", "Failed to set the central heating setpoint");
            return;
        }
    }

    void control(ClimateCall const &call) override {
        if (call.get_mode().has_value()) {
            auto new_mode = *call.get_mode();

            if (new_mode == ClimateMode::CLIMATE_MODE_OFF || new_mode == ClimateMode::CLIMATE_MODE_HEAT) {
                _opentherm_gateway->disable_primary_heating_override();
                _time_of_last_temperature = 0;
                _central_heating_setpoint = _start_central_heating_setpoint;
            }

            // Turning off is not supported
            this->mode = *call.get_mode();
            this->publish_state();
        }
        if (call.get_target_temperature().has_value()) {
            float target_temperature = *call.get_target_temperature();
            bool success = _opentherm_gateway->set_room_setpoint(target_temperature);
            if (!success) {
                ESP_LOGD("otgw", "Failed to set the room setpoint");
                return;
            }

            this->target_temperature = target_temperature;
            this->publish_state();
        }
    }

    ClimateTraits traits() override {
        auto traits = climate::ClimateTraits();
        traits.set_supports_current_temperature(true);
        traits.set_supported_modes({esphome::climate::CLIMATE_MODE_HEAT, esphome::climate::CLIMATE_MODE_AUTO});
        traits.set_visual_min_temperature(0);
        traits.set_visual_max_temperature(30);
        traits.set_visual_temperature_step(0.01);
        return traits;
    }
};

#endif
