//
// Created by guido on 23/11/20.
//

#include "connection_handler.h"

#define DEBUG 0

connection_handler::command_code connection_handler::hashit(std::string const& command){
    if(command == "addFile") return addFile;
    if(command == "updateFile") return updateFile;
    if(command == "removeFile") return removeFile;
    if(command == "checkFilesystemStatus") return checkFilesystemStatus;
    if(command == "checkFileVersion") return checkFileVersion;
    return unknown;
}

// Separate command from its argument
std::vector<std::string> connection_handler::parse_command(const std::string cmd){
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

    return tokens;


}

void connection_handler::process_unknown(const std::string& cmd, const std::string& argument){
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
    server_.io_context.post([this](){
        handle_read();
    });
}

//TO TEST
void connection_handler::process_addFile(const std::vector<std::string>& arguments){
    std::string path_ = arguments[1];
    path_ = logged_user->get_folder_path() +"/"  +path_;
    boost::filesystem::path path_boost(path_.substr(0, path_.find_last_of("\\/")));
    std::cout << "new File path " << path_boost.string() << std::endl;

    std::string file_size_ = arguments[2];

    if(DEBUG) write_str("[+] Add File request received\n");
    boost::filesystem::create_directories(path_boost);

    boost::asio::post(server_.pool, [this, path_, file_size_] {
        std::stringstream sstream(file_size_);
        size_t file_size;
        sstream >> file_size;
        server_.io_context.post([this, path_, &file_size](){
            std::cout << "new File name " << path_ << std::endl;
            std::ofstream output_file (path_);
            if( !output_file ){
                if(DEBUG) write_str("[+] Error happened opening the file\n>> ");
                std::cout << "Error happened opening the file\n";
            }
            else{
                handle_file_read(output_file, file_size);
                if(DEBUG) write_str("[+] File received\n>> ");
            }
        });

        // A seguito della lettura del comando addFile [path] segue un \n, quindi il file
        // Completa la lettura con handle_read_file()
        // Quindi invia ad User il path + file letto
    });
    /**server_.io_context.post([this](){
        handle_read();
    });*/
}

void connection_handler::process_removeFile(const std::vector<std::string>& arguments){
    std::string path = arguments[1];
    boost::asio::post(server_.pool, [this, path] {
        std::filesystem::remove(logged_user->get_folder_path() + "/" + path);
    });
    if(DEBUG) write_str("[+] File removed\n>> ");
    server_.io_context.post([this](){
        handle_read();
    });
}

//TO TEST
void connection_handler::process_updateFile(const std::vector<std::string>& arguments){
    std::string path_ = arguments[1];
    path_ = logged_user->get_folder_path() +"/"  +path_;
    boost::filesystem::path path_boost(path_.substr(0, path_.find_last_of("\\/")));

    std::string file_size_ = arguments[2];
    if(DEBUG) write_str("[+] Update File request received\n");

    boost::filesystem::create_directories(path_boost);

    boost::asio::post(server_.pool, [this, path_, file_size_] {
        std::stringstream sstream(file_size_);
        size_t file_size;
        sstream >> file_size;
        server_.io_context.post([this, &path_, &file_size](){
            std::ofstream output_file (path_);
            handle_file_read(output_file, file_size);
            if(DEBUG) write_str("[+] File received\n>> ");
        });

        // A seguito della lettura del comando updateFile [path] segue un \n, quindi il file
        // Completa la lettura con handle_read_file()
        // Quindi invia ad User il path + file letto
    });
    server_.io_context.post([this](){
        handle_read();
    });
}

void connection_handler::process_command(std::string& cmd){

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
                    if(DEBUG) write_str("Login SUCCESS\n[" + logged_user->get_username() + "] >> ");
                }else{
                    if(DEBUG) write_str("Login FAILED, try again inserting correct username and password\n>> ");
                }

                server_.io_context.post([this](){
                    handle_read();
                });
                return;
            }else{
                if(DEBUG) write_str("Login format is incorrect, try again inserting both username and password\n>> ");
                server_.io_context.post([this](){
                    handle_read();
                });
            }//if-else
        }//if(login)

        if(DEBUG) write_str("Login needed to use the Back-up Server\n>> ");

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
                write_(logged_user.value().get_filesystem_status());
                if(DEBUG) write_str(">> ");
            });
        }
        else if(arguments.size() > 1){
            std::string argument = arguments[1];

            switch(hashit(cmd)){
                case addFile:
                    process_addFile(arguments);
                    break;

                case removeFile:
                    process_removeFile(arguments);
                    break;

                case updateFile:
                    process_updateFile(arguments);
                    break;

                case unknown:
                    process_unknown(cmd, argument);
                    break;

            }

        }
        else{
            write_str("You should insert at least one argument for the desired command\n>> ");
        }

        // richiedi al context di eseguire una nuova read
        // dopo aver dato la risposta al command
        // si comporta come un pool di thread
        /*server_.io_context.post([this](){
            handle_read();
        });*/
    }//if(logged_user_command)

}

void connection_handler::handle_read(){
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
                                          //debug
                                          if(bytes_read > 1){
                                              std::cout << "Message read: " << message_command << ", bytes_read: " << bytes_read << std::endl;
                                              process_command(message_command);
                                          }

                                      }
                                      else{
                                          close_();
                                          throw err;
                                      }

                                  });
}// handle_read

// TO TEST!!
// IDEA Trasforma la lettura in sincrona. La funzione sarà eseguito su un thread apposito e non c'è
//      bisogno che venga eseguita la lettura async
void connection_handler::handle_file_read(std::ofstream& output_file, size_t file_size){
    boost::array<char, 1024> buf;
    boost::system::error_code error;
    bool end = false;
    //write_str("[+] I'm reading the file...\n");

    while( !end ){
        size_t len = socket_.read_some(boost::asio::buffer(buf), error);
        if(!error){
            if (len>0) {
                std::cout << "Parte di file: " << buf.c_array() << std::endl;
                output_file.write(buf.c_array(), (std::streamsize) len);
            }
            if (output_file.tellp()>= (std::fstream::pos_type)(std::streamsize)file_size)
                end=true; // file was received
        }
        else{
            close_();
            throw std::runtime_error("read_file error");
        }
        std::cout << "received " << output_file.tellp() << " bytes.\n";
    }
    output_file.close();
    std::cout<< "File read successfully" << std::endl;
    server_.io_context.post([this](){
        handle_read();
    });

}// handle_file_read

void connection_handler::handle_write(){
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

    boost::asio::async_write(socket_, buffer_seq_, [this](
            const boost::system::error_code& err, std::size_t bytes_transferred){
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

bool connection_handler::writing() const { return !buffer_seq_.empty();}


connection_handler::connection_handler(server* server): server_(*server), read_timer_(server->io_context),
write_timer_(server->io_context), socket_(server->io_context),
logged_user(std::nullopt){}

void connection_handler::start(){
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
boost::asio::ip::tcp::socket& connection_handler::get_socket(){
    return socket_;
}

void connection_handler::close_(){
    closing_ = true;
    if(!writing())
        shutdown_();
}

void connection_handler::shutdown_(){
    if(!closed_){
        closing_ = closed_ = true;
        boost::system::error_code err;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
        socket_.close();
        server_.active_connections.erase(shared_from_this());
    }
}


template <typename T>
void connection_handler::write_(T&& t){
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

void connection_handler::write_str (std::string&& data){
    std::lock_guard l(buffers_mtx_);
    buffers_[active_buffer_^1].push_back(std::move(data));
    if(!writing())
        handle_write();
}