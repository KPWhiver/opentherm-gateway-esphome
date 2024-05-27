#include "climate.h"

namespace esphome {
namespace otgw {

OpenthermGatewayClimate::OpenthermGatewayClimate(float max_temperature, bool off_supported)
    : _off_supported(off_supported) {
  if (!off_supported) {
    this->mode = climate::CLIMATE_MODE_HEAT;
  }

  _traits.set_supports_current_temperature(true);
  _traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
  if (off_supported) {
    _traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  }
  _traits.set_supports_action(true);
  _traits.set_visual_min_temperature(1);
  _traits.set_visual_max_temperature(max_temperature);
  _traits.set_visual_temperature_step(0.01);
}

void OpenthermGatewayClimate::control(const climate::ClimateCall &call) {
  bool publish = false;

  if (call.get_mode().has_value()) {
    auto new_mode = *call.get_mode();
    if (new_mode != climate::CLIMATE_MODE_OFF || _off_supported) {
      this->mode = new_mode;
    }

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

void OpenthermGatewayClimate::set_cooling_supported(bool supported) {
  _traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
  if (_off_supported) {
    _traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  }
  if (supported) {
    _traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  }
}

void OpenthermGatewayClimate::set_target_temperature(float temperature) {
  this->target_temperature = temperature;
  this->publish_state();
}

void OpenthermGatewayClimate::set_max_temperature(float temperature) {
  _traits.set_visual_max_temperature(temperature);
}

void OpenthermGatewayClimate::set_current_temperature(float temperature) {
  this->current_temperature = temperature;
  this->publish_state();
}

void OpenthermGatewayClimate::set_action(climate::ClimateAction action) {
  this->action = action;
  this->publish_state();
}

void OpenthermGatewayClimate::set_mode(climate::ClimateMode mode) {
  this->mode = mode;
  this->publish_state();
}

void OpenthermGatewayClimate::set_callback(decltype(OpenthermGatewayClimate::_callback) &&callback) {
  _callback = callback;
}

climate::ClimateTraits OpenthermGatewayClimate::traits() { return _traits; }

}  // namespace otgw
}  // namespace esphome
