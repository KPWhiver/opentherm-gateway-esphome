#ifndef OPENTHERM_GATEWAY_THERMOSTAT_H
#define OPENTHERM_GATEWAY_THERMOSTAT_H

#include "esphome.h"
#include "opentherm_gateway.h"

class OpenthermGatewayThermostat : public Component, public Climate {
    OpenthermGateway *_opentherm_gateway;

public:
    OpenthermGatewayThermostat(OpenthermGateway *opentherm_gateway) :
        _opentherm_gateway(opentherm_gateway) {

        this->mode = ClimateMode::CLIMATE_MODE_HEAT;
        this->publish_state();

        _opentherm_gateway->s_room_setpoint_1->add_on_state_callback([=](float room_setpoint) {
            ESP_LOGD("otgw", "Room setpoint: %2.2f", room_setpoint);
            this->target_temperature = room_setpoint;
            this->publish_state();
        });
        _opentherm_gateway->s_room_temperature->add_on_state_callback([=](float room_temperature) {
            ESP_LOGD("otgw", "Room temperature: %2.2f", room_temperature);
            this->current_temperature = room_temperature;
            this->publish_state();
        });

    }

    void setup() override {
    }

    void control(ClimateCall const &call) override {
        if (call.get_mode().has_value()) {
            // Turning off is not supported
            this->mode = ClimateMode::CLIMATE_MODE_HEAT;
            this->publish_state();
        }
        if (call.get_target_temperature().has_value()) {
            float target_temperature = *call.get_target_temperature();
            bool success = _opentherm_gateway->set_room_setpoint(target_temperature);
            if (!success)
                return;

            this->target_temperature = target_temperature;
            this->publish_state();
        }
    }

    ClimateTraits traits() override {
        auto traits = climate::ClimateTraits();
        traits.set_supports_current_temperature(true);
        traits.set_supported_modes({esphome::climate::CLIMATE_MODE_HEAT});
        traits.set_visual_min_temperature(0);
        traits.set_visual_max_temperature(30);
        traits.set_visual_temperature_step(0.01);
        return traits;
    }
};

#endif
