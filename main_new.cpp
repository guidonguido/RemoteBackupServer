//
// Created by guido on 28/11/20.
//

#include "server_new.h"
#include "User.h"

int main() {
    try{
        server_new server(5004);
        server.run();
    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(-1);
    }
}