#include "network/http/HTTPContext.h"
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>

namespace Server {
    namespace Http {

        IOStatus HTTPContext::writeCompletely(const char *buffer, size_t size) {
            while (written != size) {
                nn = write(sd, buffer + written, size - written);
                if (nn == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        return IOStatus::wouldBlock;
                    }

                    return IOStatus::error;
                }

                written += nn;
            }

            written = 0;
            return IOStatus::success;
        }

        HTTPContext::HTTPContext(uv_stream_t* conn) : conn(conn), fd(0) {
            buffer = new char[1024];
            sd = conn->io_watcher.fd;
        }

        void HTTPContext::writeFile() {
            if (p.empty() || bodyDone)
                return;

            if (unfinishedTask == TaskType::no) {
                sizeLeft = fs::file_size(p);
                fd = open(p.c_str(), O_RDONLY);
                if (fd < 0) {
                    spdlog::debug("can't open file, {}", strerror(errno));
                    return;
                }
            }

            while (sizeLeft != 0) {
                unfinishedTask = TaskType::no; // resuming task
                written = sendfile(sd, fd, nullptr, sizeLeft);
                if (written != -1) {
                    sizeLeft -= written;
                } else {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        unfinishedTask = TaskType::writeFile;
                    } else {
                        spdlog::debug("sendfile, errono {}", strerror(errno));
                    }

                    return;
                }
            }

            bodyDone = true;
        }

        void HTTPContext::writeHeader() {
            if (headerDone) {
                return;
            }

            std::string responseHeader = response.getHeader();
            writeCompletely(responseHeader.c_str(), responseHeader.length());
            headerDone = true;
        }

        bool HTTPContext::hasUnfinishedTask() {
            return unfinishedTask != TaskType::no;
        }

        HTTPContext::~HTTPContext() {
            if (fd > 0) {
                close(fd);
            }

            delete[] buffer;
        }

        void HTTPContext::resumeTask() {
            if (unfinishedTask == TaskType::writeFile) {
                writeFile();
            }
        }

        void HTTPContext::attachFile(const std::filesystem::path &p) {
            this->p = p;
        }
    }
}
