import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
)
from . import OpenthermGateway, CONF_OTGW_ID

AUTO_LOAD = ["otgw"]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    # Master state
    cv.Optional("master_central_heating_1"): binary_sensor.binary_sensor_schema(),
    cv.Optional("master_central_heating_2"): binary_sensor.binary_sensor_schema(),
    cv.Optional("master_water_heating"): binary_sensor.binary_sensor_schema(),
    cv.Optional("master_cooling"): binary_sensor.binary_sensor_schema(),
    cv.Optional("master_outside_temperature_compensation"): binary_sensor.binary_sensor_schema(),

    # Slave state
    cv.Optional("slave_fault"): binary_sensor.binary_sensor_schema(),
    cv.Optional("slave_central_heating_1"): binary_sensor.binary_sensor_schema(),
    cv.Optional("slave_central_heating_2"): binary_sensor.binary_sensor_schema(),
    cv.Optional("slave_water_heating"): binary_sensor.binary_sensor_schema(),
    cv.Optional("slave_flame"): binary_sensor.binary_sensor_schema(),
    cv.Optional("slave_cooling"): binary_sensor.binary_sensor_schema(),
    cv.Optional("slave_diagnostic_event"): binary_sensor.binary_sensor_schema(),

    # Faults
    cv.Optional("service_required"): binary_sensor.binary_sensor_schema(),
    cv.Optional("lockout_reset"): binary_sensor.binary_sensor_schema(),
    cv.Optional("low_water_pressure"): binary_sensor.binary_sensor_schema(),
    cv.Optional("gas_flame_fault"): binary_sensor.binary_sensor_schema(),
    cv.Optional("air_pressure_fault"): binary_sensor.binary_sensor_schema(),
    cv.Optional("water_overtemperature"): binary_sensor.binary_sensor_schema(),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    sensors = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf[CONF_ID]
        if id and id.type == binary_sensor.BinarySensor:
            sens = await binary_sensor.new_binary_sensor(conf)
            cg.add(getattr(hub, f"{key}.set")(sens))
