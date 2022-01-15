cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(TransportSystem)

set(SOURCES 
    src/main.cpp
)
set(HEADERS
    src/test_rendering.hpp
)
# TODO: Generate pairs with prefix src/ using operator for
set(PAIRS
    src/geo.cpp src/geo.h
    src/domain.cpp src/domain.h
    src/json.cpp src/json.h
    src/json_builder.cpp src/json_builder.h
    src/json_reader.cpp src/json_reader.h
    src/request_handler.cpp src/request_handler.h
    src/transport_catalogue.cpp src/transport_catalogue.h
    src/svg.cpp src/svg.h
    src/map_renderer.cpp src/map_renderer.h
)

add_executable(Task ${SOURCES} ${HEADERS} ${PAIRS})

set(CXX_COVERAGE_COMPILE_FLAGS "-std=c++17 -Wall -Werror -g")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_COMPILE_FLAGS}")

set_target_properties(
    Task PROPERTIES
    CXX_STANDART 17
    CXX_STANDART_REQUIRED ON
)