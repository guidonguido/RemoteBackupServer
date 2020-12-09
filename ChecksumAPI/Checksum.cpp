//
// Created by Andrea Scoppetta on 29/11/20.
//

#include "Checksum.h"

std::string serialize_file(const std::string& file_path){
    std::ifstream source_file(file_path, std::ios_base::binary | std::ios_base::ate);
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    char buffer[1024];

    while(!source_file.eof()) {
        source_file.read(buffer, 1024);
        archive << buffer;
    }

    return archive_stream.str();
}

std::string get_file_checksum(const std::string& file_path){
    std::string serialized_file = serialize_file(file_path);
    return sha256(serialized_file);
}

bool checksums_equal(const std::string& checksum1, const std::string& checksum2){
    return checksum1 == checksum2;
}

