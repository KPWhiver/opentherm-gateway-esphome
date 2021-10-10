#ifndef OPENTHERM_GATEWAY_ONOFF_H
#define OPENTHERM_GATEWAY_ONOFF_H

#include "esphome.h"
#include "opentherm_gateway.h"

class OpenthermGatewayOnoff : public Component, public Switch {
    OpenthermGateway *_opentherm_gateway;

public:
    OpenthermGatewayOnoff(OpenthermGateway *opentherm_gateway) :
        _opentherm_gateway(opentherm_gateway) {}

    void write_state(bool heating) {
        _opentherm_gateway->set_secondary_heating(heating);

        publish_state(heating);
    }
};

#endif
