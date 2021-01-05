//
// Created by guido on 28/11/20.
//

#ifndef REMOTEBACKUP_SERVER_SERVER_H
#define REMOTEBACKUP_SERVER_SERVER_H

#include <iostream>
#include <boost/asio.hpp>
#include "connection_handler.h"

class server {
public:


    std::shared_ptr<boost::asio::thread_pool> pool;
    std::shared_ptr<boost::asio::io_context> io_context;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unordered_set<std::shared_ptr<connection_handler>> active_connections;

    server(int port);

    void run();

    void stop();

    std::function<void(std::shared_ptr<connection_handler>)> erase_connection;
    std::function<void(void)> stop_server;


private:


    const boost::posix_time::time_duration read_timeout = boost::posix_time::seconds(30);
    const boost::posix_time::time_duration write_timeout = boost::posix_time::seconds(30);

    void accept_connection();

};


#endif //REMOTEBACKUP_SERVER_SERVER_H
