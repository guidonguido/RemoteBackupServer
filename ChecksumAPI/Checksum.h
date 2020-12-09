//
// Created by Andrea Scoppetta on 30/11/20.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/serialization/string.hpp>
#include <boost/serialization/array.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "SHA256.h"

#ifndef REMOTEBACKUP_CLIENT_CHECKSUM_H
#define REMOTEBACKUP_CLIENT_CHECKSUM_H

std::string serialize_file(const std::string& file_path);

std::string get_file_checksum(const std::string& file_path);
bool checksums_equal(const std::string& checksum1, const std::string& checksum2);

#endif //REMOTEBACKUP_CLIENT_CHECKSUM_H
