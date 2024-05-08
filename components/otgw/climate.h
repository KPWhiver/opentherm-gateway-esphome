#pragma once

#include "esphome/components/climate/climate.h"

namespace esphome {
namespace otgw {

class OpenthermGatewayClimate : public Component, public climate::Climate {
    float _max_temperature;
    std::function<void(optional<float>)> _callback;

public:
    OpenthermGatewayClimate(float max_temperature);

    void control(const climate::ClimateCall& call) override;

    void set_max_temperature(float max_temperature);

    void set_current_temperature(float current_temperature);

    void set_action(climate::ClimateAction action);

    void set_callback(decltype(_callback)&& callback);

    climate::ClimateTraits traits() override;
};

}
}
