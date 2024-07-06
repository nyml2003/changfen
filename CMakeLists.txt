cmake_minimum_required(VERSION 3.0)
project(sed)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(SED_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SED_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(SED_TEST_DIR ${CMAKE_SOURCE_DIR}/tests)
set(SED_UNIT_TEST_DIR ${SED_TEST_DIR}/unittest)
set(SED_TEST_FUNCTIONAL_DIR ${SED_TEST_DIR}/functional)
set(SED_TEST_HIDDEN_FUNCTIONAL_DIR ${SED_TEST_DIR}/hidden_functional)
set(SED_ENTRY ${SED_SOURCE_DIR}/main.cpp)
set(SED_INDEX ${SED_INCLUDE_DIR}/index.h)
set(SED_UTIL_H ${SED_INCLUDE_DIR}/utils.h)
set(SED_UTIL_CPP ${SED_SOURCE_DIR}/utils.cpp)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 -DNDEBUG")
file(GLOB SED_FRONT_END_HEADER_FILES ${SED_INCLUDE_DIR}/frontend/*.h)
file(GLOB SED_IR_HEADER_FILES ${SED_INCLUDE_DIR}/ir/*.h)
file(GLOB SED_PASS_HEADER_FILES ${SED_INCLUDE_DIR}/backend/passes/*.h)
file(GLOB SED_BACKEND_HEADER_FILES ${SED_INCLUDE_DIR}/backend/*.h)
file(GLOB SED_FRONT_END_SOURCE_FILES ${SED_SOURCE_DIR}/frontend/*.cpp)
file(GLOB SED_IR_SOURCE_FILES ${SED_SOURCE_DIR}/ir/*.cpp)
file(GLOB SED_PASS_SOURCE_FILES ${SED_SOURCE_DIR}/backend/passes/*.cpp)
file(GLOB SED_BACKEND_SOURCE_FILES ${SED_SOURCE_DIR}/backend/*.cpp)

# Bison and Flex
if (EXISTS ${SED_INCLUDE_DIR}/frontend/syntax)
    file(REMOVE_RECURSE ${SED_INCLUDE_DIR}/frontend/syntax)
endif()
if (NOT EXISTS ${SED_INCLUDE_DIR}/frontend/syntax)
    file(MAKE_DIRECTORY ${SED_INCLUDE_DIR}/frontend/syntax)
endif()
if (EXISTS ${SED_SOURCE_DIR}/frontend/syntax)
    file(REMOVE_RECURSE ${SED_SOURCE_DIR}/frontend/syntax)
endif()
if (NOT EXISTS ${SED_SOURCE_DIR}/frontend/syntax)
    file(MAKE_DIRECTORY ${SED_SOURCE_DIR}/frontend/syntax)
endif()

find_package(BISON REQUIRED)
BISON_TARGET(
    parser
    ${SED_SOURCE_DIR}/frontend/parser.y
    ${SED_SOURCE_DIR}/frontend/syntax/parser.cpp
    COMPILE_FLAGS "-Wcounterexamples -Wother"
    DEFINES_FILE ${SED_INCLUDE_DIR}/frontend/syntax/parser.h
)
find_package(FLEX REQUIRED)
FLEX_TARGET(
    scanner
    ${SED_SOURCE_DIR}/frontend/scanner.l
    ${SED_SOURCE_DIR}/frontend/syntax/scanner.cpp
)

ADD_FLEX_BISON_DEPENDENCY(scanner parser)
include_directories(${SED_INCLUDE_DIR} ${SED_INCLUDE_DIR}/frontend ${SED_INCLUDE_DIR}/frontend/syntax)
add_executable(
    sed
    ${SED_INDEX}
    ${SED_UTIL_H}
    ${SED_FRONT_END_HEADER_FILES}
    ${SED_IR_HEADER_FILES}
    ${SED_PASS_HEADER_FILES}
    ${SED_BACKEND_HEADER_FILES}
    ${BISON_parser_OUTPUTS}
    ${FLEX_scanner_OUTPUTS}
    ${SED_UTIL_CPP}
    ${SED_FRONT_END_SOURCE_FILES}
    ${SED_IR_SOURCE_FILES}
    ${SED_PASS_SOURCE_FILES}
    ${SED_BACKEND_SOURCE_FILES}
    ${SED_ENTRY}
)
add_custom_command(
    TARGET sed
    POST_BUILD
    COMMAND python3 ${SED_TEST_DIR}/persist.py
    --executable $<TARGET_FILE:sed>
    --input ${SED_TEST_FUNCTIONAL_DIR} ${SED_TEST_HIDDEN_FUNCTIONAL_DIR}
    --pwd ${SED_TEST_DIR}
    --test ${SED_TEST_DIR}
)
# find_package(GTest REQUIRED)
# file(GLOB SED_FRONT_END_UNIT_TEST_FILES ${SED_UNIT_TEST_DIR}/frontend/*.cpp)
# add_executable(
#     sed_frontend_unit_test
#     ${SED_INDEX}
#     ${SED_UTIL_H}
#     ${SED_FRONT_END_HEADER_FILES}
#     ${SED_IR_HEADER_FILES}
#     ${SED_PASS_HEADER_FILES}
#     ${SED_BACKEND_HEADER_FILES}
#     ${BISON_parser_OUTPUTS}
#     ${FLEX_scanner_OUTPUTS}
#     ${SED_UTIL_CPP}
#     ${SED_FRONT_END_SOURCE_FILES}
#     ${SED_IR_SOURCE_FILES}
#     ${SED_PASS_SOURCE_FILES}
#     ${SED_BACKEND_SOURCE_FILES}
#     ${SED_ENTRY}
# )

# target_link_libraries(sed_frontend_unit_test GTest::GTest GTest::Main)
# enable_testing()
# add_test(
#     NAME sed_frontend_unit_test
#     COMMAND sed_frontend_unit_test
# )

