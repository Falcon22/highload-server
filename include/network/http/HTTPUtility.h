#ifndef SERVER_HTTPUTILITY_H
#define SERVER_HTTPUTILITY_H

#include <string>
#include <regex>
#include <filesystem>
#include <fstream>

namespace Server {
namespace Http {

    namespace fs = std::filesystem;

    namespace utility {
        std::string URLDecode(const std::string &uri);
        std::string URLEncode(const std::string &uri);

        std::string RFC1123Time(time_t time);
        std::string getContentType(const fs::path &path);
    }

} // namespace Http
} // namespace Server




#endif //SERVER_HTTPUTILITY_H
