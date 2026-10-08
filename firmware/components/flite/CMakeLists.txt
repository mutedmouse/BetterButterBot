file(GLOB_RECURSE sources ./**/*.cpp ./**/*.c)
list(FILTER sources EXCLUDE REGEX "lex_data_include*")

idf_component_register(SRCS "${sources}" INCLUDE_DIRS "include")

target_compile_definitions(${COMPONENT_LIB} PRIVATE CST_USER_MALLOC)
