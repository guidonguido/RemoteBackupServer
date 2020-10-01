//
// Created by guido on 21/09/20.
//
#include "server.hpp"

int main() {
    try{
        Server server(5005);
        server.run();
    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(-1);
    }
}