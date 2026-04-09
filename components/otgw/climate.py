import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import (
    CONF_ID,
)
from . import OpenthermGateway, CONF_OTGW_ID, otgw_ns

AUTO_LOAD = ["otgw"]

CONF_ROOM_THERMOSTAT = "room_thermostat"

OpenthermGatewayClimate = otgw_ns.class_("OpenthermGatewayClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    cv.Optional(CONF_ROOM_THERMOSTAT): climate.climate_schema(OpenthermGatewayClimate)
    .extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayClimate),
    }),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    if CONF_ROOM_THERMOSTAT in config:
        var = cg.new_Pvariable(config[CONF_ROOM_THERMOSTAT][CONF_ID])
        await cg.register_component(var, config[CONF_ROOM_THERMOSTAT])
        await climate.register_climate(var, config[CONF_ROOM_THERMOSTAT])
        cg.add(hub.set_room_thermostat(var))
