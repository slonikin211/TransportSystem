cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(TransportSystem)

set(SOURCES 
    src/main.cpp
)
set(HEADERS
    src/geo.h
    src/tests.h
)
set(PAIRS
    src/input_reader.cpp src/input_reader.h
    src/transport_catalogue.cpp src/transport_catalogue.h
    src/stat_reader.cpp src/stat_reader.h
)

add_executable(Task ${SOURCES} ${HEADERS} ${PAIRS})

set(CXX_COVERAGE_COMPILE_FLAGS "-std=c++17 -Wall -Werror -g")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_COMPILE_FLAGS}")

set_target_properties(
    Task PROPERTIES
    CXX_STANDART 17
    CXX_STANDART_REQUIRED ON
)