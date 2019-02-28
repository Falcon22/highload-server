#include <network/Server.h>

#include <iostream>

#include <spdlog/spdlog.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <uv.h>

#include <config/ServerConfig.h>

namespace Server {
namespace Network {
    Server::Server() : config(std::make_shared<ServerConfig>()) {
    }

    Server::~Server() {}

    Server::Server(std::shared_ptr<ServerConfig> cfg) : config(std::move(cfg)) {}

    void Server::Start() {
        struct sockaddr_storage commands_addres;
        int rc = uv_ip4_addr("0.0.0.0", config->Port, (struct sockaddr_in *)&commands_addres);
        if (rc != 0) {
            throw std::runtime_error("Failed to call uv_ip4_addr");
        }
        spdlog::info("Server starting, port: {}", config->Port);
    }

    void Server::Stop() {

    }
}
}