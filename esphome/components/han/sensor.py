

# Required: uart_bus
# Configuration
# 
# uart:
#   id: uart_bus
#   tx_pin: GPIO01
#   rx_pin: GPIO03
#   baud_rate: 2400
# han:
#   uart_id: uart_bus
#   id: han_port
# sensor:
#   - platform: han
#     name: 'Current L1'
#     han_id: han_port
#     tag: 'tag-id'
#     unit_of_measurement: kWh
#     icon: mdi:flash  
#   - platform: han
#     name: 'Current L2'
#     han_id: han_port
#     tag: 'tag-id-2'
#     unit_of_measurement: kWh
#     icon: mdi:flash  


import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, ICON_FLASH, UNIT_KILOWATT_HOURS, STATE_CLASS_MEASUREMENT
import re

from . import han_ns, Han, CONF_HAN_ID

CONF_OBIS_CODE = "obis_code"

hanSensor = han_ns.class_("ObisSensor", sensor.Sensor, cg.Component)

def validate_obis_code(value):
    # This method should return a valid configuration value or raise an exception
    obisCode = re.split('-|,|\.|:', value)
    if len(obisCode) != 6:
        raise cv.Invalid(f"OBIS code must consist of 6 numerals separated by one of .,-:")

    # Convert to integers
    obisCodeNum = [ int(v) for v in obisCode ]

    for v in obisCodeNum:
        if v > 255 or v < 0:
            raise cv.Invalid(f"OBIS value {v} is outside valid range (0,255)")
    
    return obisCodeNum

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS, 
        icon=ICON_FLASH,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(hanSensor),
            cv.GenerateID(CONF_HAN_ID): cv.use_id(Han),
            cv.Required(CONF_OBIS_CODE): validate_obis_code,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    
    # Get the class instance of this sensor
    # This invokes the constructor (const char* )
    var = cg.new_Pvariable(config[CONF_ID], *config[CONF_OBIS_CODE])

    # Register the sensor as a component
    await cg.register_component(var, config)

    # Register the sensor as a sensor
    await sensor.register_sensor(var, config)

    # Get a pointer to the Han class
    han = await cg.get_variable(config[CONF_HAN_ID])

    # Call the han class to register a han listener
    cg.add(han.registerListener(var))