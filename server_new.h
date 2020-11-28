//
// Created by guido on 28/11/20.
//

#ifndef REMOTEBACKUP_SERVER_SERVER_NEW_H
#define REMOTEBACKUP_SERVER_SERVER_NEW_H


#include <iostream>
#include <regex>

#include <boost/serialization/unordered_map.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <unordered_set>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#include "User.h"
#include "connection_handler_new.h"

class server_new {
public:


    std::shared_ptr<boost::asio::thread_pool> pool;
    std::shared_ptr<boost::asio::io_context> io_context;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unordered_set<std::shared_ptr<connection_handler_new>> active_connections;

    server_new(int port);

    void run();

    void stop();

    std::function<void(std::shared_ptr<connection_handler_new>)> erase_connection;
    std::function<void(void)> stop_server;



private:


    const boost::posix_time::time_duration read_timeout = boost::posix_time::seconds(30);
    const boost::posix_time::time_duration write_timeout = boost::posix_time::seconds(30);

    void accept_connection();

};


#endif //REMOTEBACKUP_SERVER_SERVER_NEW_H
