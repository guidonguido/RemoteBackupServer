cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -pthread )


add_executable(RemoteBackup_Server User.cpp main.cpp)