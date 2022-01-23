cmake_minimum_required(VERSION 3.20)
project(devent C)

set(CMAKE_C_STANDARD 99)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} OS)

include_directories(../dlib ./include ../dns)
add_library(devent
        docket.c
        listener.c
        dns.h
        read.c
        event.c
        utils.c
        log.c
        accept.c
        write.c
        buffer.c
        connect.c
        dns.c
        loop.c)

add_subdirectory(test)

if (DEVENT_SSL_ENABLE)
    add_definitions("-DDEVENT_SSL")
    message("Devent ssl is enable")

    message("find ssl from: ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/")

    find_library(
            OPENSSL_SSL
            libssl.a

            ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/
    )

    find_library(
            OPENSSL_CRYPTO
            libcrypto.a

            ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/include)

    message("OPENSSL_SSL: ${OPENSSL_SSL}")
    message("OPENSSL_CRYPTO: ${OPENSSL_CRYPTO}")
endif ()

add_dependencies(
        devent

        # dns
        cdns
)

target_link_libraries(
        devent

        # ssl
        ${OPENSSL_SSL}
        ${OPENSSL_CRYPTO}
)

add_dependencies(
        devent_test

        devent
)