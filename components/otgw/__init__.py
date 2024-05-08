import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
)

CONF_OTGW_ID = "otgw_id"

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor", "climate", "number"]

otgw_ns = cg.esphome_ns.namespace('otgw')
OpenthermGateway = otgw_ns.class_('OpenthermGateway', uart.UARTDevice, cg.Component)

CONF_OVERRIDE_THERMOSTAT = "override_thermostat"
CONF_OUTSIDE_TEMPERATURE = "outside_temperature"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OpenthermGateway),

    cv.Optional(CONF_OVERRIDE_THERMOSTAT): cv.boolean,
    cv.Optional(CONF_OUTSIDE_TEMPERATURE): cv.use_id(sensor.Sensor),
}).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    uart_component = await cg.get_variable(config[CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_component)

    if CONF_OUTSIDE_TEMPERATURE in config:
        sens = await cg.get_variable(config[CONF_OUTSIDE_TEMPERATURE])
        cg.add(var.outside_temperature_override.set(sens));

    if CONF_OVERRIDE_THERMOSTAT in config:
        cg.add(var.set_override_thermostat(config[CONF_OVERRIDE_THERMOSTAT]))

    await cg.register_component(var, config)
