#include "network/http/HTTPResponse.h"
#include <spdlog/fmt/fmt.h>

namespace Server {
    namespace Http {
        std::string HTTPResponse::getHeader() {
            std::string header(fmt::format(
                    "{} {} {}\r\n",
                    statusLine.http_version, statusLine.status_code, getReasonPhrase(statusLine.status_code)
            ));

            std::for_each(headers.begin(), headers.end(),
                          [&header](auto pair) { header += fmt::format("{}: {}\r\n", pair.first, pair.second); });

            header += "\r\n";

            return header;
        }
    }
}
