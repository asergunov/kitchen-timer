from esphome.components import time as time_
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.core import CORE
from esphome.const import CONF_ID, CONF_ON_TIME, CONF_TRIGGER_ID, CONF_MINUTES, CONF_HOURS, CONF_TIME_ID
from esphome import automation

daily_signal_ns = cg.esphome_ns.namespace("daily_signal")
SignalComponent = daily_signal_ns.class_("SignalComponent", cg.Component)
SignalTrigger = daily_signal_ns.class_("SignalTrigger", automation.Trigger.template(), time_.CronTrigger)

CONF_SIGNAL_TIME = "time"
CONF_SIGNAL_CLOCK = "clock"

def validate_signal_time(value):
    """
    TODO: Validate HH:MM line
    """
    return validator(value)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SignalComponent),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
        cv.GenerateID(CONF_ON_TIME): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SignalTrigger),
                cv.Optional(CONF_SIGNAL_TIME): validate_signal_time,
            },
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    clock = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(clock))

    await cg.register_component(var, config)
    
    for conf in config.get(CONF_ON_TIME, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], clock)

        await cg.register_component(trigger, conf)
        await automation.build_automation(trigger, [], conf)
        cg.add(var.set_trigger(trigger))