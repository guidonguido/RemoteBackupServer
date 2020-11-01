//
// Created by guido on 21/09/20.
//
#include "Server.hpp"
#include "User.h"

int main() {
    try{
        Server server(5000);
        server.run();
    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(-1);
    }
}