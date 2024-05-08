import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
)
from . import OpenthermGateway, CONF_OTGW_ID

AUTO_LOAD = ["otgw"]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    # Master state
    cv.Optional("heater_state"): text_sensor.text_sensor_schema(),
    cv.Optional("last_command"): text_sensor.text_sensor_schema(),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    text_sensors = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf[CONF_ID]
        if id and id.type == text_sensor.TextSensor:
            sens = await text_sensor.new_text_sensor(conf)
            cg.add(getattr(hub, f"{key}.set")(sens))
