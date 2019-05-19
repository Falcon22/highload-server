#ifndef SERVER_HTTPREQUEST_H
#define SERVER_HTTPREQUEST_H

#include <string>
#include <unordered_map>

namespace Server {
    namespace Http {
        struct RequestLine {
            std::string method;
            std::string request_uri;
            std::string http_version;
        };

// idk wat im doin
        std::string toLowerCase(std::string str);

        struct HTTPRequest {
            RequestLine requestLine;
            std::unordered_map<std::string, std::string> headers;

            void parseRawHeader(const std::string &raw);
        };
    }
}
#endif