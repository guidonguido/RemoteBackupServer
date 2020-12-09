cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread" )

add_executable(RemoteBackup_Server User.cpp OldServer2/server.cpp OldServer2/server.h OldServer2/connection_handler.cpp
        OldServer2/connection_handler.h connection_handler_new.cpp connection_handler_new.h server_new.cpp server_new.h
        ChecksumAPI/Checksum.cpp ChecksumAPI/Checksum.h ChecksumAPI/SHA256.cpp ChecksumAPI/SHA256.h main_new.cpp)


find_package(Boost REQUIRED COMPONENTS serialization filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(RemoteBackup_Server ${Boost_LIBRARIES})
