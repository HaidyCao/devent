cmake_minimum_required(VERSION 3.10)
project(devent_test C)

set(CMAKE_C_STANDARD 99)
string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} OS)

include_directories(../include)
include_directories(../../dlib)

add_executable(devent_test main.c utils_test.c utils_test.h echo_server_test.c echo_server_test.h telnet_test.c telnet_test.h buffer_test.c buffer_test.h ssl_telnet_test.c ssl_telnet_test.h ssl_echo_server_test.c ssl_echo_server_test.h)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie -mcmodel=large")
#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -no-pie -mcmodel=large")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/${OS}/include)

if (WIN32)
    set(CRYPT32 "Crypt32")
endif ()
target_compile_definitions(
        devent
        PRIVATE

        DEVENT_MULTI_THREAD
        DEVENT_SSL
)
add_definitions(
        "-DDEVENT_SSL -DDEVENT_MULTI_THREAD"
)
option(DEVENT_SSL_ENABLE "ssl enable" ON)

target_link_libraries(
        devent_test

        devent
        cdns
        coll
        ${CRYPT32}
)