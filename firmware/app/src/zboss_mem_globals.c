/*
 * Holds the configurable RAM globals of ZBOSS in a dedicated translation unit.
 *
 * The prebuilt ZBOSS library contains weak defaults for these globals. When they
 * are defined in a file which is compiled with LTO, the linker reports
 * "size of symbol ... changed" warnings. Therefore this file is compiled with
 * -fno-lto (see CMakeLists.txt), like in the samples of the Zigbee add-on.
 */

#include <zboss_api.h>
#include <zb_mem_config_med.h>
