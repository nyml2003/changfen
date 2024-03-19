cmake_minimum_required(VERSION 3.0)
project(changfen)

set(CMAKE_CXX_STANDARD 17)

find_package(FLEX REQUIRED)
FLEX_TARGET(scanner src/scanner.l ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-label -Wno-unused-local-typedefs -Wno-unused-private-field")

add_executable(changfen
        ${FLEX_scanner_OUTPUTS}
        src/LexTable.cpp
)

enable_testing()

file(GLOB TEST_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/*)

foreach (TEST_DIR ${TEST_DIRS})
    set(TEST_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/tests/${TEST_DIR}/in)
    set(TEST_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tests/${TEST_DIR}/out)
    if (NOT EXISTS ${TEST_OUTPUT})
        file(WRITE ${TEST_OUTPUT} "")
    endif()
    add_test(
        NAME ${TEST_DIR}
        COMMAND changfen ${TEST_INPUT} ${TEST_OUTPUT}
    )
    
    
endforeach()
