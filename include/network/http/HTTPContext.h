#ifndef SERVER_HTTPCONTEXT_H
#define SERVER_HTTPCONTEXT_H

#include "network/http/HTTPRequest.h"
#include "network/http/HTTPResponse.h"
#include <filesystem>
#include <fstream>
#include <uv.h>

namespace Server {
    namespace Http {
        namespace fs = std::filesystem;

        enum class IOStatus {
            wouldBlock = -2,
            error = -1,
            success = 0,
        };

        enum class TaskType {
            no = 0,
            writeFile = 1,
            writeData,
            read,
        };

        class HTTPContext {
        public:
            explicit HTTPContext(uv_stream_t* conn = nullptr);

            virtual ~HTTPContext();

            HTTPRequest request;
            HTTPResponse response;

            void writeHeader();

            void attachFile(const fs::path &p);

            void writeFile();

            bool hasUnfinishedTask();

            void resumeTask();

            uv_stream_t* conn;

            int sd;

        private:
            TaskType unfinishedTask{TaskType::no};

            IOStatus writeCompletely(const char *buffer, size_t size);

            ssize_t written{0}, nn{0};
            uintmax_t sizeLeft{0};

            bool headerDone{false};
            bool bodyDone{false};

            std::filesystem::path p;

            char *buffer;
            int fd;
        };
    }
}


#endif //EESKORKA_HTTPCONTEXT_H
