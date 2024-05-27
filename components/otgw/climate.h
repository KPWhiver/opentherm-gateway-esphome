#pragma once

#include "esphome/components/climate/climate.h"

namespace esphome {
namespace otgw {

class OpenthermGatewayClimate : public Component, public climate::Climate {
  std::function<void(optional<float>)> _callback;
  climate::ClimateTraits _traits;
  bool _off_supported;

 public:
  OpenthermGatewayClimate(float max_temperature, bool off_supported);

  void control(const climate::ClimateCall &call) override;

  void set_cooling_supported(bool supported);
  void set_target_temperature(float temperature);
  void set_max_temperature(float temperature);
  void set_current_temperature(float temperature);

  void set_mode(climate::ClimateMode mode);
  void set_action(climate::ClimateAction action);

  void set_callback(decltype(_callback) &&callback);

  climate::ClimateTraits traits() override;
};

}  // namespace otgw
}  // namespace esphome
