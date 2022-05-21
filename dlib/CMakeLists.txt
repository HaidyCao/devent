# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.1)

project("dlib")

set(CMAKE_C_STANDARD 99)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

if (CMAKE_HOST_APPLE)
    set(OS darwin)
    set(M_OS darwin)
elseif (CMAKE_HOST_UNIX)
    set(OS linux)
    set(M_OS linux)
endif ()
if (DEFINED ANDROID_ABI)
    set(OS android)
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions("-DEVENT_LOG_DISABLED -O2")
    set(CMAKE_C_LDFLAGS "${CMAKE_C_LDFLAGS} -s")
    message("Release build")
else ()
    add_definitions("-Wall -g")
endif ()

add_definitions("-D_GNU_SOURCE -D__USE_MISC")

add_library( # Sets the name of the library.
        coll

        c_hash.c
        c_hash_set.c
        c_array_map.c
        c_sparse_array.c
        clib.c
        c_linked_list.c
        c_hash_map.c
        c_array_list.c)

add_executable(
        lib_test

        lib_test.c
        c_array_list.c
        log.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.
if (MINGW)
    set(LIB_WS2_32 "ws2_32")
    message(LIB_WS2_32 = ${LIB_WS2_32})
else ()
    set(LIB_WS2_32 "")
endif ()

if (ANDROID AND NOT "${CMAKE_HOST_SYSTEM}" MATCHES "Android-*")
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

    target_link_libraries(
            coll

            ${log-lib}
    )
    target_link_libraries(
            lib_test

            coll
            ${log-lib}
    )
else ()
    target_link_libraries(
            coll

            ${LIB_WS2_32}
    )
    target_link_libraries(
            lib_test

            coll
    )
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/${OS})
endif ()