# Copyright (c) 2026 Zigbee-Thermocouple project
# SPDX-License-Identifier: Apache-2.0

FILE(GLOB event_sources *.c)
target_sources(app PRIVATE ${event_sources})
