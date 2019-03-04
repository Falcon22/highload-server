#include <network/Server.h>

#include <spdlog/spdlog.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include <config/ServerConfig.h>

#include <network/Worker.h>

namespace Server {
namespace Network {
    Server::Server() : config(std::make_shared<ServerConfig>()) {
        for (int i = 0; i < config->Workers; i++) {
            std::shared_ptr<Worker> worker = std::make_shared<Worker>(i, config->WorkerTaskLimit, config->SlowRequestMillisecondTime);
            workers.push_back(worker);
        }
    }

    Server::~Server() {}

    Server::Server(std::shared_ptr<ServerConfig> cfg) : config(std::move(cfg)) {}

    void Server::Start() {
        struct sockaddr_storage commands_addres;
        int rc = uv_ip4_addr("0.0.0.0", config->Port, (struct sockaddr_in *)&commands_addres);
        if (rc != 0) {
            spdlog::error("Failed to call uv_ip4_addr: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_ip4_addr");
        }
        for (const auto &worker : workers) {
            worker->Start(commands_addres);
        }
        spdlog::info("Server starting, port: {}", config->Port);
    }

    void Server::Stop() {
        for (const auto &worker : workers) {
            worker->Stop();
        }
        spdlog::info("Server stopped");
    }
}
}