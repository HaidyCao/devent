cmake_minimum_required(VERSION 3.20)
project(devent_test C)

set(CMAKE_C_STANDARD 99)

include_directories(../include)
include_directories(../../dlib)

add_executable(devent_test main.c utils_test.c utils_test.h echo_server_test.c echo_server_test.h telnet_test.c telnet_test.h buffer_test.c buffer_test.h ssl_telnet_test.c ssl_telnet_test.h ssl_echo_server_test.c ssl_echo_server_test.h)


if (DEVENT_SSL_ENABLE)
    add_definitions("-DDEVENT_SSL")
    message("Devent ssl is enable")

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/${OS}/include)
endif ()

target_link_libraries(
        devent_test

        coll-static
        cdns-static
        devent
)