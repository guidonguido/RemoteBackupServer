//
// Created by guido on 26/09/20.
//

#include "User.h"
#include <optional>

User::User(const std::string username, std::filesystem::path folder_path):
        username(username), folder_path(folder_path){}

User::User(const User &&other): username(std::move(other.username)), folder_path(std::move(other.folder_path)) {}

void User::set_folder_path(const std::string &&path) {
        folder_path = path;
}

std::optional<User> User::check_login(const std::string &&username, const std::string &&password) {
    std::ifstream ifile("./user_info/login_info.txt");
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
            return std::move(found_user);
        }
    }

    return std::nullopt;
}