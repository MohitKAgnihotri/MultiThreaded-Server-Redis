cmake_minimum_required(VERSION 3.19)
project(server)

set(CMAKE_CXX_STANDARD 14)
link_libraries(pthread)

add_executable(server server.c server.h)

add_executable(client client.c client.h)