#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <memory>
#include <string>

namespace Server {

class ServerConfig;

class Config {
private:
    std::shared_ptr<ServerConfig> serverConfig;

public:
    Config();
    ~Config();

    void Load(const std::string file);

    std::shared_ptr<ServerConfig> GetServerConfig() const { return serverConfig; }
};

} // namespace Server

#endif //SERVER_CONFIG_H
