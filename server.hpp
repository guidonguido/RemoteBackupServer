//
// Created by guido on 18/09/20.
//
#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unordered_set>
#include "User.h"

// TO-DO-NEXT Separa l'oggetto del comando dal comando ricevuto ;)
// E cerca di estrarre un user dalla tabella LOGIN_INFO

class Server {

    class connection_handler: public std::enable_shared_from_this<connection_handler> {

        boost::asio::ip::tcp::socket socket_;

        friend class Server;
        Server& server_;
        boost::asio::streambuf buf_in_;
        std::mutex buffers_mtx_;
        std::vector<std::string> buffers_[2]; // a double buffer
        std::vector<boost::asio::const_buffer> buffer_seq_;
        int active_buffer_ = 0;
        bool closing_ = false;
        bool closed_ = false;
        boost::asio::deadline_timer read_timer_, write_timer_;

        std::optional<User> logged_user;

        void process_command(const std::string& cmd){

            if(cmd == "stop") {
                server_.stop();
                return;
            }
            if(cmd == "close"){
                close_();
                return;
            }

            if(!logged_user.has_value()){
                if(cmd == "login"){

                    // logged_user = User::check_login()

                    server_.io_context.post([this](){
                        handle_read();
                    });
                    return;
                }
                write_("Login needed to use the Back-up Server\n");

                // richiedi al context di eseguire una nuova read
                // dopo aver dato la risposta al command
                // si comporta come un pool di thread
                server_.io_context.post([this](){
                    handle_read();
                });
            } else{
                // Post new function to execute to server pool
                boost::asio::post(server_.pool, [this, cmd] {
                    std::ostringstream oss;
                    oss << "[+] Server executing response on thread #" << std::this_thread::get_id() << std::endl;
                    write_(oss.str());
                    for(int i=0; i < 123; i++){
                        write_("Hello, "+cmd+" "+std::to_string(i+1)+"\r\n");
                    }

                    // richiedi al context di eseguire una nuova read
                    // dopo aver dato la risposta al command
                    // si comporta come un pool di thread
                    server_.io_context.post([this](){
                        handle_read();
                    });
                });

            }

        }

        void handle_read(){
            /**read_timer_.expires_from_now(server_.read_timeout);
            read_timer_.async_wait([this](boost::system::error_code err){
                if(!err){
                    std::cout << "Read timeout occurred " << std::endl;
                    shutdown_();
                }
            });*/

            //read until delimeter "\n"
            boost::asio::async_read_until(socket_, buf_in_, "\n",
                                          [this, self = shared_from_this()](boost::system::error_code err, std::size_t bytes_read){
                                              read_timer_.cancel();
                                              if(!err){
                                                  const char* message = boost::asio::buffer_cast<const char*>(buf_in_.data());
                                                  std::string message_command(message, bytes_read - (bytes_read > 1 && message[bytes_read - 2] == '\r' ? 2 : 1));
                                                  buf_in_.consume(bytes_read);
                                                  process_command(message_command);
                                              }
                                              else{
                                                  close_();
                                                  throw err;
                                              }

                                          });
        }// handle_read

        void handle_write(){
            active_buffer_ ^= 1; //XOR -- if is 1 put 0, if 0 put 1. Switch buffers
            for(const auto &data: buffers_[active_buffer_]){
                buffer_seq_.push_back(boost::asio::buffer(data));
            }
            /*write_timer_.expires_from_now(server_.write_timeout);
            write_timer_.async_wait([this](boost::system::error_code err){
                if(err){
                    std::cout << "Write timeout" << std::endl;
                    shutdown_();
                    throw err;
                }
            });*/

            boost::asio::async_write(socket_, buffer_seq_, [this](const boost::system::error_code& err, std::size_t bytes_transferred){
                write_timer_.cancel();
                std::lock_guard l(buffers_mtx_);
                buffers_[active_buffer_].clear();
                buffer_seq_.clear();
                if(!err){
                    std::cout << "Wrote " << bytes_transferred << " bytes" << std::endl;
                    if(!buffers_[active_buffer_ ^ 1].empty()) //the other buffer contains data to write
                        handle_write();
                } else{
                    close_();
                    throw err;
                }
            });
        }

        bool writing() const { return !buffer_seq_.empty();}

    public:

        connection_handler(Server* server): server_(*server), read_timer_(server->io_context),
                                            write_timer_(server->io_context), socket_(server->io_context),
                                            logged_user(std::nullopt){}

        void start(){
            std::cout << "Connection created " << std::endl;
            socket_.set_option(boost::asio::ip::tcp::no_delay(true));
            std::ostringstream oss;
            oss << "[++] Commands: " << std::endl;
            oss << "\t login [username] [password] -- login to the Back-up Server " << std::endl;
            oss << "\t close                       -- close the connetion to Server" << std::endl;
            oss << "\t stop                        -- stop the Server" << std::endl;
            write_(oss.str());
            handle_read();
        }

        // socket creation
        boost::asio::ip::tcp::socket& get_socket(){
            return socket_;
        }

        void close_(){
            closing_ = true;
            if(!writing())
                shutdown_();
        }

        void shutdown_(){
            if(!closed_){
                closing_ = closed_ = true;
                boost::system::error_code err;
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
                socket_.close();
                server_.active_connections.erase(shared_from_this());
            }
        }

        void write_(std::string&& data){
            std::lock_guard l(buffers_mtx_);
            buffers_[active_buffer_^1].push_back(std::move(data));
            if(!writing())
                handle_write();
        }
    }; //connection_handler

    boost::asio::thread_pool pool;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unordered_set<std::shared_ptr<connection_handler>> active_connections;

    const boost::posix_time::time_duration read_timeout = boost::posix_time::seconds(30);
    const boost::posix_time::time_duration write_timeout = boost::posix_time::seconds(30);

    void accept_connection(){
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

public:
    Server(int port):
        acceptor_(this->io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port), false),
        pool(2){}

    void run(){
        std::cout << "Listening on " << acceptor_.local_endpoint() << std::endl;
        accept_connection();
        io_context.run();
    }

    void stop(){
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

};




