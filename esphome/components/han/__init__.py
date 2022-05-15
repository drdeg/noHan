

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
from esphome.components import uart
from esphome.const import CONF_ID

AUTO_LOAD = ['sensor']
CODEOWNERS = ["@david"]

# Create the namespace reference
han_ns = cg.esphome_ns.namespace("han")

# Create the class reference (name, *parents)
Han = han_ns.class_("HANDecoder", cg.Component, uart.UARTDevice)

CONF_HAN_ID = "han_id"

# Configuration variable uart_id is inherited from UART_DEVICE_SCHEMA

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Han),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
