# Zigbee-Thermocouple

[![License](https://img.shields.io/badge/License-GPL%203.0-blue.svg)](https://opensource.org/license/gpl-3-0/)
[![PCB](https://github.com/Kampi/Zigbee-Thermocouple/actions/workflows/pcb.yaml/badge.svg)](https://github.com/Kampi/Zigbee-Thermocouple/actions/workflows/pcb.yaml)
[![Documentation](https://img.shields.io/badge/Documentation-HTML-007ec6?longCache=true&style=flat&logo=asciidoctor&colorA=555555)](https://Kampi.github.io/Zigbee-Thermocouple/)

## Table of Contents

- [Zigbee-Thermocouple](#zigbee-thermocouple)
  - [Table of Contents](#table-of-contents)
  - [About](#about)
  - [Zigbee production config](#zigbee-production-config)
  - [Zigbee2MQTT](#zigbee2mqtt)
  - [Directory Breakdown](#directory-breakdown)
  - [Ressources](#ressources)
  - [Maintainer](#maintainer)

## About

...

It is built with **KiCad** and integrates a full **CI/CD pipeline** using **KiBot**, ensuring reproducible outputs for documentation, manufacturing, and releases.

Key features:

- Open-source hardware design
- Automated generation of Gerber, BoM, and documentation via KiBot
- Workflow stages (DRAFT â†’ PRELIMINARY â†’ CHECKED â†’ RELEASED) for structured releases
- Clear documentation and changelog to track progress
- Designed with collaboration and transparency in mind

Please check the [wiki](https://github.com/Kampi/Zigbee-Thermocouple/wiki) for more information about the project.

## Zigbee production config

You must flash a production config to the device before you can use it with Zigbee. You can either use the prebuilt config `prebuilt/thermocouple_config.hex` or create your own from `firmware/app/zigbee_config.yml`.

```sh
nrfutil nrf5sdk-tools zigbee production_config firmware/app/zigbee_config.yml zigbee_config.hex --offset 0x17c000
nrfutil device program --firmware zigbee_config.hex --options chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE,verify=VERIFY_READ
```

Alternatively run `west upload-config`, which generates the config with a random device address and flashes it.

> **NOTE**
> The offset `0x17c000` is the address of the `zboss_product_config` partition (see `ncs-zigbee/dts/nrf54l15_cpuapp_partitions.dtsi`). Change `extended_address` in the Zigbee configuration file if you use more than one device!

## Zigbee2MQTT

The device only uses standard ZCL clusters (Basic, Identify, Power Configuration and Temperature Measurement), so no changes to Zigbee2MQTT itself are required.

1. Install [Zigbee2MQTT](https://www.zigbee2mqtt.io/)
2. Copy the external converter from `z2m/data/external_converters` into the `data/external_converters` directory of your Zigbee2MQTT installation
3. Use `z2m/data/configuration.yaml` as a template for your configuration. The Zigbee channel must be part of `CONFIG_ZIGBEE_CHANNEL_MASK` in `firmware/app/prj.conf` (channel 11 or 16)
4. Restart Zigbee2MQTT, enable joining and reset the device

## Directory Breakdown

- **`.github`**: GitHub related files
- **`.gitlab`**: GitLab related files
- **`.gitignore`**: Ignored files list
- **`3d-print`**: Project related files for 3D printer
- **`cad`**: Project related CAD files
- **`firmware`**: Firmware directory
- **`prebuilt`**: Prebuilt binaries (e.g. Zigbee production config)
- **`thermocouple`**: KiCad project for the PCB
- **`scripts`**: Additional scripts for CI/CD etc.
- **`CHANGELOG.md`**: Version history
- **`LICENSE`**: Project license
- **`README.md`**: Project overview
- **`z2m`**: Zigbee2MQTT configuration and external converter

## Ressources

- [KiBot Template](https://github.com/nguyen-v/KDT_Hierarchical_KiBot)
- [KiCad Project Template](https://github.com/Kampi/Template-Project)

## Maintainer

- [Daniel Kampert](mailto:DanielKampert@kampis-elektroecke.de)

