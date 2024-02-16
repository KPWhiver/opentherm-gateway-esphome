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
    esphome::optional<float> _average_temperature;
    float _average_temperature_change = 0;
    float _predicted_temperature = 100; // To stop the system from starting heating immediately
    esphome::optional<uint16_t> _oldest_temperature_index;
    unsigned long _time_of_last_temperature = 0;

    float _outside_temperature = 0;
    float const _max_central_heating_setpoint = 55;
    float const _min_central_heating_setpoint = 25;
    unsigned long _time_of_start_heating = 0;
    bool _previously_heating = false;

    float const _temperature_change_per_degree_per_lookahead = 2;
    unsigned long const _lookahead_time_ms = 5400.0 * 1000.0;

public:
    OpenthermGatewayThermostat(OpenthermGateway *opentherm_gateway, Sensor *external_return_temperature) :
        _opentherm_gateway(opentherm_gateway) {

        // Initial state
        this->target_temperature = 0; // to stop the system from starting heating immediately

        this->mode = ClimateMode::CLIMATE_MODE_HEAT;
        this->publish_state();

        _opentherm_gateway->s_room_setpoint_1->add_on_state_callback([=](float room_setpoint) {
            this->target_temperature = room_setpoint;
            this->publish_state();
        });
        _opentherm_gateway->s_room_temperature->add_on_state_callback([=](float room_temperature) {
            this->current_temperature = room_temperature;

            this->calculate_average_temperature_change();
            this->calculate_central_heating_setpoint();

            this->publish_state();
        });
        _opentherm_gateway->s_outside_temperature->add_on_state_callback([=](float outside_temperature) {
            _outside_temperature = outside_temperature;
        });
        _opentherm_gateway->s_central_heating_temperature->add_on_state_callback([=](float central_heating_temperature) {
            this->calculate_central_heating_setpoint();
        });
    }

    void setup() override {
    }

    void calculate_average_temperature_change() {
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
        _opentherm_gateway->s_average_temperature->publish_state(*_average_temperature);
        _opentherm_gateway->s_average_temperature_change->publish_state(_average_temperature_change);

        // Calculate the time since the last temperature measurement
        unsigned long current_time = millis();
        unsigned long time_since_last_temperature = 60000; // Decent default
        if (_time_of_last_temperature > 0 && current_time > _time_of_last_temperature) {
            time_since_last_temperature = current_time - _time_of_last_temperature;
        }
        _time_of_last_temperature = current_time;

        float time_factor = _lookahead_time_ms / float(time_since_last_temperature);
        _predicted_temperature = *_average_temperature + time_factor * _average_temperature_change;
        _opentherm_gateway->s_predicted_temperature->publish_state(_predicted_temperature);
    }

    void calculate_central_heating_setpoint() {
        bool request_heating = false;
        if (this->mode == ClimateMode::CLIMATE_MODE_OFF) {
            this->action = ClimateAction::CLIMATE_ACTION_OFF;
        } else if (_predicted_temperature < this->target_temperature) {
            this->action = ClimateAction::CLIMATE_ACTION_HEATING;
            request_heating = true;
        } else {
            this->action = ClimateAction::CLIMATE_ACTION_IDLE;
        }

        esphome::optional<float> central_heating_setpoint = _max_central_heating_setpoint;

        // Check if we should stop heating
        unsigned long current_time = millis();
        unsigned long time_since_start_heating = current_time - _time_of_start_heating;
        unsigned long const minute = 60000;

        if (_previously_heating && !request_heating) {
            bool flame = _opentherm_gateway->s_slave_flame->state;
            float central_heating_temperature = _opentherm_gateway->s_central_heating_temperature->raw_state;
            bool currently_heating = flame || central_heating_temperature > *central_heating_setpoint;

            if (time_since_start_heating < 5 * minute) { // Heat for at least 5 minutes
                request_heating = true;
            } else if (currently_heating && time_since_start_heating < 15 * minute) {
                request_heating = true;
            }
        } else if (!_previously_heating && request_heating) {
            _time_of_start_heating = current_time;
        }

        if (!request_heating) {
            central_heating_setpoint.reset();
        }
        _previously_heating = request_heating;

        _opentherm_gateway->set_primary_heating_override(central_heating_setpoint);
        this->publish_state();
    }

    void control(ClimateCall const &call) override {
        if (call.get_mode().has_value()) {
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
        traits.set_supported_modes({esphome::climate::CLIMATE_MODE_OFF, esphome::climate::CLIMATE_MODE_HEAT});
        traits.set_supports_action(true);
        traits.set_visual_min_temperature(0);
        traits.set_visual_max_temperature(30);
        traits.set_visual_temperature_step(0.01);
        return traits;
    }
};

#endif
