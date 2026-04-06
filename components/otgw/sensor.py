import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_KILOWATT_HOURS,
    UNIT_HOUR,
    UNIT_HERTZ,
    UNIT_AMPERE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_PRESSURE,
    DEVICE_CLASS_SPEED,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_HUMIDITY,
    STATE_CLASS_TOTAL_INCREASING,
    STATE_CLASS_MEASUREMENT,
)
from . import OpenthermGateway, CONF_OTGW_ID

AUTO_LOAD = ["otgw"]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OTGW_ID): cv.use_id(OpenthermGateway),

    # Setpoints
    cv.Optional("max_central_heating_setpoint"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("central_heating_setpoint_1"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("central_heating_setpoint_2"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("hot_water_setpoint"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("remote_override_room_setpoint_1"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("remote_override_room_setpoint_2"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("room_setpoint_1"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("room_setpoint_2"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("cooling_control"): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=2,
    ),

    # Temperatures
    cv.Optional("central_heating_temperature_1"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("central_heating_temperature_2"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("hot_water_temperature_1"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("hot_water_temperature_2"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("room_temperature_1"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("room_temperature_2"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("outside_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("return_water_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("solar_storage_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("solar_collector_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("exhaust_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("boiler_heat_exchanger_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    # Modulation
    cv.Optional("max_relative_modulation_level"): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=2,
    ),
    cv.Optional("max_boiler_capacity"): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
    ),
    cv.Optional("min_modulation_level"): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=2,
    ),
    cv.Optional("relative_modulation_level"): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),


    # Water
    cv.Optional("central_heating_water_pressure"): sensor.sensor_schema(
        unit_of_measurement="bar",
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("hot_water_flow_rate"): sensor.sensor_schema(
        unit_of_measurement="l/min",
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    # Starts
    cv.Optional("slave_power_cycles"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("failed_burner_starts"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("flame_signal_low_count"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("central_heating_burner_starts"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("central_heating_pump_starts"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("hot_water_pump_starts"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("hot_water_burner_starts"): sensor.sensor_schema(
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),

    # Operation hours
    cv.Optional("cooling_operation_time"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HOUR,
        device_class=DEVICE_CLASS_DURATION,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("central_heating_burner_operation_time"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HOUR,
        device_class=DEVICE_CLASS_DURATION,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("central_heating_pump_operation_time"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HOUR,
        device_class=DEVICE_CLASS_DURATION,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("hot_water_pump_operation_time"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HOUR,
        device_class=DEVICE_CLASS_DURATION,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("hot_water_burner_operation_time"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HOUR,
        device_class=DEVICE_CLASS_DURATION,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),

    # Other
    cv.Optional("number_of_slave_parameters"): sensor.sensor_schema(),
    cv.Optional("fault_history_buffer_size"): sensor.sensor_schema(),
    cv.Optional("boiler_fan_speed_setpoint"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        device_class=DEVICE_CLASS_SPEED,
    ),
    cv.Optional("boiler_fan_speed"): sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        device_class=DEVICE_CLASS_SPEED,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("flame_current"): sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        device_class=DEVICE_CLASS_CURRENT,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("relative_humidity"): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        device_class=DEVICE_CLASS_HUMIDITY,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_OTGW_ID])

    sensors = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf[CONF_ID]
        if id and id.type == sensor.Sensor:
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(hub, f"set_sensor({hub}->{key}, {sens})"))
