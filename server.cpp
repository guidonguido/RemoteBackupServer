//
// Created by guido on 28/11/20.
//

#include "server.h"

server::server(int port):
        io_context(new boost::asio::io_context()),
        acceptor_(*io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port),
                false){

    this->pool = std::make_shared<boost::asio::thread_pool>(4);

    // After the connection is closed, it can be erased from the active_connections
    erase_connection = [this](std::shared_ptr<connection_handler> connection_handler) {
        active_connections.erase(connection_handler);
    };

    stop_server = [this](){
        stop();
    };
}

void server::run(){
    std::cout << "Listening on " << acceptor_.local_endpoint() << std::endl;
    accept_connection();
    io_context->run();
}

void server::stop(){
    acceptor_.close();
    pool->join();    // Wait for all task in the pool to complete
    std::vector<std::shared_ptr<connection_handler>> session_to_close; //to iterate over map
    std::copy(active_connections.begin(), active_connections.end(), back_inserter(session_to_close));
    for(auto& s: session_to_close){
        s->shutdown_();
    }
    active_connections.clear();
    io_context->stop();
}

void server::accept_connection(){
    if(acceptor_.is_open()){
        std::shared_ptr<connection_handler> session = std::make_shared<connection_handler>(
                pool, io_context, erase_connection, stop_server );
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

