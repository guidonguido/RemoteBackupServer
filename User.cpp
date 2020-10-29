//
// Created by guido on 26/09/20.
//

#include "User.h"
#include <optional>
#include <boost/filesystem.hpp>

User::User(const std::string username, std::filesystem::path folder_path):
        username(username), folder_path(folder_path){}

User::User(const User &&other): username(std::move(other.username)), folder_path(std::move(other.folder_path)) {}

std::string User::get_username() const {
    return this->username;
}

void User::set_folder_path(const std::string &&path) {
        folder_path = path;
}

std::unordered_map<std::string, int> User::get_filesystem_status() {
    // boost::unordered_map<std::string, boost::filesystem::directory_entry> status;
    std::unordered_map<std::string, int> status;
    // for(auto entry: std::filesystem::recursive_directory_iterator(this->folder_path))
    for(auto entry: std::filesystem::recursive_directory_iterator("../synchronized_folders/guido"))
        status[entry.path().filename()] = 1;
    return status;
}

std::optional<std::reference_wrapper<User>> User::check_login(const std::string&& username, const std::string&& password) {
    std::ifstream ifile("../user_info/login_info");
    std::string line;
    std::size_t pos;

    std:: string username_, password_, path_;

    // Find a row in LOGIN_INFO that matches username ad password
    while(std::getline(ifile, line)){
        username_ = line.substr(line.find_first_not_of(' '), line.find(' '));
        line.erase(0, line.find(' '));
        line.erase(0, line.find_first_not_of(' '));

        password_ = line.substr(line.find_first_not_of(' '), line.find(' '));
        line.erase(0, line.find(' '));
        line.erase(0, line.find_first_not_of(' '));

        if(username_ == username && password_ == password){
            path_ = line.substr(line.find_first_not_of(' '), line.find(' '));
            ifile.close();
            User found_user(username_, std::filesystem::path(path_));
            return found_user;
        }
    }

    return std::nullopt;
}