cmake_minimum_required(VERSION 3.20)
project(docks C)

set(CMAKE_C_STANDARD 99)

include_directories(../dlib)
include_directories(../devent/include include)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} OS)

set(DEVENT_SSL_ENABLE 1)
add_definitions("-DDEVENT_SSL")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/include)

add_library(
        docks

        utils.c
        server.c
        server_context.c
        method.c
        negotication.c
        auth.c
        docks_connect.c
        remote.c
        remote_context.c
        transfer.c)

add_executable(
        docks-server

        docks_server.c
        server_config.c
        utils.c
        server.c
        server_context.c
        method.c
        negotication.c
        auth.c
        docks_connect.c
        remote.c
        remote_context.c
        transfer.c
        client_context.c
)

add_executable(
        docks-client

        server_config.c
        utils.c
        server.c
        server_context.c
        method.c
        negotication.c
        auth.c
        docks_connect.c
        remote.c
        remote_context.c
        transfer.c
        docks_client.c
        client_config.c
        client_context.c)

target_link_libraries(
        docks

        coll-static
        cdns-static
        devent
)

target_link_libraries(
        docks-server

        coll-static
        cdns-static
        devent
)

target_link_libraries(
        docks-client

        coll-static
        cdns-static
        devent
)