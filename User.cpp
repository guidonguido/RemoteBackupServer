//
// Created by guido on 26/09/20.
//

#include "User.h"
#include <optional>
#include <boost/filesystem.hpp>
#include <iostream>
#include "ChecksumAPI/Checksum.h"

User::User(const std::string username, std::string folder_path):
        username(username), folder_path(folder_path){}

User::User(const User &&other): username(std::move(other.username)), folder_path(std::move(other.folder_path)) {}
User::User(const User &other): username(other.username), folder_path(other.folder_path) {}
User& User::operator=(const User& source){
    if(this != &source){
        this->username = source.username;
        this->folder_path = source.folder_path;
    }
    return *this;
}


std::string User::get_username() const {
    return this->username;
}

std::string User::get_folder_path() const {
    return this->folder_path;
}

void User::set_folder_path(const std::string &&path) {
        folder_path = path;
}

std::unordered_map<std::string, std::string> User::get_filesystem_status() {
    std::unordered_map<std::string, std::string> status;
    std::string root_path(this->folder_path);


    for(auto entry: std::filesystem::recursive_directory_iterator(this->folder_path)){
        std::string relative_path = entry.path().string().substr(
                root_path.length()+1, entry.path().string().length()-root_path.length()-1);
        if( !entry.is_directory() ){
            status[relative_path] = get_file_checksum(entry.path().string());
        } else{
            status[relative_path] = std::string("");
        }

    }

    return status;
}

//std::optional<std::reference_wrapper<User>> User::check_login(const std::string&& username, const std::string&& password) {
std::optional<User> User::check_login(const std::string&& username, const std::string&& password) {
    std::ifstream ifile("../user_info/login_info");
    std::string line;
    std::size_t pos;

    std:: string username_, password_;
    std::ostringstream path_;
    std::cout << "Findind user... from " << std::filesystem::current_path().string() << std::endl;

    // Find a row in LOGIN_INFO that matches username ad password
    while(std::getline(ifile, line)){
        username_ = line.substr(line.find_first_not_of(' '), line.find(' '));
        line.erase(0, line.find(' '));
        line.erase(0, line.find_first_not_of(' '));

        password_ = line.substr(line.find_first_not_of(' '), line.find(' '));
        line.erase(0, line.find(' '));
        line.erase(0, line.find_first_not_of(' '));

        std::cout << "Username: " << username << " , Password: " << password << std::endl;

        if(username_ == username && password_ == password){
            path_ << "../synchronized_folders/";
            path_ << line.substr(line.find_first_not_of(' '), line.find(' '));
            ifile.close();
            User found_user(username_, path_.str());
            return found_user;
        }
    }

    return std::nullopt;
}