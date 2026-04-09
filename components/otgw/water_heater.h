#pragma once

#include "esphome/components/water_heater/water_heater.h"

namespace esphome {
namespace otgw {

class OpenthermGatewayWaterHeater : public Component, public water_heater::WaterHeater {
 protected:
  std::function<void()> _target_callback;
  std::function<void()> _mode_callback;
  water_heater::WaterHeaterTraits _traits;

 public:
  OpenthermGatewayWaterHeater(bool eco_mode);

  void control(const water_heater::WaterHeaterCall &call) override;

  void set_cooling_supported(bool supported);
  void set_target_temperature(float temperature);
  void set_max_temperature(float temperature);
  void set_min_temperature(float temperature);
  void set_current_temperature(float temperature);

  void set_mode(water_heater::WaterHeaterMode mode);

  void set_callbacks(decltype(_target_callback) &&target_callback, decltype(_mode_callback) &&mode_callback);

  water_heater::WaterHeaterTraits traits() override;
  water_heater::WaterHeaterCallInternal make_call() override;
};

}  // namespace otgw
}  // namespace esphome
