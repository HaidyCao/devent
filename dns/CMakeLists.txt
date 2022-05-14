# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


message("CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}; M_OS = ${CMAKE_HOST_SYSTEM}")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions("-DEVENT_LOG_DISABLED -O2")
    set(CMAKE_C_LDFLAGS "${CMAKE_C_LDFLAGS} -s")
    message("Release build")
else ()
    add_definitions("-Wall")
endif ()

add_definitions("-DEVENT_LOG_DISABLED")

add_library( # Sets the name of the library.
        cdns

        c_dns.c
        c_dns_header.c)

if (ANDROID)
    if ("${CMAKE_HOST_SYSTEM}" MATCHES "Android-*")
        if (${CMAKE_CL_64})
            find_library(
                    log-lib

                    log
                    /system/lib64
            )
        else ()
            find_library(
                    log-lib

                    log
                    /system/lib
            )
        endif ()
    else ()
        find_library(
                log-lib

                log)
    endif ()
else ()
    set(log-lib "")
    add_executable(
            dns_test

            ../dlib/clib.c
            c_dns_test.c
    )

    target_link_libraries(
            dns_test

            cdns
    )
endif ()

target_link_libraries(
        cdns

        ${log-lib}
)
