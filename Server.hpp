//
// Created by guido on 18/09/20.
//
#pragma once

#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/impl/basic_text_oarchive.ipp>
#include <boost/archive/impl/text_oarchive_impl.ipp>
#include <boost/archive/basic_text_oprimitive.hpp>

#include <boost/serialization/unordered_map.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unordered_set>
#include "User.h"

// Separa l'oggetto del comando dal comando ricevuto ;)
// Cerca di estrarre un user dalla tabella LOGIN_INFO -- Fatto. Login implementato

// TO-DO-NEXT Decidere che fomato devono avere le info sul filesystem
// IDEA: Tree di directory_entry + checksum (o file_status)

// Aggiungere lettura di un Oggetto serializzato header + data (se necessario)

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

        std::optional<std::reference_wrapper<User>> logged_user;

        // Separate command from its argument
        std::vector<std::string> parse_command(const std::string cmd){
            size_t prev = 0, pos = 0;
            std::vector<std::string> tokens;
            do
            {
                pos = cmd.find(" ", prev);
                if (pos == std::string::npos) pos = cmd.length();
                std::string token = cmd.substr(prev, pos-prev);
                if (!token.empty()) tokens.push_back(token);
                prev = pos + std::string(" ").length();
            }while (pos < cmd.length() && prev < cmd.length());

            if(tokens.size() > 0)
                return tokens;
        }

        void process_command(std::string& cmd){

            std::vector<std::string> arguments = parse_command(cmd);
            cmd = arguments[0];

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

                    if(arguments.size() == 3){
                        std::string username = arguments[1];
                        std::string password = arguments[2];
                        logged_user = User::check_login(std::move(username), std::move(password));
                        if(logged_user.has_value()){
                            write_str("Login SUCCESS\n[" + logged_user->get().get_username() + "] >> ");
                        }else{
                            write_str("Login FAILED, try again inserting correct username and password\n>> ");
                        }

                        server_.io_context.post([this](){
                            handle_read();
                        });
                        return;
                    }else{
                        write_str("Login format is incorrect, try again inserting both username and password\n>> ");
                        server_.io_context.post([this](){
                            handle_read();
                        });
                    }//if-else
                }//if(login)

                write_str("Login needed to use the Back-up Server\n>> ");

                // richiedi al context di eseguire una nuova read
                // dopo aver dato la risposta al command
                // si comporta come un pool di thread
                server_.io_context.post([this](){
                    handle_read();
                });
            } else{
                // Post new function to execute to server pool
                if(cmd == "checkFilesystemStatus"){
                    boost::asio::post(server_.pool, [this] {
                        std::ostringstream oss;
                        write_(logged_user.value().get().get_filesystem_status());
                        write_str(">> ");
                    });
                }
                else if(arguments.size() > 1){
                    std::string argument = arguments[1];
                    boost::asio::post(server_.pool, [this, cmd, argument] {
                        std::ostringstream oss;
                        oss << "[+] Server executing response on thread #" << std::this_thread::get_id() << std::endl;
                        oss << "[+] The command " << cmd << " is not known. But you can join this Hello messages" << std::endl;
                        write_str(oss.str());
                        for(int i=0; i < 123; i++){
                            write_str("Hello, "+cmd+" "+argument+" " +std::to_string(i+1)+"\r\n");
                        }
                        write_str(">> ");
                    });
                }
                else{
                    write_str("You should insert at least one argument for the desired command\n>> ");
                }

                // richiedi al context di eseguire una nuova read
                // dopo aver dato la risposta al command
                // si comporta come un pool di thread
                server_.io_context.post([this](){
                    handle_read();
                });
            }//if(logged_user_command)

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
            oss << ">> ";
            write_str(oss.str());
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


        template <typename T>
        void write_(T&& t){
            // Serialize the data first so we know how large it is.
            std::ostringstream archive_stream;
            boost::archive::text_oarchive archive(archive_stream);
            archive << t;
            std::string outbound_data_ = archive_stream.str();

            // Format the header.
            std::ostringstream header_stream;
            header_stream << std::setw(8)
                          << std::hex << outbound_data_.size();
            std::string outbound_header_ = header_stream.str();

            std::lock_guard l(buffers_mtx_);
            buffers_[active_buffer_^1].push_back(std::move(outbound_header_));
            buffers_[active_buffer_^1].push_back(std::move(outbound_data_));
            if(!writing())
                handle_write();
        }

        void write_str (std::string&& data){
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




