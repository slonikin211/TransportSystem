cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(TransportSystem)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/transport_router.proto src/svg.proto src/map_renderer.proto src/transport_catalogue.proto)

set(SOURCES 
    src/main.cpp
)
set(HEADERS
    src/graph.h
    src/ranges.h
    src/router.h
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
    src/transport_router.cpp src/transport_router.h
    src/serialization.cpp src/serialization.h
)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${SOURCES} ${HEADERS} ${PAIRS})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

set(CXX_COVERAGE_COMPILE_FLAGS "-std=c++17 -Wall -Werror -g")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_COMPILE_FLAGS}")

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${PROTOBUF_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)

set_target_properties(
    transport_catalogue PROPERTIES
    CXX_STANDART 17
    CXX_STANDART_REQUIRED ON
)
