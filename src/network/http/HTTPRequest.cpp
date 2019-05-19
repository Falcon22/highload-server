#include <algorithm>
#include <sstream>
#include "network/http/HTTPRequest.h"

namespace Server {
    namespace Http {

        std::string toLowerCase(std::string str) {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        }

        void HTTPRequest::parseRawHeader(const std::string &raw) {
            std::istringstream rawReader(raw);
            std::string header;

            // RequestLine parsing
            std::getline(rawReader, header);
            auto spIndex1 = header.find(' ');
            auto spIndex2 = header.find(' ', spIndex1 + 1);
            requestLine.method = header.substr(0, spIndex1);
            requestLine.request_uri = header.substr(spIndex1 + 1, spIndex2 - spIndex1 - 1);
            requestLine.http_version = header.substr(spIndex2 + 1, header.length() - spIndex2 - 2);

            // Headers parsing
            while (std::getline(rawReader, header) && header != "\r") {
                auto sepIndex = header.find(':');
                if (sepIndex != std::string::npos) {
                    headers.insert(std::make_pair(
                            toLowerCase(header.substr(0, sepIndex)),
                            header.substr(sepIndex + 2,
                                          header.length() - sepIndex - 3) // todo idk if SP is RFC required
                    ));
                }
            }

        }
    }
}
