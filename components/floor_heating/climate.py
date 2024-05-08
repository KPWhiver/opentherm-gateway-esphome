import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import climate, sensor, number, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_HEAT_ACTION,
    CONF_IDLE_ACTION,
)

CONF_TEMPERATURE = "temperature"
CONF_OUTSIDE_TEMPERATURE = "outside_temperature"
CONF_TARGET_TEMPERATURE_OVERRIDE = "target_temperature_override"
CONF_HEATER_ACTIVE = "heater_active"
CONF_HEATER_RETURN_TEMPERATURE = "heater_return_temperature"
CONF_DEFAULT_TARGET_TEMPERATURE = "default_target_temperature"
CONF_MAX_HEATER_TEMPERATURE_SETPOINT = "max_heater_temperature_setpoint"
CONF_MIN_HEATER_TEMPERATURE_SETPOINT = "min_heater_temperature_setpoint"
CONF_HEATER_TEMPERATURE = "heater_temperature"
CONF_HEATER_TEMPERATURE_SETPOINT = "heater_temperature_output"

floor_heating_ns = cg.esphome_ns.namespace("floor_heating")
FloorHeatingClimate = floor_heating_ns.class_("FloorHeatingClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(FloorHeatingClimate),
    cv.Required(CONF_TEMPERATURE): cv.use_id(sensor.Sensor),
    cv.Required(CONF_HEATER_TEMPERATURE): cv.use_id(sensor.Sensor),
    cv.Required(CONF_HEATER_ACTIVE): cv.use_id(binary_sensor.BinarySensor),
    cv.Required(CONF_DEFAULT_TARGET_TEMPERATURE): cv.temperature,
    cv.Required(CONF_MAX_HEATER_TEMPERATURE_SETPOINT): cv.temperature,
    cv.Required(CONF_MIN_HEATER_TEMPERATURE_SETPOINT): cv.temperature,
    cv.Required(CONF_IDLE_ACTION): automation.validate_automation(single=True),
    cv.Required(CONF_HEAT_ACTION): automation.validate_automation(single=True),
    cv.Optional(CONF_OUTSIDE_TEMPERATURE): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_HEATER_RETURN_TEMPERATURE): cv.use_id(sensor.Sensor),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    sens = await cg.get_variable(config[CONF_TEMPERATURE])
    cg.add(var.set_temperature(sens));

    sens = await cg.get_variable(config[CONF_HEATER_TEMPERATURE])
    cg.add(var.set_heater_temperature(sens));

    cg.add(var.set_default_target_temperature(config[CONF_DEFAULT_TARGET_TEMPERATURE]))
    cg.add(var.set_max_heater_temperature_setpoint(config[CONF_MAX_HEATER_TEMPERATURE_SETPOINT]))
    cg.add(var.set_min_heater_temperature_setpoint(config[CONF_MIN_HEATER_TEMPERATURE_SETPOINT]))

    await automation.build_automation(
        var.get_idle_trigger(), [], config[CONF_IDLE_ACTION]
    )
    await automation.build_automation(
        var.get_heat_trigger(), [], config[CONF_HEAT_ACTION]
    )

    if CONF_OUTSIDE_TEMPERATURE in config:
        sens = await cg.get_variable(config[CONF_OUTSIDE_TEMPERATURE])
        cg.add(var.set_outside_temperature(sens));

    if CONF_HEATER_RETURN_TEMPERATURE in config:
        sens = await cg.get_variable(config[CONF_HEATER_RETURN_TEMPERATURE])
        cg.add(var.set_heater_return_temperature(sens));
