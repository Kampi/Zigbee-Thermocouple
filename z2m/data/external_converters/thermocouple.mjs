import * as m from "zigbee-herdsman-converters/lib/modernExtend";

export default {
    zigbeeModel: ["Thermocouple"],
    model: "Thermocouple",
    vendor: "Kampi",
    description: "Zigbee-based thermocouple sensor (https://github.com/Kampi/Zigbee-Thermocouple)",
    extend: [m.identify({isSleepy: true}), m.battery({voltage: true, voltageReporting: true}), m.temperature({reporting: {min: "10_SECONDS", max: "1_HOUR", change: 10}})],
};
