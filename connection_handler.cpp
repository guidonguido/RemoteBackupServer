//
// Created by guido on 28/11/20.
//

#include "connection_handler.h"

#define DEBUG 0

/*
 * Constructor
 */
connection_handler::connection_handler(std::shared_ptr<boost::asio::thread_pool> pool,
                                       std::shared_ptr<boost::asio::io_context> io_context,
                                       std::function<void(std::shared_ptr<connection_handler>)>& erase_callback,
                                       std::function<void()>& stop_callback):
                                            read_timer_(*io_context),
                                            write_timer_(*io_context),
                                            socket_(*io_context),
                                            pool(pool),
                                            io_context(io_context),
                                            erase_callback(erase_callback),
                                            stop_callback(stop_callback),
                                            logged_user(std::nullopt){}

/*
 * Get a code representation of possible commands
 */
connection_handler::command_code connection_handler::hashit(std::string const& command){
    if(command == "addFile") return addFile;
    if(command == "getFile") return getFile;
    if(command == "updateFile") return updateFile;
    if(command == "removeFile") return removeFile;
    if(command == "checkFilesystemStatus") return checkFilesystemStatus;
    if(command == "checkFileVersion") return checkFileVersion;
    return unknown;
}

/*
 * Separate command from its argument
 */
std::vector<std::string> connection_handler::parse_command(const std::string cmd){
    try{
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
    } catch (std::exception& e) {
        std::cout << "[ERROR] parse_command Error: " << e.what() << std::endl;
        close_();
        throw e;
    }

}

/*
 * TODO alternative processing
 */
void connection_handler::process_unknown(const std::string& cmd, const std::string& argument){
    boost::asio::post(*pool, [this, cmd, argument] {
        std::ostringstream oss;
        oss << "[+] Server executing response on thread #" << std::this_thread::get_id() << std::endl;
        oss << "[+] The command " << cmd << " is not known. But you can join this Hello messages" << std::endl;
        if(DEBUG) write_str(oss.str());
        for(int i=0; i < 123; i++){
            if(DEBUG) write_str("Hello, "+cmd+" "+argument+" " +std::to_string(i+1)+"\r\n");
        }
        if(DEBUG) write_str(">> ");
    });
    read_command();
}

/*
 * commands:
 * >> addFile or updateFile <path> <file_size>
 */
void connection_handler::process_addOrUpdateFile(const std::vector<std::string>& arguments){
    boost::asio::post(*pool, [this, arguments] {
        try{
            std::string path_ = arguments[1];
            std::string file_size_ = arguments[2];
            path_ = logged_user->get_folder_path() +"/"  +path_;

            // directory path
            boost::filesystem::path path_boost(path_.substr(0, path_.find_last_of("\\/")));

            std::cout << "\n[socket "<< &this->socket_ << "]addFile request " << path_boost.string() << std::endl;
            if(DEBUG) std::cout << "\n[socket "<< &this->socket_ << "]new File directory path " << path_boost.string() << std::endl;

            if(DEBUG) write_str("[+] Add File request received\n");
            write_str("[SERVER_SUCCESS] Command received\n");
            boost::filesystem::create_directories(path_boost);

            std::stringstream sstream(file_size_);
            size_t file_size;
            sstream >> file_size;

            io_context->post([this, path_, &file_size](){
                std::cout << "new File complete path " << path_ << std::endl;
                std::ofstream output_file (path_);
                if( !output_file ){
                    if(DEBUG) write_str("[+] Error happened opening the file\n>> ");
                    std::cout << "Error happened opening the file\n";
                    read_command();
                }
                else{
                    handle_file_read(output_file, file_size);
                    if(DEBUG) write_str("[+] File received\n>> ");
                }
            });

            // A seguito della lettura del comando addFile [path] segue un \n, quindi il file
        } catch (std::exception& e) {
            std::cout << "[ERROR] process_addOrUpdateFile Error: " << e.what() << std::endl;
            close_();
            throw e;
        }

    });
}// process_addOrUpdateFile

/*
 * command:
 * >> getFile <path>
 */
void connection_handler::process_getFile(const std::vector<std::string> &arguments) {
    std::string path_ = arguments[1];
    path_ = logged_user->get_folder_path() +"/"  +path_;
    std::cout << "\n[socket "<< &this->socket_ << "]getFile request " << path_ << std::endl;

    boost::asio::post(*pool, [this, path_]{
        std::ifstream input_file(path_, std::ios_base::binary | std::ios_base::ate);
        if(!input_file){
            write_str("[+] Error happened opening the file\n>> ");
            std::cout << "Error happened opening the file\n";
        }
        else{
            try {
                // 1. Server first response OK + <file_size>
                std::ostringstream response;
                response << "[SERVER_SUCCESS] ";
                response << std::filesystem::file_size(path_);
                response << "\n";
                write_str(response.str());

            } catch(std::filesystem::filesystem_error& e) {
                // Happens if path points to a folder
                if(DEBUG){
                    std::ostringstream oss;
                    oss << "[+] Error happened: ";
                    oss << e.what();
                    oss << "\n>> ";
                    write_str(oss.str());
                }
                std::cout << e.what() << '\n';
                read_command();
                return;
            }

            // 2. Server reads from client to synchronize for file_send
            // busy wait
            std::string response = read_string();

            //TODO Remove debugging prints
            if(DEBUG) std::cout << "File size tellg() = " << input_file.tellg() << std::endl;
            if(DEBUG) std::cout << "File size std::filesystem::file_size(path_) = " << std::filesystem::file_size(path_) << std::endl;

            // 3. Server sends the requested file
            write_file(input_file, path_);
            std::cout<< "File sent successfully" << std::endl;

            read_command();
        }
    });
}


/*
 * command:
 * >> removeFile <path>
 */
void connection_handler::process_removeFile(const std::vector<std::string>& arguments){

    std::string path = arguments[1];
    boost::asio::post(*pool, [this, path] {
        try{
            write_str("[SERVER_SUCCESS] Command received\n");
            std::filesystem::remove(logged_user->get_folder_path() + "/" + path);
            write_str("[SERVER_SUCCESS] File deleted successfully\n");
        } catch (std::exception& e) {
            std::cout << "[ERROR] process_removeFile Error: " << e.what() << std::endl;
            if(DEBUG) write_str("[+] Error happened removing the file\n>> ");
        }

    });
    if(DEBUG) write_str("[+] File removed\n>> ");
    read_command();
}// process_removeFile


/* command:
 * >> checkFilesystemStatus
 */
void connection_handler::process_checkFilesystemStatus() {
    boost::asio::post(*pool, [this] {
        write_str("[SERVER_SUCCESS] Command received\n");

        write_(logged_user.value().get_filesystem_status());
        if(DEBUG) write_str(">> ");
        read_command();
    });
}// process_removeFile


/*
 * command:
 * login <user> <password>
 */
void connection_handler::process_login(const std::vector<std::string>& arguments){
    try{
        write_str("[SERVER_SUCCESS] Command received\n");

        if(arguments.size() == 3){
            std::string username = arguments[1];
            std::string password = arguments[2];
            logged_user = User::check_login(std::move(username), std::move(password));
            if(logged_user.has_value()){
                if(DEBUG) write_str("Login SUCCESS\n[" + logged_user->get_username() + "] >> ");
            }else{
                if(DEBUG) write_str("Login FAILED, try again inserting correct username and password\n>> ");
            }
        }else{
            if(DEBUG) write_str("Login format is incorrect, try again inserting both username and password\n>> ");
        }//if-else
    } catch (std::exception& e) {
        std::cout << "[ERROR] process_login Error: " << e.what() << std::endl;
        close_();
        throw e;
    }

}// process_login

/*
 * After a command is read
 * Parsing ad processing of known commands
 */
void connection_handler::process_command(std::string cmd){

    // split command and its arguments
    std::vector<std::string> arguments = parse_command(cmd);
    cmd = arguments[0];

    // No login check: only stop and close allowed
    if(cmd == "stop") {
        stop_callback();
        return;
    }
    if(cmd == "close"){
        close_();
        return;
    }

    if(!logged_user.has_value()){
        if(cmd == "login"){
            process_login(arguments);
        }//if(login)

        if(DEBUG) write_str("Login needed to use the Back-up Server\n>> ");

        // richiedi al context di eseguire una nuova read
        // dopo aver dato la risposta al command
        // si comporta come un pool di thread
        read_command();

        return;
    }
    // Post new function to execute to server_old pool
    if(cmd == "checkFilesystemStatus"){
        process_checkFilesystemStatus();
        return;
    }

    if(arguments.size() > 1){
        std::string argument = arguments[1];

        switch(hashit(cmd)){
            case addFile:
                process_addOrUpdateFile(arguments);
                break;

            case updateFile:
                process_addOrUpdateFile(arguments);
                break;

            case getFile:
                process_getFile(arguments);
                break;

            case removeFile:
                process_removeFile(arguments);
                break;

            case unknown:
                process_unknown(cmd, argument);
                break;
        }
    }
    else {
        if(DEBUG) write_str("You should insert at least one argument for the desired command\n>> ");
        read_command();
    }
}

void connection_handler::read_command(){
    io_context->post([this]() {
        handle_read();
    });
}

void connection_handler::handle_read(){
    /**read_timer_.expires_from_now(server_.read_timeout);
    read_timer_.async_wait([this](boost::system::error_code err){
        if(!err){
            std::cout << "Read timeout occurred " << std::endl;
            shutdown_();
        }
    });*/

    // read until delimeter "\n"
    boost::asio::async_read_until(socket_, buf_in_, "\n",
            [this, self = shared_from_this()](boost::system::error_code err, std::size_t bytes_read){
        read_timer_.cancel();
        if(!err){
              const char* message = boost::asio::buffer_cast<const char*>(buf_in_.data());
              std::string message_command(message, bytes_read - (bytes_read > 1 && message[bytes_read - 2] == '\r' ? 2 : 1));
              buf_in_.consume(bytes_read);
              //debug
              if(bytes_read > 1){
                  std::cout << "\n[socket "<< &this->socket_ << "]Message read: " << message_command << ", bytes_read: " << bytes_read << std::endl;
                  process_command(message_command);
              }
              else{
                  read_command();
              }
          }
          else{
            std::cout << "\n[socket "<< &this->socket_ << "] Closed the Connection" << std::endl;
            close_();
              // throw err;
          }
  });
}// handle_read

std::string connection_handler::read_string(){
    boost::system::error_code err;
    boost::asio::streambuf buf;
    size_t bytes_read = boost::asio::read_until(socket_, buf_in_, "\n", err);

    if(!err){
        const char* message = boost::asio::buffer_cast<const char*>(buf_in_.data());
        std::string message_response(message);

        if(bytes_read > 1) {
          std::cout << "\n[socket " << &this->socket_ << "]Message read: "
                    << message_response << ", bytes_read: " << bytes_read
                    << std::endl;
          return message_response;
        }
        else{
            return read_string();
        }
    }

    else{
        std::cout << "[ERROR] read_string Error " << std::endl;
        close_();
        throw err;
    }

}// read_string


void connection_handler::handle_file_read(std::ofstream& output_file, std::size_t file_size){
    try{
        size_t len;
        size_t bytes_to_read = file_size;
        std::vector<char> buf(1024);
        boost::system::error_code error;
        bool end = false;

        while( !end ){

            if( bytes_to_read >= 1024 )
                //len = socket_.read_some(boost::asio::buffer(buf), error);
                len = boost::asio::read(socket_, boost::asio::buffer(buf), error);
            else{
                buf = std::vector<char>(bytes_to_read);
                len = socket_.read_some(boost::asio::buffer(buf), error);
            }
            if(!error){
                if (len>0) {
                    if(DEBUG) std::cout << "\n[socket "<< &this->socket_ << "] Parte di file: " << std::string(buf.begin(), buf.end()) << std::endl;
                    bytes_to_read -= len;
                    output_file.write(std::string(buf.begin(), buf.end()).c_str(), (std::streamsize) len);
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
        write_str("[SERVER_SUCCESS] File read successfully\n");
        std::cout<< "File read successfully" << std::endl;
        read_command();
    } catch (std::exception& e) {
        std::cout << "[ERROR] handle_file_read Error: " << e.what() << std::endl;
        output_file.close();
        close_();
        throw e;
    }

}// handle_file_read

void connection_handler::handle_write(){
    try{
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
    } catch (std::exception& e) {
        std::cout << "[ERROR] write_file Error: " << e.what() << std::endl;
        close_();
        throw e;
    }
}// handle_write

bool connection_handler::writing() const { return !buffer_seq_.empty();}

template <typename T>
void connection_handler::write_(T&& t){
    // Serialize the data first so we know how large it is.
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << t;
    std::string outbound_data_ = archive_stream.str() +"\n"; // \n Needed from the client in order to read correctly

    /**
     * Format the header.
     * Uncomment if needed
     * In the RemoteBackup project, clients doesn't read any header
     */
    // std::ostringstream header_stream;
    // header_stream << std::setw(8) << std::hex << outbound_data_.size();
    // std::string outbound_header_ = header_stream.str();

    std::lock_guard l(buffers_mtx_);
    // buffers_[active_buffer_^1].push_back(std::move(outbound_header_));
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

void connection_handler::write_file(std::ifstream& source_file, std::string path){

    char* buf = new char[1024];
    boost::system::error_code error;

    size_t file_size = source_file.tellg();
    source_file.seekg(0);

    if(DEBUG) { std::cout << "[DEBUG] Start sending file content" << std::endl; }

    long bytes_sent = 0;

    try{
        while(!source_file.eof()) {
            source_file.read(buf, (std::streamsize) 1024);

            int bytes_read_from_file = source_file.gcount();

            if (bytes_read_from_file <= 0) {
                std::cout << "[ERROR] Read file error" << std::endl;
                break;
                //TODO handle this error
            }

            boost::asio::write(this->socket_, boost::asio::buffer(
                    buf,source_file.gcount()), boost::asio::transfer_all(), error);
            if(error) {
                std::cout << "[ERROR] Send file error: " << error << std::endl;
                throw error;
            }

            bytes_sent += bytes_read_from_file;
        }
        source_file.close();
    } catch (std::exception& e) {
        std::cout << "[ERROR] write_file Error: " << error << std::endl;
        source_file.close();
        close_();
        throw e;
    }

}


void connection_handler::start(){
    std::cout << "Connection created " << std::endl;
    socket_.set_option(boost::asio::ip::tcp::no_delay(true));
    std::ostringstream oss;
    oss << "[++] Commands: " << std::endl;
    oss << "\t login [username] [password] -- login to the Back-up Server " << std::endl;
    oss << "\t close                       -- close the connetion to Server" << std::endl;
    oss << "\t stop                        -- stop the Server" << std::endl;
    oss << ">> ";
    if(DEBUG) write_str(oss.str());
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
        erase_callback(shared_from_this());
    }
}



