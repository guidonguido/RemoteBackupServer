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

std::string get_file_checksum_WRONG(const std::string& file_path){
    std::string serialized_file = serialize_file(file_path);
    return sha256(serialized_file);
}

std::string get_file_checksum(const std::string& file_path){
    std::ifstream fp(file_path, std::ios::in | std::ios::binary);
    if (not fp.good()) {
        std::ostringstream os;
        os << "Cannot open \"" << file_path << "\": " << std::strerror(errno) << ".";
        throw std::runtime_error(os.str());
    }
    constexpr const std::size_t buffer_size { 1 << 12 };
    char buffer[buffer_size];
    unsigned char hash[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    while (fp.good()) {
        fp.read(buffer, buffer_size);
        SHA256_Update(&ctx, buffer, fp.gcount());
    }
    SHA256_Final(hash, &ctx);
    fp.close();
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        os << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return os.str();
}

bool checksums_equal(const std::string& checksum1, const std::string& checksum2){
    return checksum1 == checksum2;
}

