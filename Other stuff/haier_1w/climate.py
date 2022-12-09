import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import (
    climate,
    remote_transmitter,
    remote_receiver,
    # sensor,
    remote_base,
)
from esphome.components.remote_base import CONF_RECEIVER_ID, CONF_TRANSMITTER_ID
from esphome.const import CONF_SUPPORTS_COOL, CONF_SUPPORTS_HEAT #, CONF_SENSOR
from esphome.const import (
    CONF_ID,
    CONF_SUPPORTS_COOL, 
    CONF_SUPPORTS_HEAT, 
    # CONF_SENSOR,
    #CONF_UPDATE_INTERVAL,
)

# AUTO_LOAD = ["sensor", "remote_base"]
AUTO_LOAD = ["remote_base"]
CODEOWNERS = ["@CubeBall"]

haier_1w_ns = cg.esphome_ns.namespace("haier_1w")
Haier1w = haier_1w_ns.class_(
    "Haier1w", climate.Climate, cg.PollingComponent, remote_base.RemoteReceiverListener
)

CONF_SEND_HEADER_HIGH = "send_header_high"
CONF_RECEIVE_HEADER_HIGH = "recceive_header_high"
CONF_SEND_HEADER_LOW = "send_header_low"
CONF_RECEIVE_HEADER_LOW = "receive_header_low"
CONF_BIT_SPACE = "bit_space"
# CONF_BIT_HIGH = "bit_high"
CONF_BIT_ONE = "bit_one"
# CONF_BIT_ONE_LOW = "bit_one_low"
CONF_BIT_ZERO = "bit_zero"
# CONF_BIT_ZERO_LOW = "bit_zero_low"
#
# CONF_UPDATE_INTERVAL = "update_interval"

CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Haier1w),
        cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
        cv.Optional(CONF_SUPPORTS_COOL, default=True): cv.boolean,
        cv.Optional(CONF_SUPPORTS_HEAT, default=True): cv.boolean,
        # cv.Optional(CONF_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_RECEIVER_ID): cv.use_id(
            remote_receiver.RemoteReceiverComponent
        ),
        #Protocol options
        cv.Optional(
            CONF_SEND_HEADER_HIGH, default="0us" #"202000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_RECEIVE_HEADER_HIGH, default="25000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_SEND_HEADER_LOW, default="48000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_RECEIVE_HEADER_LOW, default="24000us" #"24000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_SPACE, default="4000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_ONE, default="12000us" #Check for inverting
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_ZERO, default="4000us" #Check for inverting
        ): cv.positive_time_period_microseconds,
        # cv.Optional(
        #     CONF_UPDATE_INTERVAL, default="5000ms"
        # ): cv.positive_time_period_milliseconds,
    }
    #).extend(cv.polling_component_schema(CONF_UPDATE_INTERVAL))
    ).extend(cv.COMPONENT_SCHEMA)
)

# async def register_haier_1w(var, config):
async def to_code(config):
# async def register_haier_1w(config):
    # cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_supports_cool(config[CONF_SUPPORTS_COOL]))
    cg.add(var.set_supports_heat(config[CONF_SUPPORTS_HEAT]))
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
 
    # if CONF_SENSOR in config:
    #     sens = await cg.get_variable(config[CONF_SENSOR])
    #     cg.add(var.set_sensor(sens))
    if CONF_RECEIVER_ID in config:
        receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
        cg.add(receiver.register_listener(var))

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
    cg.add(var.set_send_header_high(config[CONF_SEND_HEADER_HIGH]))
    cg.add(var.set_receive_header_high(config[CONF_RECEIVE_HEADER_HIGH]))
    cg.add(var.set_send_header_low(config[CONF_SEND_HEADER_LOW]))
    cg.add(var.set_receive_header_low(config[CONF_RECEIVE_HEADER_LOW]))
    cg.add(var.set_bit_space(config[CONF_BIT_SPACE]))
    cg.add(var.set_bit_one(config[CONF_BIT_ONE]))
    cg.add(var.set_bit_zero(config[CONF_BIT_ZERO]))
    

# #Haier1w = haier_1w_ns.class_("Haier1w", climate_ir.ClimateIR)
# #Haier1w = cg.global_ns.class_("Haier1w", climate_ir.ClimateIR, cg.PollingComponent)
# Haier1w = haier_1w_ns.class_("Haier1w", climate_ir.ClimateIR, cg.PollingComponent)

# # CONF_HEADER_HIGH = "header_high"
# # CONF_HEADER_LOW = "header_low"
# # CONF_BIT_HIGH = "bit_high"
# # CONF_BIT_ONE_LOW = "bit_one_low"
# # CONF_BIT_ZERO_LOW = "bit_zero_low"
# #CONF_UPDATE_INTERVAL = "update_interval"

# CONFIG_SCHEMA = (
#     climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
#     {
#         cv.GenerateID(): cv.declare_id(Haier1w),
#     #     cv.Optional(
#     #         CONF_HEADER_HIGH, default="8000us"
#     #     ): cv.positive_time_period_microseconds,
#     #     cv.Optional(
#     #         CONF_HEADER_LOW, default="4000us"
#     #     ): cv.positive_time_period_microseconds,
#     #     cv.Optional(
#     #         CONF_BIT_HIGH, default="600us"
#     #     ): cv.positive_time_period_microseconds,
#     #     cv.Optional(
#     #         CONF_BIT_ONE_LOW, default="1600us"
#     #     ): cv.positive_time_period_microseconds,
#     #     cv.Optional(
#     #         CONF_BIT_ZERO_LOW, default="550us"
#     #     ): cv.positive_time_period_microseconds,
#     }
#     )
#     .extend(cv.polling_component_schema("5000ms"))
# )


# async def to_code(config):
# #def to_code(config):
#     var = cg.new_Pvariable(config[CONF_ID])
#     #await cg.register_component(var, config)
#     #await cg.register_component(var, config)
#     # await climate_ir.register_climate_ir(var, config)  
    
#     # cg.add(var.set_header_high(config[CONF_HEADER_HIGH]))
#     # cg.add(var.set_header_low(config[CONF_HEADER_LOW]))
#     # cg.add(var.set_bit_high(config[CONF_BIT_HIGH]))
#     # cg.add(var.set_bit_one_low(config[CONF_BIT_ONE_LOW]))
#     # cg.add(var.set_bit_zero_low(config[CONF_BIT_ZERO_LOW]))
#     # cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))

#     #await cg.register_component(var, config)
#     yield cg.register_component(var, config)
#     yield climate_ir.register_climate_ir(var, config)  
      