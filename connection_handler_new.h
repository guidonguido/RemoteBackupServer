//
// Created by guido on 28/11/20.
//

#ifndef REMOTEBACKUP_SERVER_CONNECTION_HANDLER_NEW_H
#define REMOTEBACKUP_SERVER_CONNECTION_HANDLER_NEW_H

#include <iostream>
#include <regex>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/impl/basic_text_oarchive.ipp>
#include <boost/archive/impl/text_oarchive_impl.ipp>
#include <boost/archive/basic_text_oprimitive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <unordered_set>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "User.h"


class connection_handler_new: public std::enable_shared_from_this<connection_handler_new> {

    std::shared_ptr<boost::asio::thread_pool> pool;
    std::shared_ptr<boost::asio::io_context> io_context;

    // Callback that deletes the connection from the list in server
    std::function<void(std::shared_ptr<connection_handler_new>)> erase_callback;
    std::function<void()> stop_callback;

    boost::asio::streambuf buf_in_;
    std::mutex buffers_mtx_;
    std::vector<std::string> buffers_[2]; // a double buffer
    std::vector<boost::asio::const_buffer> buffer_seq_;
    int active_buffer_ = 0;
    bool closing_ = false;
    bool closed_ = false;
    boost::asio::deadline_timer read_timer_, write_timer_;

    //std::optional<std::reference_wrapper<User>> logged_user;
    std::optional<User> logged_user;



    //Enum representing known commands
    enum command_code{
        addFile,
        getFile,
        updateFile,
        removeFile,
        checkFilesystemStatus,
        checkFileVersion,
        unknown
    };

    command_code hashit(std::string const& command);

    // Separate command from its argument
    std::vector<std::string> parse_command(const std::string cmd);

    void process_command(std::string cmd);

    void process_unknown(const std::string& cmd, const std::string& argument);

    void process_addOrUpdateFile(const std::vector<std::string>& arguments);

    void process_getFile(const std::vector<std::string>& arguments);

    void process_removeFile(const std::vector<std::string>& arguments);

    void process_login(const std::vector<std::string>& arguments);

    void process_checkFilesystemStatus();


public:
    boost::asio::ip::tcp::socket socket_;

    connection_handler_new(std::shared_ptr<boost::asio::thread_pool> pool,
            std::shared_ptr<boost::asio::io_context> io_context,
            std::function<void(std::shared_ptr<connection_handler_new>)>& erase_callback,
            std::function<void()>& stop_callback
    );


    void start();

    // socket creation
    boost::asio::ip::tcp::socket& get_socket();

    void close_();

    void shutdown_();


    template <typename T>
    void write_(T&& t);

    void write_str (std::string&& data);

    void read_command();

    void handle_read();

    // TO TEST!!
    // IDEA Trasforma la lettura in sincrona. La funzione sarà eseguito su un thread apposito e non c'è
    //      bisogno che venga eseguita la lettura async
    void handle_file_read(std::ofstream& output_file, size_t file_size);

    void handle_write();

    bool writing() const;


}; //connection_handler




#endif //REMOTEBACKUP_SERVER_CONNECTION_HANDLER_NEW_H
