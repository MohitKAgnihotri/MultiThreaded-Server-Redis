cmake_minimum_required(VERSION 3.19)
project(server)

set(CMAKE_CXX_STANDARD 14)
link_libraries(pthread m)

add_executable(server server.c server.h json.c json.h database.c database.h sharedmemory.c sharedmemory.h)
target_link_libraries(server /mnt/c/Workspace/Chegg/MultiThreaded-Server-Redis/library/libhiredis.so)
target_include_directories(server PUBLIC includes)

add_executable(client client.c client.h json.c json.h)

add_executable(test test.c)
target_include_directories(test PUBLIC includes)
target_link_libraries(test /mnt/c/Workspace/Chegg/MultiThreaded-Server-Redis/library/libhiredis.so)
