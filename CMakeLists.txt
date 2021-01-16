cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread -lstdc++fs" )


add_executable(RemoteBackup_Server User.cpp connection_handler.cpp connection_handler.h server.cpp server.h
        ChecksumAPI/Checksum.cpp ChecksumAPI/Checksum.h ChecksumAPI/SHA256.cpp ChecksumAPI/SHA256.h main_new.cpp)

find_package(Boost REQUIRED COMPONENTS serialization filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(RemoteBackup_Server ${Boost_LIBRARIES})

find_package(OpenSSL REQUIRED)
target_link_libraries(RemoteBackup_Server OpenSSL::SSL)


