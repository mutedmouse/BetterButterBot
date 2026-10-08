# CMakeLists for ESP-IDF

file(GLOB_RECURSE SOURCES "src/**.cpp" "src/**.c")
set(COMPONENT_SRCS ${SOURCES})

set(COMPONENT_ADD_INCLUDEDIRS src)

set(COMPONENT_REQUIRES CMF)

register_component()
