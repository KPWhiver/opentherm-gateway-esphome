import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import (
    CONF_ID,
)
from . import OpenthermGateway, CONF_OTGW_ID, otgw_ns

AUTO_LOAD = ["otgw"]

CONF_RESET_SERVICE_REQUEST = "reset_service_request"

OpenthermGatewayButton = otgw_ns.class_("OpenthermGatewayButton", button.Button)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    cv.Optional(CONF_RESET_SERVICE_REQUEST): button.button_schema(OpenthermGatewayButton),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    if CONF_RESET_SERVICE_REQUEST in config:
        var = await button.new_button(config[CONF_RESET_SERVICE_REQUEST])
        cg.add(hub.set_reset_service_request_button(var))
