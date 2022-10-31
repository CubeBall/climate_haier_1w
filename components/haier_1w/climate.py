import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

AUTO_LOAD = ["climate_ir"]

climate_haier_1w_ns = cg.esphome_ns.namespace("climate_haier_1w")
Haier1wClimate = climate_haier_1w_ns.class_("Haier1wClimate", climate_ir.ClimateIR)

CONF_SEND_HEADER_HIGH = "header_send_high"
CONF_RECEIVE_HEADER_HIGH = "header_receive_high"
CONF_SEND_HEADER_LOW = "header_send_low"
CONF_RECEIVE_HEADER_LOW = "header_receive_low"
CONF_BIT_HIGH = "bit_high"
CONF_BIT_ONE_LOW = "bit_one_low"
CONF_BIT_ZERO_LOW = "bit_zero_low"

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Haier1wClimate),
        cv.Optional(
            CONF_SEND_HEADER_HIGH, default="200000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_RECEIVE_HEADER_HIGH, default="32000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_SEND_HEADER_LOW, default="48000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_RECEIVE_HEADER_LOW, default="24000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_HIGH, default="4000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_ONE_LOW, default="12000us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_ZERO_LOW, default="4000us"
        ): cv.positive_time_period_microseconds,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)

    cg.add(var.set_send_header_high(config[CONF_SEND_HEADER_HIGH]))
    cg.add(var.set_receive_header_high(config[CONF_RECEIVE_HEADER_HIGH]))
    cg.add(var.set_send_header_low(config[CONF_SEND_HEADER_LOW]))
    cg.add(var.set_receive_header_low(config[CONF_RECEIVE_HEADER_LOW]))
    cg.add(var.set_bit_high(config[CONF_BIT_HIGH]))
    cg.add(var.set_bit_one_low(config[CONF_BIT_ONE_LOW]))
    cg.add(var.set_bit_zero_low(config[CONF_BIT_ZERO_LOW]))
