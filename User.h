//
// Created by guido on 26/09/20.
//

#ifndef PROVABOOSTASIO_USER_H
#define PROVABOOSTASIO_USER_H

#include <filesystem>
#include <fstream>
#include <string>
#include <optional>

class User {
    std::filesystem::path folder_path;
    std::string username;

    User(const std::string username, std::filesystem::path folder_path);
    User(const User& other) = delete;
public:
    User(const User&& other);

    std::string get_username() const;

    void set_folder_path(const std::string&& path);

    static std::optional<std::reference_wrapper<User>> check_login(const std::string&& username, const std::string&& password);
};




#endif //PROVABOOSTASIO_USER_H
