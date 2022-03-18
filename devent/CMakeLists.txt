cmake_minimum_required(VERSION 3.20)
project(devent C)

set(CMAKE_C_STANDARD 99)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} OS)
message(OS = ${OS})

include_directories(../dlib ./include ../dns)
add_library(devent
        docket.c
        listener.c
        dns.h
        read.c
        event.c
        event_ssl.c
        utils.c
        log.c
        accept.c
        write.c
        buffer.c
        connect.c
        dns.c
        loop.c)

add_subdirectory(test)

if (WIN32)
    set(LIB_SSL "libssl.lib")
    set(LIB_CRYPTO "libcrypto.lib")
else (WIN32)
    set(LIB_SSL "libssl.a")
    set(LIB_CRYPTO "libcrypto.a")
endif ()

if (DEVENT_SSL_ENABLE)
    add_definitions("-DDEVENT_SSL")
    message("Devent ssl is enable")

    message("find ssl from: ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/")

    find_library(
            OPENSSL_SSL
            ${LIB_SSL}

            ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/
            ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/bin/
    )

    find_library(
            OPENSSL_CRYPTO
            ${LIB_CRYPTO}

            ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/
            ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/bin/
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/include)

    message("OPENSSL_SSL: ${OPENSSL_SSL}")
    message("OPENSSL_CRYPTO: ${OPENSSL_CRYPTO}")
endif ()

if (${OS} STREQUAL "windows")
    set(LIBRESOLV "")
else ()
    set(LIBRESOLV "resolv")
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

        # libresolv
        ${LIBRESOLV}
)

add_dependencies(
        devent_test

        devent
)