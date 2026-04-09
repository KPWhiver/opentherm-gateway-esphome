import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import water_heater
from esphome.const import (
    CONF_ID,
)
from . import OpenthermGateway, CONF_OTGW_ID, otgw_ns

AUTO_LOAD = ["otgw"]

CONF_HEATING_CIRCUIT_1 = "heating_circuit_1"
CONF_HEATING_CIRCUIT_2 = "heating_circuit_2"
CONF_HOT_WATER = "hot_water"

OpenthermGatewayWaterHeater = otgw_ns.class_("OpenthermGatewayWaterHeater", water_heater.WaterHeater, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    cv.Optional(CONF_HEATING_CIRCUIT_1): water_heater.water_heater_schema(OpenthermGatewayWaterHeater)
    .extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayWaterHeater),
    }),
    cv.Optional(CONF_HEATING_CIRCUIT_2): water_heater.water_heater_schema(OpenthermGatewayWaterHeater)
    .extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayWaterHeater),
    }),
    cv.Optional(CONF_HOT_WATER): water_heater.water_heater_schema(OpenthermGatewayWaterHeater)
    .extend({
        cv.GenerateID(): cv.declare_id(OpenthermGatewayWaterHeater),
    }),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    cg.add_define("USE_WATER_HEATER_VISUAL_OVERRIDES")

    if CONF_HEATING_CIRCUIT_1 in config:
        var = cg.new_Pvariable(config[CONF_HEATING_CIRCUIT_1][CONF_ID], False)
        await cg.register_component(var, config[CONF_HEATING_CIRCUIT_1])
        await water_heater.register_water_heater(var, config[CONF_HEATING_CIRCUIT_1])
        cg.add(hub.set_heating_circuit_1(var))

    if CONF_HEATING_CIRCUIT_2 in config:
        var = cg.new_Pvariable(config[CONF_HEATING_CIRCUIT_2][CONF_ID], False)
        await cg.register_component(var, config[CONF_HEATING_CIRCUIT_2])
        await water_heater.register_water_heater(var, config[CONF_HEATING_CIRCUIT_2])
        cg.add(hub.set_heating_circuit_2(var))

    if CONF_HOT_WATER in config:
        var = cg.new_Pvariable(config[CONF_HOT_WATER][CONF_ID], True)
        await cg.register_component(var, config[CONF_HOT_WATER])
        await water_heater.register_water_heater(var, config[CONF_HOT_WATER])
        cg.add(hub.set_hot_water(var))
