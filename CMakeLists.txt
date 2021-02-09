cmake_minimum_required(VERSION 3.19)
project(server)

set(CMAKE_CXX_STANDARD 14)
link_libraries(pthread m)

add_executable(server server.c server.h json.c json.h)

add_executable(client client.c client.h json.c json.h)