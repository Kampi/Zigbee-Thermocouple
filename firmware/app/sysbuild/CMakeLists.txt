# Add the app directory as BOARD_ROOT so that boards/kampi/zigbee_thermocouple is found.
list(APPEND BOARD_ROOT ${APP_DIR})

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
