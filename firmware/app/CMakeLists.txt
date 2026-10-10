cmake_minimum_required(VERSION 3.20.0)

# Add the app directory as BOARD_ROOT so that boards/kampi/zigbee_thermocouple is found.
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
list(REMOVE_DUPLICATES BOARD_ROOT)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(thermocouple)

target_sources(app PRIVATE
    src/main.c
    src/zboss_mem_globals.c
)

# The ZBOSS memory configuration must not be compiled with LTO
if(CONFIG_LTO)
    set_source_files_properties(
        src/zboss_mem_globals.c
        PROPERTIES COMPILE_OPTIONS -fno-lto
    )
endif()
