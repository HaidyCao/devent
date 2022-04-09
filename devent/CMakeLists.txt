cmake_minimum_required(VERSION 3.10)
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
        file_event.c
        event_ssl.c
        utils.c
        log.c
        accept.c
        write.c
        buffer.c
        event_lock.c
        connect.c
        dns.c
        loop.c)

add_subdirectory(test)

if (WIN32)
    if (MINGW)
        set(LIB_SSL "libssl.a")
        set(LIB_CRYPTO "libcrypto.a")
    else ()
        set(LIB_SSL "libssl.lib")
        set(LIB_CRYPTO "libcrypto.lib")
    endif ()
else (WIN32)
    set(LIB_SSL "libssl.a")
    set(LIB_CRYPTO "libcrypto.a")
endif ()

if (DEVENT_SSL_ENABLE)
    add_definitions("-DDEVENT_SSL")
    message("Devent ssl is enable")

    string(REGEX REPLACE "(.*/)\(.*\)" "\\1" DEVENT_C_COMPILER_DIR ${CMAKE_C_COMPILER})
#    set(CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/lib/)

    message("find ssl from: ../dependencies/${OS}/lib/")
    message("find ssl from: ../dependencies/${OS}/bin/")
    message("LIB_SSL = ${LIB_SSL}, LIB_CRYPTO = ${LIB_CRYPTO}")

    find_library(
            OPENSSL_SSL
            ${LIB_SSL}

            ../dependencies/${OS}/lib/
            ../dependencies/${OS}/bin/
    )

    find_library(
            OPENSSL_CRYPTO
            ${LIB_CRYPTO}

            ../dependencies/${OS}/lib/
            ../dependencies/${OS}/bin/
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS}/include)

    message("OPENSSL_SSL: ${OPENSSL_SSL}")
    message("OPENSSL_CRYPTO: ${OPENSSL_CRYPTO}")
endif ()

if (WIN32)
    set(LIBRESOLV "")
    set(LIB_PTHREAD "")
    set(LIB_DL "")
    if (MINGW)
        set(LIBIPHLPAPI "IPHLPAPI")
        SET(LIBWSOCK32 "wsock32")
    else ()
        set(LIBIPHLPAPI "")
    endif ()
else ()
    set(LIBIPHLPAPI "")
    set(LIBRESOLV "resolv")
    find_library(LIB_PTHREAD pthread)
    find_library(LIB_DL dl)

    message("LIB_PTHREAD = ${LIB_PTHREAD}")
    message("LIB_DL = ${LIB_DL}")
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

        ${LIB_DL}
        ${LIB_PTHREAD}

        # libresolv
        ${LIBRESOLV}
        ${LIBIPHLPAPI}
        ${LIBWSOCK32}
)

add_dependencies(
        devent_test

        devent
)