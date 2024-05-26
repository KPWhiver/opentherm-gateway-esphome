import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import (
    CONF_ID,
)
from . import OpenthermGateway, CONF_OTGW_ID, otgw_ns

AUTO_LOAD = ["otgw"]

CONF_ROOM_THERMOSTAT = "room_thermostat"
CONF_HEATING_CIRCUIT_1 = "heating_circuit_1"
CONF_HEATING_CIRCUIT_2 = "heating_circuit_2"
CONF_HOT_WATER = "hot_water"

OpenthermGatewayClimate = otgw_ns.class_("OpenthermGatewayClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    cv.Optional(CONF_ROOM_THERMOSTAT): climate.CLIMATE_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayClimate),
    }),
    cv.Optional(CONF_HEATING_CIRCUIT_1): climate.CLIMATE_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayClimate),
    }),
    cv.Optional(CONF_HEATING_CIRCUIT_2): climate.CLIMATE_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayClimate),
    }),
    cv.Optional(CONF_HOT_WATER): climate.CLIMATE_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayClimate),
    }),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    if CONF_ROOM_THERMOSTAT in config:
        var = cg.new_Pvariable(config[CONF_ROOM_THERMOSTAT][CONF_ID], 30, False)
        await cg.register_component(var, config[CONF_ROOM_THERMOSTAT])
        await climate.register_climate(var, config[CONF_ROOM_THERMOSTAT])
        cg.add(hub.set_room_thermostat(var))

    if CONF_HEATING_CIRCUIT_1 in config:
        var = cg.new_Pvariable(config[CONF_HEATING_CIRCUIT_1][CONF_ID], 90, True)
        await cg.register_component(var, config[CONF_HEATING_CIRCUIT_1])
        await climate.register_climate(var, config[CONF_HEATING_CIRCUIT_1])
        cg.add(hub.set_heating_circuit_1(var))

    if CONF_HEATING_CIRCUIT_2 in config:
        var = cg.new_Pvariable(config[CONF_HEATING_CIRCUIT_2][CONF_ID], 90, True)
        await cg.register_component(var, config[CONF_HEATING_CIRCUIT_2])
        await climate.register_climate(var, config[CONF_HEATING_CIRCUIT_2])
        cg.add(hub.set_heating_circuit_2(var))

    if CONF_HOT_WATER in config:
        var = cg.new_Pvariable(config[CONF_HOT_WATER][CONF_ID], 90, True)
        await cg.register_component(var, config[CONF_HOT_WATER])
        await climate.register_climate(var, config[CONF_HOT_WATER])
        cg.add(hub.set_hot_water(var))
