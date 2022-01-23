cmake_minimum_required(VERSION 3.10.2)
# Declares and names the project.
project("docket")

IF (ANDROID)
    add_library( # Sets the name of the library.
            docket-jni

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            native-lib.cpp)
ENDIF (ANDROID)

IF (ANDROID)
    find_library( # Sets the name of the path variable.
            log-lib

            # Specifies the name of the NDK library that
            # you want CMake to locate.
            log)
ENDIF (ANDROID)

add_subdirectory(dlib)
add_subdirectory(dns)
add_subdirectory(devent)
add_subdirectory(docks)

add_dependencies(
        devent

        cdns-static
        coll-static
)

add_dependencies(
        docks

        coll-static
        devent
)

add_dependencies(
        dns_test

        coll-static
)

IF (ANDROID)
    target_link_libraries( # Specifies the target library.
            docket-jni
            coll-static
            docket

            # Links the target library to the log library
            # included in the NDK.
            ${log-lib})
ENDIF (ANDROID)
