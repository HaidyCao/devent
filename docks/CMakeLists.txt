cmake_minimum_required(VERSION 3.20)
project(docks C)

set(CMAKE_C_STANDARD 99)

include_directories(../dlib)
include_directories(../devent/include include)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} OS)
message("docks OS = ${OS}")

set(DEVENT_SSL_ENABLE 1)
add_definitions("-DDEVENT_SSL")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/include)

if (WIN32)
    set(
            WIN_SRC

            ../getopt.c
    )
endif ()

add_library(
        docks

        ${WIN_SRC}
        ../dlib/log.c
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

        ${WIN_SRC}
        ../dlib/log.c
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

        ${WIN_SRC}
        ../dlib/log.c
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

        devent
        cdns
        coll
)

target_link_libraries(
        docks-server

        devent
        cdns
        coll
)

target_link_libraries(
        docks-client

        devent
        cdns
        coll
)