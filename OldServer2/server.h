//
// Created by guido on 23/11/20.
//

#ifndef REMOTEBACKUP_SERVER_SERVER_H
#define REMOTEBACKUP_SERVER_SERVER_H

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

#include "../User.h"

class connection_handler;

class server {
public:


    boost::asio::thread_pool pool;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unordered_set<std::shared_ptr<connection_handler>> active_connections;

    server(int port);

    void run();

    void stop();

private:


    const boost::posix_time::time_duration read_timeout = boost::posix_time::seconds(30);
    const boost::posix_time::time_duration write_timeout = boost::posix_time::seconds(30);

    void accept_connection();

};


#endif //REMOTEBACKUP_SERVER_SERVER_H
