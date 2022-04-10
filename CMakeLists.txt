cmake_minimum_required(VERSION 3.10.2)
# Declares and names the project.
project("docket")

add_subdirectory(dlib)
add_subdirectory(dns)
add_subdirectory(devent)
add_subdirectory(docks)

add_dependencies(
        devent

        cdns
        coll
)

add_dependencies(
        docks

        coll
        devent
)

if (ANDROID)
else ()
    add_dependencies(
            dns_test

            coll
    )
endif ()
