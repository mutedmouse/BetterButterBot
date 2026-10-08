set(SRC_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Containers"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Core"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Entity"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Event"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Log"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Memory"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Misc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Object"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Periphery"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Statics"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Thread"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Util"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FileSystem"
        CACHE INTERNAL "" FORCE)

set(EXCLUDED_ITEMS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Display.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Display.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Camera.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Camera.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Periphery/WiFi.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Periphery/WiFi.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AACAudioGenerator.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AACAudioGenerator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/FileAudioSource.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/FileAudioSource.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/Audio.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/Audio.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AudioSource.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AudioGenerator.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDefs.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDefs.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevice.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleService.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleType.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_CO2Sensor.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_CO2Sensor.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_IRModule.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_IRModule.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_LEDModule.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_LEDModule.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_MotionSensor.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_MotionSensor.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_Perfboard.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_Perfboard.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_TempHumModule.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_TempHumModule.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Input/InputTouchGPIO.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Input/InputTouchGPIO.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Output/OutputMCPWM.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Output/OutputMCPWM.h"
        CACHE INTERNAL "" FORCE)

foreach(SRC IN LISTS SRC_DIRS)
    file(GLOB_RECURSE DIR_SOURCES "${SRC}/*/**.h" "${SRC}/*/**.cpp" "${SRC}/*/**.hpp" "${SRC}/*/**.inl" "${SRC}/**.h" "${SRC}/**.cpp" "${SRC}/**.hpp" "${SRC}/**.inl")
    list(APPEND SOURCES ${DIR_SOURCES})
endforeach()

foreach(SRC IN LISTS EXCLUDED_ITEMS)
    list(REMOVE_ITEM SOURCES "${SRC}")
endforeach()

set(LIBS_INCL "${CMAKE_CURRENT_SOURCE_DIR}/lib/glm/glm" "${CMAKE_CURRENT_SOURCE_DIR}/lib/libhelix-aac/src" CACHE INTERNAL "" FORCE)

# Modules
if(CONFIG_CMF_MODULES)
    list(APPEND SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDefs.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDefs.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevice.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevice.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleService.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleType.h")

    if(CONFIG_RM_TempHum)
        list(APPEND SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_TempHumModule.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_TempHumModule.h")
    endif()
    if(CONFIG_RM_LED)
        list(APPEND SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_LEDModule.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_LEDModule.h")
    endif()
    if(CONFIG_RM_Motion)
        list(APPEND SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_MotionSensor.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_MotionSensor.h")
    endif()
    if(CONFIG_RM_CO2)
        list(APPEND SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_CO2Sensor.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_CO2Sensor.h")
    endif()
    if(CONFIG_RM_IR)
        list(APPEND SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_IRModule.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_IRModule.h")
    endif()
    if(CONFIG_RM_PerfBoard)
        list(APPEND SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_Perfboard.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Modules/ModuleDevices/RM_Perfboard.h")
    endif()
endif()

# LVGL specific
if(CONFIG_CMF_LVGL)
    file(GLOB_RECURSE GRAPHICS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/LV_Interface/**.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/LV_Interface/**.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/LV_Interface/**.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/LV_Interface/**.h")
    list(APPEND SOURCES ${GRAPHICS_SOURCES})
endif()

# LGFX specific
if(CONFIG_CMF_LGFX)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Display.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Display.h")
endif()

# Audio specific
if(CONFIG_CMF_AUDIO)
    list(APPEND SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AACAudioGenerator.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AACAudioGenerator.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/Audio.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/Audio.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AudioSource.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/AudioGenerator.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/FileAudioSource.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/Audio/FileAudioSource.cpp")
    file(GLOB_RECURSE LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/libhelix-aac/src/**.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/lib/libhelix-aac/src/**.c")
else()
    set(LIBS "" CACHE INTERNAL "" FORCE)
endif()

# Camera specific
if(CONFIG_CMF_CAMERA)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Camera.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/Devices/Camera.cpp")
endif()

# WiFi specific
if(CONFIG_CMF_WIFI)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Periphery/WiFi.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Periphery/WiFi.h")
endif()

# Touch specific
if(CONFIG_CMF_TOUCH)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Input/InputTouchGPIO.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Input/InputTouchGPIO.h")
endif()

# MCPWM specific
if(CONFIG_CMF_MCPWM)
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Output/OutputMCPWM.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/Output/OutputMCPWM.h")
endif()

set(REQUIRED_COMPONENTS "esp_timer" "driver" "spiffs" "esp_adc" "esp_wifi" CACHE INTERNAL "" FORCE)

idf_component_register(SRCS ${SOURCES}  ${LIBS} INCLUDE_DIRS "src"  ${LIBS_INCL} REQUIRES ${REQUIRED_COMPONENTS})