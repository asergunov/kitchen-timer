from esphome.components.time import RealTimeClock
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_ON_TIME, CONF_TRIGGER_ID, CONF_TIME_ID
from esphome import automation

timer_ns = cg.esphome_ns.namespace("timer")
TimerComponent = timer_ns.class_("TimerComponent", cg.Component)
TimerTrigger = timer_ns.class_("TimerTrigger", automation.Trigger.template(), cg.Component)

CONF_SIGNAL_TIME = "time"
CONF_SIGNAL_CLOCK = "clock"

def validate_timer_time(value):
    """
    TODO: Validate HH:MM line
    """
    return validator(value)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TimerComponent),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(RealTimeClock),
        cv.GenerateID(CONF_ON_TIME): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerTrigger),
                cv.Optional(CONF_SIGNAL_TIME): validate_timer_time,
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