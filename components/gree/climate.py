# import logging
import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.components import climate, select, switch, uart
from esphome.const import (
    CONF_ID,
    CONF_SUPPORTED_PRESETS,
    # CONF_SUPPORTED_SWING_MODES,
)
from esphome.components.climate import (
    ClimatePreset,
    # ClimateSwingMode,
)

CODEOWNERS = ["@bekmansurov"]
DEPENDENCIES = ["climate", "uart"]
AUTO_LOAD = ["select", "switch"]

gree_ns = cg.esphome_ns.namespace("gree")
GreeClimate = gree_ns.class_(
    "GreeClimate", climate.Climate, cg.PollingComponent, uart.UARTDevice
)
GreeFeatureSwitch = gree_ns.class_("GreeFeatureSwitch", switch.Switch, cg.Component)
GreeLouverSelect = gree_ns.class_("GreeLouverSelect", select.Select, cg.Component)
GreeFeature = gree_ns.enum("GreeFeature")

CONF_SLEEP = "sleep"
CONF_DISPLAY = "display"
CONF_TURBO = "turbo"
CONF_LOUVER = "louver"

LOUVER_OPTIONS = [
    "Off",
    "Full Swing",
    "Top",
    "Upper",
    "Middle",
    "Lower",
    "Bottom",
    "Lower Swing",
    "Middle Swing",
    "Upper Swing",
]

# ALLOWED_CLIMATE_SWING_MODES = {
#     "BOTH": ClimateSwingMode.CLIMATE_SWING_BOTH,
#     "VERTICAL": ClimateSwingMode.CLIMATE_SWING_VERTICAL,
#     "HORIZONTAL": ClimateSwingMode.CLIMATE_SWING_HORIZONTAL,
# }
# validate_swing_modes = cv.enum(ALLOWED_CLIMATE_SWING_MODES, upper=True)

ALLOWED_CLIMATE_PRESETS = {
    "NONE": ClimatePreset.CLIMATE_PRESET_NONE,
    "BOOST": ClimatePreset.CLIMATE_PRESET_BOOST,
    # "SLEEP": ClimatePreset.CLIMATE_PRESET_SLEEP,
}
validate_presets = cv.enum(ALLOWED_CLIMATE_PRESETS, upper=True)

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(GreeClimate),
            cv.Optional(CONF_SUPPORTED_PRESETS): cv.ensure_list(validate_presets),
            cv.Optional(CONF_SLEEP): switch.switch_schema(GreeFeatureSwitch),
            cv.Optional(CONF_DISPLAY): switch.switch_schema(GreeFeatureSwitch),
            cv.Optional(CONF_TURBO): switch.switch_schema(GreeFeatureSwitch),
            cv.Optional(CONF_LOUVER): select.select_schema(GreeLouverSelect),
            # cv.Optional(CONF_SUPPORTED_SWING_MODES): cv.ensure_list(
                # validate_swing_modes
            # ),
        }
    )
    # wifi module polls every 300ms but do we need it so often? set it to 10s
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)
    # if CONF_SUPPORTED_SWING_MODES in config:
    # cg.add(var.set_supported_swing_modes(config[CONF_SUPPORTED_SWING_MODES]))
    if CONF_SUPPORTED_PRESETS in config:
        cg.add(var.set_supported_presets(config[CONF_SUPPORTED_PRESETS]))

    feature_switches = (
        (CONF_SLEEP, GreeFeature.SLEEP, var.set_sleep_switch),
        (CONF_DISPLAY, GreeFeature.DISPLAY_LIGHT, var.set_display_switch),
        (CONF_TURBO, GreeFeature.TURBO, var.set_turbo_switch),
    )
    for key, feature, setter in feature_switches:
        if key in config:
            sw = cg.new_Pvariable(config[key][CONF_ID])
            await switch.register_switch(sw, config[key])
            await cg.register_component(sw, config[key])
            cg.add(sw.set_parent(var))
            cg.add(sw.set_feature(feature))
            cg.add(setter(sw))

    if CONF_LOUVER in config:
        sel = cg.new_Pvariable(config[CONF_LOUVER][CONF_ID])
        await select.register_select(sel, config[CONF_LOUVER], options=LOUVER_OPTIONS)
        await cg.register_component(sel, config[CONF_LOUVER])
        cg.add(sel.set_parent(var))
        cg.add(var.set_louver_select(sel))
