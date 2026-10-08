# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)

set(IDF_TARGET esp32s3)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ButterBot-Firmware)

# The wake-word feature frontend only places its buffers in PSRAM when USE_ESP32 is defined
# (ESPHome sets it; a plain IDF build does not), and internal RAM is too tight to spare.
idf_component_get_property(msf_lib esphome__esp-micro-speech-features COMPONENT_LIB)
target_compile_definitions(${msf_lib} PRIVATE USE_ESP32)