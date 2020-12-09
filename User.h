//
// Created by guido on 26/09/20.
//

#ifndef PROVABOOSTASIO_USER_H
#define PROVABOOSTASIO_USER_H

#include <filesystem>
#include <fstream>
#include <string>
#include <optional>
#include <unordered_map>
#include <boost/filesystem/path.hpp>

class User {
    std::string folder_path;
    std::string username;

    User(const std::string username, std::string folder_path);

public:
    User(const User& other);
    User(const User&& other);
    User& operator=(const User& source);

    std::string get_username() const;
    std::string get_folder_path() const;

    void set_folder_path(const std::string&& path);

    std::unordered_map<std::string, std::string> get_filesystem_status();

    //static std::optional<std::reference_wrapper<User>> check_login(const std::string&& username, const std::string&& password);
    static std::optional<User> check_login(const std::string&& username, const std::string&& password);

};




#endif //PROVABOOSTASIO_USER_H
