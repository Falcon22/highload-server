#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <list>
#include <memory>
#include "spdlog/spdlog.h"

namespace spdlog {
    class logger;
};

namespace Server {

class ServerConfig;


namespace Network {

    class Worker;

    class Server {
    public:
        Server();

        ~Server();

        Server(std::shared_ptr<ServerConfig> cfg);

        /**
        * Start server
        *
        * Allocates all resources, start threads if needs and begin execute received command on the given instance
         */
         void Start();

         /**
         * Stop server
         *
         * Stop accept new commands, waits until in flight commands finished execution and deallocated all resources
         * taken by the current instance
         */
         void Stop();

    private:

        // Server configuration
        std::shared_ptr<ServerConfig> config;


        // Threads that are processing network connections
        std::list<std::shared_ptr<Worker>> workers;

    };

} // namespace Network
} // namespace Server
#endif //SERVER_SERVER_H
