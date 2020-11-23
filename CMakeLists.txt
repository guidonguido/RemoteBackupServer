cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread " )

add_executable(RemoteBackup_Server User.cpp main.cpp server.cpp server.h connection_handler.cpp connection_handler.h)


find_package(Boost REQUIRED COMPONENTS serialization filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(RemoteBackup_Server ${Boost_LIBRARIES})
