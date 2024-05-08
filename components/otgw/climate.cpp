#include "climate.h"

namespace esphome {
namespace otgw {

OpenthermGatewayClimate::OpenthermGatewayClimate(float max_temperature) :
    _max_temperature(max_temperature) {}

void OpenthermGatewayClimate::control(const climate::ClimateCall& call) {
    bool publish = false;

    if (call.get_mode().has_value()) {
        this->mode = *call.get_mode();
        publish = true;
    }
    if (call.get_target_temperature().has_value()) {
        this->target_temperature = *call.get_target_temperature();
        publish = true;
    }

    if (publish) {
        optional<float> state;
        if (this->mode == climate::CLIMATE_MODE_HEAT) {
            state = this->target_temperature;
        }
        _callback(state);

        this->publish_state();
    }
}

void OpenthermGatewayClimate::set_max_temperature(float max_temperature) {
    _max_temperature = max_temperature;
}

void OpenthermGatewayClimate::set_current_temperature(float current_temperature) {
    this->current_temperature = current_temperature;
    this->publish_state();
}

void OpenthermGatewayClimate::set_action(climate::ClimateAction action) {
    this->action = action;
    this->publish_state();
}

void OpenthermGatewayClimate::set_callback(decltype(OpenthermGatewayClimate::_callback)&& callback) {
    _callback = callback;
}

climate::ClimateTraits OpenthermGatewayClimate::traits() {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({esphome::climate::CLIMATE_MODE_HEAT});
    traits.set_supports_action(true);
    traits.set_visual_min_temperature(0);
    traits.set_visual_max_temperature(_max_temperature);
    traits.set_visual_temperature_step(0.01);
    return traits;
}

}
}
