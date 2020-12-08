//
// Created by guido on 23/11/20.
//

#include "server.h"
#include "connection_handler.h"

server::server(int port):
        acceptor_(this->io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port), false),
        pool(2){}

void server::run(){
    std::cout << "Listening on " << acceptor_.local_endpoint() << std::endl;
    accept_connection();
    io_context.run();
}

void server::stop(){
    acceptor_.close();
    pool.join();    // Wait for all task in the pool to complete
    std::vector<std::shared_ptr<connection_handler>> session_to_close; //to iterate over map
    std::copy(active_connections.begin(), active_connections.end(), back_inserter(session_to_close));
    for(auto& s: session_to_close){
        s->shutdown_();
    }
    active_connections.clear();
    io_context.stop();
}

void server::accept_connection(){
    if(acceptor_.is_open()){
        auto session = std::make_shared<connection_handler>(this);
        acceptor_.async_accept(session->socket_, [this, session](boost::system::error_code err){
            if( !err ){
                active_connections.insert(session);
                session->start();
            }
            else{
                std::cerr << "error accept: " << err.message() << std::endl;
                throw err;
            }
            accept_connection();
        });

    }
}