import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ID,
    UNIT_CELSIUS,
    DEVICE_CLASS_TEMPERATURE,
)
from . import OpenthermGateway, otgw_ns, CONF_OTGW_ID

AUTO_LOAD = ["otgw"]

TemperatureNumber = otgw_ns.class_("TemperatureNumber", number.Number)
Type = otgw_ns.enum("Type");

CONF_TYPE = "type"
CONF_PRIORITY = "priority"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    cv.Required(CONF_TYPE): cv.enum({
        "heater_setpoint": Type.HEATER_SETPOINT,
    }),
    cv.Required(CONF_PRIORITY): cv.int_,
}).extend(number.number_schema(
    TemperatureNumber,
    unit_of_measurement=UNIT_CELSIUS,
    device_class=DEVICE_CLASS_TEMPERATURE,
))

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    num = await number.new_number(config, min_value=0, max_value=55, step=0.1)
    cg.add(num.set_priority(config[CONF_PRIORITY]))
    cg.add(hub.add_central_heating_setpoint(num))
