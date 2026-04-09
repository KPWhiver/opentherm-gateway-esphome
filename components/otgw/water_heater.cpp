#include "water_heater.h"

namespace esphome {
namespace otgw {

OpenthermGatewayWaterHeater::OpenthermGatewayWaterHeater(bool eco_mode) {
  if (eco_mode) {
    _traits.set_supported_modes(water_heater::WaterHeaterModeMask{
      water_heater::WATER_HEATER_MODE_OFF,
      water_heater::WATER_HEATER_MODE_ECO,
      water_heater::WATER_HEATER_MODE_PERFORMANCE,
    });
  } else {
    _traits.set_supported_modes(water_heater::WaterHeaterModeMask{
      water_heater::WATER_HEATER_MODE_OFF,
      water_heater::WATER_HEATER_MODE_PERFORMANCE,
    });
  }

  _traits.add_feature_flags(
    water_heater::WATER_HEATER_SUPPORTS_OPERATION_MODE |
    water_heater::WATER_HEATER_SUPPORTS_TARGET_TEMPERATURE |
    water_heater::WATER_HEATER_SUPPORTS_CURRENT_TEMPERATURE
  );
  _traits.set_target_temperature_step(0.01);

  this->set_visual_max_temperature_override(90);
  this->set_visual_min_temperature_override(1);
}

void OpenthermGatewayWaterHeater::control(const water_heater::WaterHeaterCall &call) {
  if (call.get_mode().has_value()) {
    set_mode_(*call.get_mode());
    _mode_callback();
  }

  set_target_temperature_(call.get_target_temperature());
  _target_callback();

  this->publish_state();
}

void OpenthermGatewayWaterHeater::set_cooling_supported(bool supported) {
  // TODO: Add support for cooling. there was some support before, but we should really make sure it is supported properly
}

void OpenthermGatewayWaterHeater::set_target_temperature(float temperature) {
  this->target_temperature_ = temperature;
  this->publish_state();
}

void OpenthermGatewayWaterHeater::set_max_temperature(float temperature) {
  this->set_visual_max_temperature_override(temperature);
  this->publish_state();
}

void OpenthermGatewayWaterHeater::set_min_temperature(float temperature) {
  this->set_visual_min_temperature_override(temperature);
  this->publish_state();
}

void OpenthermGatewayWaterHeater::set_current_temperature(float temperature) {
  this->current_temperature_ = temperature;
  this->publish_state();
}

void OpenthermGatewayWaterHeater::set_mode(water_heater::WaterHeaterMode mode) {
  this->mode_ = mode;
  this->publish_state();
}

void OpenthermGatewayWaterHeater::set_callbacks(
  decltype(OpenthermGatewayWaterHeater::_target_callback) &&target_callback,
  decltype(OpenthermGatewayWaterHeater::_mode_callback) &&mode_callback
) {
  _target_callback = target_callback;
  _mode_callback = mode_callback;
}

water_heater::WaterHeaterCallInternal OpenthermGatewayWaterHeater::make_call() {
  return water_heater::WaterHeaterCallInternal(this);
}

water_heater::WaterHeaterTraits OpenthermGatewayWaterHeater::traits() { return _traits; }

}  // namespace otgw
}  // namespace esphome
