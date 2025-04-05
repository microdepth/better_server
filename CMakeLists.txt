cmake_minimum_required(VERSION 3.26)
project(better_server)

set(CMAKE_CXX_STANDARD 20)

set(PSNETLIB_ROOT "C:/Users/psdab/CLionProjects/your_psnetlib_folder")
set(ASIO_ROOT "C:/Users/psdab/Downloads/asio-1.30.2/asio-1.30.2")

include_directories(${PSNETLIB_ROOT}/include ${ASIO_ROOT}/include)

file(GLOB_RECURSE ASIO_HEADERS ${ASIO_ROOT}/include/asio/*.hpp)

add_executable(better_server main.cpp ${ASIO_HEADERS}
        net_common.h
        net_message.h
        ps_net.h
        net_tsqueue.h
        net_connection.h
        net_server.h
        net_client.h
)

target_link_libraries(better_server wsock32 ws2_32)
