//
// Created by guido on 21/09/20.
//
#include "../OldServer/Server.hpp"
#include "server.h"
#include "../User.h"

int main() {
    try{
        server server(5004);
        server.run();
    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(-1);
    }
}